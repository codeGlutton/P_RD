/*****************************************************************//**
 * @file   TacticalPassive_AddAttackPoint.cpp
 * @brief  공격력 증가 패시브 구현
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#include "TAS/Passive/TacticalPassive_AddAttackPoint.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackPoint.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "GameplayTagType.h"

UTacticalPassive_AddAttackPoint::UTacticalPassive_AddAttackPoint()
{
	// 관여 시점: 턴 시작(발동)·턴 끝(커밋·해제). BP 서브클래스에서 교체 가능
	mTimingTags.AddTag(AbilityTags::GameplayAbility_Passive_OnStartTurn);
	mTimingTags.AddTag(AbilityTags::GameplayAbility_Passive_OnEndTurn);

	// 기본 적용 이펙트(공격력 가산). 패시브 BP 서브클래스에서 BP 이펙트로 교체 가능
	mEffectClass = UTacticalEffect_AttackPoint::StaticClass();
}

void UTacticalPassive_AddAttackPoint::EvaluatePassive(
	const FPassiveActivateContext& Ctx,
	FBoardCombatTargetSnapshotData& TargetDelta,
	TInstancedStruct<FTacticalPassiveState>& PassiveState)
{
	// 대상 공격력(AttackPoint)에 보너스 누적 (무상태, Ctx/PassiveState 미사용)
	TargetDelta.mAttributes.FindOrAdd(UUnitAttributeSet::GetAttackPointAttribute()) += mAttackBonus;
}
