#include "Tutorial/FirstPlayTutorialSubsystem.h"
#include "Tutorial/FirstBattleScenario.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "Component/SkillComponent/UnitSkillComponentModel.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "UI/Combat/CombatUIModel.h"
#include "ObjectView.h"
#include "RDCollision.h"

UStaticCombatRoomSpawnData* UFirstPlayTutorialSubsystem::PrepareScenario(
	UStaticCombatRoomSpawnData* Original, const TArray<TObjectPtr<UPlayerUnitModel>>& Party)
{
	ScenarioRoom = nullptr;
	ScenarioUnit.Reset();
	ScenarioSkillIndex = INDEX_NONE;
	auto& Flow = GetUserMutableData()->GuidedTutorial;
	if (!Flow.Enrolled || Flow.FirstBattleFinished || Party.IsEmpty() || !Party[0]) return Original;
	const auto& Skills = Party[0]->GetSkillComponentModel()->GetSkills();
	TArray<const UStaticUnitSkillData*> SkillData;
	for (const auto& Skill : Skills) SkillData.Add(Cast<UStaticUnitSkillData>(Skill.mData));
	ScenarioSkillIndex = FFirstBattleScenario::SelectSkill(SkillData);
	if (ScenarioSkillIndex != INDEX_NONE) ScenarioSkillName = SkillData[ScenarioSkillIndex]->mName;
	if (ScenarioSkillIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("First battle scenario needs a basic attack in party slot 0."));
		return Original;
	}
	auto* Base = LoadObject<UStaticCombatRoomSpawnData>(nullptr,
		TEXT("/Game/BP/DataAsset/Room/Monster/DA_TestMonster_Stage1_0.DA_TestMonster_Stage1_0"));
	if (!Base) return Original;
	ScenarioRoom = DuplicateObject<UStaticCombatRoomSpawnData>(Base, this);
	FFirstBattleScenario::ApplyLayout(*ScenarioRoom);
	ScenarioUnit = Party[0];
	if (!Flow.CoreComplete)
	{
		// Combat positions are recreated on room load, so resume the authored opening from its start.
		Flow.Stage = EGuidedStage::OpenSkills;
		Flow.Lesson = EGuidedLesson::None;
	}
	UE_LOG(LogTemp, Display, TEXT("Tutorial scenario: unit=%s move=(4,3) skill=%d target=(4,4)"),
		*Party[0]->GetBoardActorDisplayName().ToString(), ScenarioSkillIndex);
	return ScenarioRoom;
}

bool UFirstPlayTutorialSubsystem::IsScenarioGuiding() const
{
	return HasScenario() && ScenarioUnit.IsValid() && GetUserMutableData()->GuidedTutorial.CanGuideCombat();
}

bool UFirstPlayTutorialSubsystem::IsScenarioTileAllowed(const FTileIndex& Tile) const
{
	if (!IsScenarioGuiding()) return true;
	return FFirstBattleScenario::AllowsTile(GetUserMutableData()->GuidedTutorial.Stage, Tile);
}

bool UFirstPlayTutorialSubsystem::IsScenarioPointerAllowed(const FVector2D& Position) const
{
	if (!IsScenarioGuiding()) return true;
	const auto* Combat = GetWorldSubsystemModel<USRPGCombatModel>(GetWorld());
	const auto* Map = Combat ? Combat->GetTileMap() : nullptr;
	auto* PC = GetWorld()->GetFirstPlayerController();
	if (!Map || !PC) return false;
	FVector2D Pixel, Viewport;
	USlateBlueprintLibrary::AbsoluteToViewport(this, Position, Pixel, Viewport);
	FHitResult Hit;
	if (!PC->GetHitResultAtScreenPosition(Pixel, RDTraceChannels::TileAnyTrace, false, Hit)) return false;
	FTileIndex Tile = FTileIndex::Invalid;
	if (Hit.GetActor() == Map->GetView<AActor>()) Tile = Map->WorldToTileIndex(Hit.ImpactPoint);
	else if (auto* View = Cast<IObjectView>(Hit.GetActor()))
		if (const auto* Model = View->GetModel<UBoardActorModel>()) Tile = Model->GetTileTransform().mIndex;
	return IsScenarioTileAllowed(Tile);
}

bool UFirstPlayTutorialSubsystem::IsScenarioCommandAllowed(ECombatInputType Type, int32 Payload) const
{
	if (!IsScenarioGuiding()) return true;
	const auto Stage = GetUserMutableData()->GuidedTutorial.Stage;
	switch (Type)
	{
	case ECombatInputType::Move: return Stage == EGuidedStage::SelectMove;
	case ECombatInputType::SelectSkill: return Stage == EGuidedStage::SelectSkill && Payload == ScenarioSkillIndex;
	case ECombatInputType::LongPressSkill: return Stage == EGuidedStage::HoldSkill && Payload == ScenarioSkillIndex;
	case ECombatInputType::Confirm: return Stage == EGuidedStage::ConfirmMove || Stage == EGuidedStage::ConfirmSkill;
	case ECombatInputType::EndTurn: return Stage == EGuidedStage::EndTurn;
	case ECombatInputType::LongPressUnit: return Payload == INDEX_NONE; // Closing the actual skill details.
	case ECombatInputType::FocusUnit: return true; // Camera anchoring requested by the instructed card.
	default: return false;
	}
}

bool UFirstPlayTutorialSubsystem::GetScenarioFocus(TArray<FVector2D>& Corners) const
{
	Corners.Reset();
	if (!IsScenarioGuiding()) return false;
	const auto Stage = GetUserMutableData()->GuidedTutorial.Stage;
	FTileIndex Tile = FTileIndex::Invalid;
	if (Stage == EGuidedStage::MoveTile || Stage == EGuidedStage::ConfirmMove) Tile = FFirstBattleScenario::Move();
	if (Stage == EGuidedStage::SkillTarget || Stage == EGuidedStage::ConfirmSkill) Tile = FFirstBattleScenario::Enemy();
	if (Tile == FTileIndex::Invalid) return false;
	const auto* Combat = GetWorldSubsystemModel<USRPGCombatModel>(GetWorld());
	const auto* Map = Combat ? Combat->GetTileMap() : nullptr;
	auto* PC = GetWorld()->GetFirstPlayerController();
	if (!Map || !PC) return false;
	const FVector Center = Map->TileToWorldLocation(Tile) + FVector(0, 0, 3);
	const FVector X = (Map->TileToWorldLocation(Tile + FTileIndex(1, 0)) - Map->TileToWorldLocation(Tile)) * .46;
	const FVector Y = (Map->TileToWorldLocation(Tile + FTileIndex(0, 1)) - Map->TileToWorldLocation(Tile)) * .46;
	for (const FVector Point : {Center-X-Y, Center+X-Y, Center+X+Y, Center-X+Y})
	{
		FVector2D Pixel, Absolute;
		if (!PC->ProjectWorldLocationToScreen(Point, Pixel)) { Corners.Reset(); return false; }
		// Keep desktop absolute coordinates; the true option subtracts the window position.
		USlateBlueprintLibrary::ScreenToWidgetAbsolute(this, Pixel, Absolute);
		Corners.Add(Absolute);
	}
	return true;
}
