#include "Tutorial/FirstBattleScenario.h"
#include "Tutorial/StaticTutorialRoomSpawnData.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "Tutorial/GuidedTutorial.h"

UStaticTutorialRoomSpawnData* FFirstBattleScenario::LoadTemplate()
{
	return LoadObject<UStaticTutorialRoomSpawnData>(nullptr,
		TEXT("/Game/BP/DataAsset/Room/Turtorial/DA_Tutorial_FirstBattle.DA_Tutorial_FirstBattle"));
}

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

bool FFirstBattleScenario::AllowsTile(const UStaticTutorialRoomSpawnData& Room, EGuidedStage Stage, const FTileIndex& Tile)
{
	if (Stage == EGuidedStage::MoveTile || Stage == EGuidedStage::ConfirmMove) return Tile == Room.MoveDestination;
	if (Stage == EGuidedStage::SkillTarget || Stage == EGuidedStage::ConfirmSkill) return Tile == Room.GetTargetTile();
	return false;
}
