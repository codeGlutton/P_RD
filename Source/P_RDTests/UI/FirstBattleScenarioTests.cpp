#include "Misc/AutomationTest.h"
#include "Tutorial/FirstBattleScenario.h"
#include "Tutorial/GuidedTutorial.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Tutorial/FirstPlayTutorialSubsystem.h"
#include "SRPGFramework/SRPGAction.h"
#include "Engine/GameInstance.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Component/SkillComponent/UnitSkillComponentModel.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialEnemyColdSkillsTest, "P_RD.Tutorial.Guided.EnemyColdSkills",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialEnemyColdSkillsTest::RunTest(const FString&)
{
	// Run in a fresh editor process, before UI/roster tests load the enemy skill assets.
	auto* Spawn = LoadObject<UStaticEnemyUnitSpawnData>(nullptr,
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_EagleUnit.DA_EagleUnit"));
	if (!TestNotNull(TEXT("Actual scripted enemy spawn data exists"), Spawn)) return false;
	UClass* EnemyClass = Spawn->mModelClass.LoadSynchronous();
	if (!TestNotNull(TEXT("Authored concrete enemy model exists"), EnemyClass)) return false;
	int32 Pending = 0;
	for (const auto& Skill : Spawn->mSkillDatas) Pending += Skill.IsPending() ? 1 : 0;
	TestTrue(TEXT("Regression starts with unloaded soft skill references"), Pending > 0);
	for (int32 EnemyIndex = 0; EnemyIndex < 2; ++EnemyIndex)
	{
		auto* Enemy = NewObject<UEnemyUnitModel>(GetTransientPackage(), EnemyClass);
		auto* Component = Enemy->GetSkillComponentModel();
		if (!TestNotNull(TEXT("Real enemy skill component exists"), Component)) return false;
		// Same assignment used by UEnemyUnitModel::PostInitializeComponentModels.
		Component->SetSkillFrom(Spawn->mSkillDatas);
		for (int32 Index = 0; Index < Spawn->mSkillDatas.Num(); ++Index)
		{
			const auto* Entry = Component->GetSkill(Index);
			TestTrue(TEXT("Each original slot contains an executable skill"), Entry && Entry->mData && !Entry->mData->mSkillPhaseLayers.IsEmpty());
			TestTrue(TEXT("Enemy receives the authored skill, not a fallback"), Entry && Entry->mData == Spawn->mSkillDatas[Index].Get());
		}
	}
	AddInfo(FString::Printf(TEXT("Assigned %d authored skills to both tutorial enemies; %d were initially unloaded."), Spawn->mSkillDatas.Num(), Pending));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialBuildActionTest, "P_RD.Tutorial.Guided.BuildActionRemainsInteractive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialBuildActionTest::RunTest(const FString&)
{
	auto* Tutorial = NewObject<UFirstPlayTutorialSubsystem>(NewObject<UGameInstance>());
	for (const TCHAR* Name : {TEXT("SRPGMoveBuildAction"), TEXT("SRPGSkillBuildAction")})
	{
		const FString Path = FString::Printf(TEXT("/Script/P_RD.%s"), Name);
		auto* Class = LoadObject<UClass>(nullptr, *Path);
		if (!TestNotNull(TEXT("Real build action class"), Class)) continue;
		auto* Build = static_cast<USRPGAction*>(NewObject<UObject>(GetTransientPackage(), Class));
		Tutorial->ActionStarted(Build);
		TestFalse(TEXT("Choosing a path or enemy never hides/locks the instruction"), Tutorial->IsWaitingForActionForTest());
		auto* PlayClass = LoadObject<UClass>(nullptr, TEXT("/Script/P_RD.SRPGMoveAction"));
		if (!TestNotNull(TEXT("Real movement action class"), PlayClass)) continue;
		auto* Play = static_cast<USRPGAction*>(NewObject<UObject>(GetTransientPackage(), PlayClass));
		Tutorial->ActionStarted(Play);
		TestTrue(TEXT("Actual movement animation blocks interaction"), Tutorial->IsWaitingForActionForTest());
		Tutorial->ActionEnded(true, Build, true);
		TestTrue(TEXT("Build cleanup cannot unlock a running animation"), Tutorial->IsWaitingForActionForTest());
		Tutorial->ActionEnded(false, Play, false);
		TestFalse(TEXT("Actual action completion releases animation lock"), Tutorial->IsWaitingForActionForTest());
	}
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstBattleScenarioTest, "P_RD.Tutorial.Guided.FixedScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFirstBattleScenarioTest::RunTest(const FString&)
{
	auto* Source = LoadObject<UStaticCombatRoomSpawnData>(nullptr,
		TEXT("/Game/BP/DataAsset/Room/Monster/DA_TestMonster_Stage1_0.DA_TestMonster_Stage1_0"));
	if (!TestNotNull(TEXT("Authored first-room template exists"), Source)) return false;
	const auto OriginalStart = Source->mPlayerTransforms[0];
	auto* Room = DuplicateObject<UStaticCombatRoomSpawnData>(Source, GetTransientPackage());
	FFirstBattleScenario::ApplyLayout(*Room);
	TestTrue(TEXT("Template asset was not changed"), Source->mPlayerTransforms[0] == OriginalStart);
	TSet<FTileIndex> Occupied;
	for (const auto& Player : Room->mPlayerTransforms)
	{
		TestFalse(TEXT("Party placements do not overlap"), Occupied.Contains(Player.mIndex));
		Occupied.Add(Player.mIndex);
	}
	for (const auto& Enemy : Room->mEnemyUnitPlacementDatas)
	{
		TestFalse(TEXT("Enemy placements do not overlap"), Occupied.Contains(Enemy.mTransform.mIndex));
		Occupied.Add(Enemy.mTransform.mIndex);
	}
	for (const auto& Obstacle : Room->mObstaclePlacementDatas)
	{
		TestFalse(TEXT("Authored combatants are not inside scenery"), Occupied.Contains(Obstacle.mTransform.mIndex));
		Occupied.Add(Obstacle.mTransform.mIndex);
	}
	TestFalse(TEXT("Required movement destination is empty"), Occupied.Contains(FFirstBattleScenario::Move()));
	TestTrue(TEXT("First party slot has the authored start"), Room->mPlayerTransforms[0].mIndex == FFirstBattleScenario::Start());
	TestTrue(TEXT("Attack target is an actual enemy placement"), Room->mEnemyUnitPlacementDatas[0].mTransform.mIndex == FFirstBattleScenario::Enemy());
	TestEqual(TEXT("Second enemy prevents premature room completion"), Room->mEnemyUnitPlacementDatas.Num(), 2);
	for (int32 X = 0; X < 10; ++X) for (int32 Y = 0; Y < 10; ++Y)
	{
		const FTileIndex Tile(X, Y);
		TestEqual(TEXT("Only the authored move tile is accepted"), FFirstBattleScenario::AllowsTile(EGuidedStage::MoveTile, Tile), Tile == FFirstBattleScenario::Move());
		TestEqual(TEXT("Only the authored enemy tile is accepted"), FFirstBattleScenario::AllowsTile(EGuidedStage::SkillTarget, Tile), Tile == FFirstBattleScenario::Enemy());
		TestFalse(TEXT("Board cannot bypass reading the skill"), FFirstBattleScenario::AllowsTile(EGuidedStage::HoldSkill, Tile));
	}
	auto* Map = NewObject<UTileMapModel>();
	Map->SetDimensions(10, 10);
	for (const TCHAR* Job : {TEXT("Knight"), TEXT("Ranger"), TEXT("Mage"), TEXT("Rogue"), TEXT("Druid"), TEXT("Barbarian")})
	{
		const FString Path = FString::Printf(TEXT("/Game/BP/DataAsset/Unit/PlayerUnit/DA_Test%sPlayerUnit.DA_Test%sPlayerUnit"), Job, Job);
		auto* Unit = LoadObject<UStaticPlayerUnitSpawnData>(nullptr, *Path);
		if (!TestNotNull(*Path, Unit)) continue;
		TArray<const UStaticUnitSkillData*> Skills;
		for (const auto& Soft : Unit->mSkillDatas)
		{
			const auto* Skill = Cast<UStaticUnitSkillData>(Soft.LoadSynchronous());
			Skills.Add(Skill);
		}
		const int32 SelectedIndex = FFirstBattleScenario::SelectSkill(Skills);
		const auto* Selected = Skills.IsValidIndex(SelectedIndex) ? Skills[SelectedIndex] : nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("%s has a targeted scripted skill"), Job), Selected)) continue;
		TestTrue(*FString::Printf(TEXT("%s can aim the fixed target after moving"), Job),
			Map->CanAim(FFirstBattleScenario::Move(), FFirstBattleScenario::Enemy(), Selected->mAimRange,
				Selected->mAimPattern, Selected->mCanAimBoardActor, static_cast<ETileLayerFlag>(Selected->mAimBlockerMask)));
		AddInfo(FString::Printf(TEXT("%s scripted skill: %s"), Job, *Selected->mName.ToString()));
	}
	return !HasAnyErrors();
}
#endif
