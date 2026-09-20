#include "TAS/Effect/Cooldown/TacticalEffect_Cooldown.h"

UTacticalEffect_Cooldown::UTacticalEffect_Cooldown()
{
	mDurationPolicy = ETacticalEffectDurationType::Duration;
	mStackingType = ETacticalEffectStackingType::None;
	mDurationMagnitude = 1;
}

UTacticalEffect_RoundCooldown::UTacticalEffect_RoundCooldown()
{
	// 기간 설정
	mDurationUnitPolicy = ETacticalEffectDurationUnitType::EveryRound;
}

UTacticalEffect_TurnCooldown::UTacticalEffect_TurnCooldown()
{
	// 기간 설정
	mDurationUnitPolicy = ETacticalEffectDurationUnitType::EveryTurn;
}
