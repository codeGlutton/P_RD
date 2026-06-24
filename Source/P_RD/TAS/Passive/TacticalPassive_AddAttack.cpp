/*****************************************************************//**
 * @file   TacticalPassive_AddAttack.cpp
 * @brief  공격력 증가 패시브 구현
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#include "TAS/Passive/TacticalPassive_AddAttack.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "AttributeSet/UnitAttributeSet.h"

void UTacticalPassive_AddAttack::ActivatePassive(
	const FPassiveActivateContext& Ctx,
	FBoardCombatTargetSnapshotData& TargetDelta,
	TInstancedStruct<FTacticalPassiveState>& PassiveState)
{
	// 대상 공격력(DamagePoint)에 보너스 누적 (무상태, Ctx/PassiveState 미사용)
	TargetDelta.mAttributes.FindOrAdd(UUnitAttributeSet::GetDamagePointAttribute()) += mAttackBonus;
}
