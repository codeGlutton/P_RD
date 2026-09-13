#include "Misc/AutomationTest.h"
#include "Tutorial/StaticTutorialRoomSpawnData.h"
#include "Tutorial/FirstBattleScenario.h"
#include "Tutorial/GuidedTutorial.h"
#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"
#include "PCGStage/StageBuilder.h"
#include "Setting/GameBalanceSettings.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialDataEditingTest, "P_RD.Tutorial.RoomData.EditedTargetsAndFootprints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialDataEditingTest::RunTest(const FString&)
{
	auto* Source = FFirstBattleScenario::LoadTemplate();
	if (!TestNotNull(TEXT("Tutorial DA loads independently of normal rooms"), Source)) return false;
	auto* Room = DuplicateObject<UStaticTutorialRoomSpawnData>(Source, GetTransientPackage());
	Room->MoveDestination = {2, 3};
	Room->TargetEnemyIndex = 1;
	Room->mEnemyUnitPlacementDatas[1].mTransform.mIndex = {2, 4};
	TestTrue(TEXT("Edited DA movement drives input acceptance"), FFirstBattleScenario::AllowsTile(*Room, EGuidedStage::MoveTile, {2, 3}));
	TestFalse(TEXT("Old hardcoded movement no longer accepted"), FFirstBattleScenario::AllowsTile(*Room, EGuidedStage::MoveTile, {4, 3}));
	TestTrue(TEXT("Edited enemy index and position drive targeting"), FFirstBattleScenario::AllowsTile(*Room, EGuidedStage::SkillTarget, {2, 4}));
	// Place an obstacle whose anchor is clear, but whose rotated extra footprint hits an enemy.
	auto* Obstacle = NewObject<UStaticObstacleSpawnData>();
	Obstacle->mRequiredEmptyTiles.Add({1, 0});
	FObstaclePlacementData Placement;
	Placement.mSpawnData = Obstacle;
	Placement.mTransform = FTileTransform({8, 8}, ETileActorDirection::Right);
	Room->mEnemyUnitPlacementDatas[1].mTransform.mIndex = LocalToTileMapTransform(FTileTransform({1, 0}), Placement.mTransform).mIndex;
	Room->mObstaclePlacementDatas = {Placement};
	TArray<FText> Errors;
	TestFalse(TEXT("Rotated multi-tile overlap is rejected"), Room->ValidateLayout(Errors));
	TestTrue(TEXT("Overlap is identified specifically"), Errors.ContainsByPredicate([](const FText& Error) { return Error.ToString().Contains(TEXT("overlapping occupancy")); }));
	Room->mObstaclePlacementDatas.Reset();
	Room->mStageLevel = EStageLevelType::Stage1;
	Errors.Reset();
	TestFalse(TEXT("Accidentally enabling random tutorial selection fails validation"), Room->ValidateLayout(Errors));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialRunAbandonTest, "P_RD.Tutorial.RoomData.CompletedThenAbandoned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialRunAbandonTest::RunTest(const FString&)
{
	FGuidedTutorialProgress Progress;
	TestFalse(TEXT("Existing unenrolled profiles keep normal rooms"), Progress.NeedsFirstRoom(false));
	Progress.EnrollIfNoPlayData(false);
	TestTrue(TEXT("New profile receives tutorial first room"), Progress.NeedsFirstRoom(false));
	Progress.SystemsComplete = true;
	Progress.Stage = EGuidedStage::EndTurn;
	Progress.Advance(EGuidedStage::EndTurn);
	TestTrue(TEXT("Actual final instruction records completion"), Progress.CoreComplete);
	TestFalse(TEXT("Battle has not finished when run is abandoned"), Progress.FirstBattleFinished);
	Progress.ResumeInNewBattle();
	TestFalse(TEXT("Completed instructions cannot force next run's first room"), Progress.NeedsFirstRoom(false));
	Progress = FGuidedTutorialProgress();
	Progress.EnrollIfNoPlayData(false);
	TestFalse(TEXT("Skipping opts out even before first victory"), Progress.NeedsFirstRoom(true));
	TestTrue(TEXT("Unfinished non-skipped instruction may restart on a new run"), Progress.NeedsFirstRoom(false));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialFirstRoomBuilderTest, "P_RD.Tutorial.RoomData.FirstRoomOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialFirstRoomBuilderTest::RunTest(const FString&)
{
	auto* Room = FFirstBattleScenario::LoadTemplate();
	if (!TestNotNull(TEXT("Tutorial asset exists"), Room)) return false;
	const auto Id = Room->GetPrimaryAssetId();
	TestTrue(TEXT("Separate tutorial folder is registered for asset loading/cooking"), UAssetManager::Get().GetPrimaryAssetPath(Id).IsValid());
	TestTrue(TEXT("Tutorial is excluded through existing StageLevel filter"), Room->mStageLevel == EStageLevelType::None);
	const auto* Settings = GetDefault<UGameBalanceSettings>();
	const auto* Table = Settings->mStageBuildSettingTable.LoadSynchronous();
	if (!TestNotNull(TEXT("Actual stage build settings"), Table)) return false;
	FLevelAttributeCache Cache;
	Cache.mRarityRates.Init(FRarityRate{1.f, 0.f, 0.f}, 100);
	Cache.mPrices.Init(100.f, 100);
	Cache.mMaxExps.Init(100.f, 100);
	for (const TCHAR* StageName : {TEXT("Stage1"), TEXT("Stage2"), TEXT("Stage3")})
	{
		const auto* Params = Table->FindRow<FStageBuilderParams>(FName(StageName), TEXT("Tutorial regression"));
		if (!TestNotNull(TEXT("Stage table row"), Params)) continue;
		for (int32 Seed = 1; Seed <= 8; ++Seed)
		{
			FRandomStream NormalStream(Seed), TutorialStream(Seed);
			auto Normal = FStageBuilder::Make(NormalStream, Settings->mGlobalStageBuildSetting, Cache, *Params).Build();
			auto Tutorial = FStageBuilder::Make(TutorialStream, Settings->mGlobalStageBuildSetting, Cache, *Params).SetFirstRoomOverride(Id).Build();
			TestEqual(TEXT("Override consumes no extra randomness"), NormalStream.GetCurrentSeed(), TutorialStream.GetCurrentSeed());
			for (int32 Row = 0; Row < Normal.mRoomRows.Num(); ++Row)
				for (int32 Column = 0; Column < Normal.mRoomRows[Row].mRooms.Num(); ++Column)
				{
					const auto* A = Normal.mRoomRows[Row].mRooms[Column].GetPtr<FRoom>();
					const auto* B = Tutorial.mRoomRows[Row].mRooms[Column].GetPtr<FRoom>();
					if (!A || !B) continue;
					TestTrue(TEXT("Normal generation never picks tutorial"), A->mStaticRoomSpawnDataId != Id);
					if (Row == 0 && Column == Tutorial.mStartColumn)
						TestTrue(TEXT("Only start room uses override"), B->mStaticRoomSpawnDataId == Id);
					else
						TestTrue(TEXT("All subsequent room selections unchanged"), A->mStaticRoomSpawnDataId == B->mStaticRoomSpawnDataId);
				}
		}
	}
	return !HasAnyErrors();
}
#endif
