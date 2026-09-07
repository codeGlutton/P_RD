#pragma once
#include "CoreMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"

class UStaticCombatRoomSpawnData;
class UStaticUnitSkillData;
enum class EGuidedStage : uint8;

// Authored opening: party slot 0 steps forward once and attacks the marked eagle.
// Coordinates are board coordinates, independent of camera, viewport size and party shuffle.
struct P_RD_API FFirstBattleScenario
{
	static FTileIndex Start() { return {4, 2}; }
	static FTileIndex Move() { return {4, 3}; }
	static FTileIndex Enemy() { return {4, 4}; }
	static void ApplyLayout(UStaticCombatRoomSpawnData& Room);
	static int32 SelectSkill(const TArray<const UStaticUnitSkillData*>& Skills);
	static bool AllowsTile(EGuidedStage Stage, const FTileIndex& Tile);
};
