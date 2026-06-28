/*****************************************************************//**
 * @file   TacticalEffect_AttackPoint.cpp
 * @brief  공격력(AttackPoint) 가산 이펙트 구현
 * @author 이문환
 * @date   2026-06-26
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_AttackPoint.h"

#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_AttackPoint::UTacticalEffect_AttackPoint()
{
	// 지속: 영구(핸들로 적용/해제) · 스택: 없음
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::None;

	// 대상 속성/연산만 정의 (크기 1, 실제 배율은 적용 측이 mDynamicMagnitude로 주입)
	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetAttackPointAttribute();
	Info.mModifierOp = ETacticalModOp::Additive;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}
