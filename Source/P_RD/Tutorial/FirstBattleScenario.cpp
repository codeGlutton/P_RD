#include "Tutorial/FirstBattleScenario.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "Tutorial/GuidedTutorial.h"

int32 FFirstBattleScenario::SelectSkill(const TArray<const UStaticUnitSkillData*>& Skills)
{
	// Prefer an attack; Druid's starting attacks target itself, so teach its targeted control skill.
	for (const bool AttackOnly : {true, false})
		for (int32 Index = 0; Index < Skills.Num(); ++Index)
		{
			const auto* Skill = Skills[Index];
			if (Skill && (!AttackOnly || Skill->mSkillType == ESkillType::Attack)
				&& Skill->mAimPattern != EAimPattern::Single && Skill->mCanAimBoardActor
				&& Skill->mAimRange >= 1 && Skill->mRequiredActionPoint <= 10) return Index;
		}
	return INDEX_NONE;
}

bool FFirstBattleScenario::AllowsTile(EGuidedStage Stage, const FTileIndex& Tile)
{
	if (Stage == EGuidedStage::MoveTile || Stage == EGuidedStage::ConfirmMove) return Tile == Move();
	if (Stage == EGuidedStage::SkillTarget || Stage == EGuidedStage::ConfirmSkill) return Tile == Enemy();
	return false;
}

void FFirstBattleScenario::ApplyLayout(UStaticCombatRoomSpawnData& Room)
{
	Room.mPlayerTransforms = {
		FTileTransform(Start(), ETileActorDirection::Right),
		FTileTransform({3, 2}, ETileActorDirection::Right),
		FTileTransform({5, 2}, ETileActorDirection::Right)};
	// Keep a second enemy so the first attack cannot bypass the end-turn instruction by winning.
	Room.mEnemyUnitPlacementDatas.SetNum(2);
	for (auto& Enemy : Room.mEnemyUnitPlacementDatas)
	{
		Enemy.mSpawnData = TSoftObjectPtr<UStaticEnemyUnitSpawnData>(FSoftObjectPath(
			TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_EagleUnit.DA_EagleUnit")));
		Enemy.mDifficulty = 1;
		Enemy.mDefaultSpeedPoint = 0;
	}
	Room.mEnemyUnitPlacementDatas[0].mTransform = FTileTransform(Enemy(), ETileActorDirection::Left);
	Room.mEnemyUnitPlacementDatas[1].mTransform = FTileTransform({6, 6}, ETileActorDirection::Left);
	Room.mObstaclePlacementDatas.RemoveAll([](const FObstaclePlacementData& Obstacle)
	{
		const auto Tile = Obstacle.mTransform.mIndex;
		return Tile == Start() || Tile == Move() || Tile == Enemy() || Tile == FTileIndex(6, 6)
			|| Tile == FTileIndex(3, 2) || Tile == FTileIndex(5, 2);
	});
	Room.mRoundStartEvents.Reset();
	Room.mRoundEndEvents.Reset();
}
