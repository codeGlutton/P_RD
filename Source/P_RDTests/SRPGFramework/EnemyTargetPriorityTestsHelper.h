#pragma once

#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h"
#include "Setting/GameTeamType.h"
#include "EnemyTargetPriorityTestsHelper.generated.h"

/** Match the real player's constructor so room duplicates retain team identity. */
UCLASS()
class UMockTargetPolicyPlayerUnitModel : public UMockPlayerUnitModel
{
	GENERATED_BODY()
public:
	UMockTargetPolicyPlayerUnitModel()
	{
		SetGenericTeamId(EGameTeamType::Adventurer);
	}
};
