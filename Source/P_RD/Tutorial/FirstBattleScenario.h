#pragma once
#include "CoreMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
class UStaticTutorialRoomSpawnData;
class UStaticUnitSkillData;
enum class EGuidedStage : uint8;
struct P_RD_API FFirstBattleScenario
{
    static UStaticTutorialRoomSpawnData* LoadTemplate();
    static int32 SelectSkill(const TArray<const UStaticUnitSkillData*>& Skills);
    static bool AllowsTile(const UStaticTutorialRoomSpawnData& Room, EGuidedStage Stage, const FTileIndex& Tile);
};
