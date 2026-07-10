/*****************************************************************//**
 * @file   TacticalEffect_HP.cpp
 * @brief  체력(HP) 가감 이펙트 구현
 * @author 이문환
 * @date   2026-06-30
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_HP.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"

/**
 * @brief 체력(HP) 이펙트의 기본 생성자
 */
UTacticalEffect_HP::UTacticalEffect_HP()
{
	// 체력은 즉시형 (되돌리는 거 없음)
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	// 스택 없음
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	// 사용할 속성: HP
	Info.mAttribute = UUnitAttributeSet::GetHPAttribute();
	// 연산 종류: 기존값에 합산할 거니까 Additive
	Info.mModifierOp = ETacticalModOp::AddBase;
	// 크기: 1.f로 고정 (패시브가 mDynamicMagnitude 설정한 값이 실제 크기가 됨)
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

void UTacticalEffect_HP::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	FSRPGAttributeEffectEventLog Log;
	Log.mEffectAttribute = UUnitAttributeSet::GetHPAttribute();
	Log.mMagnitude = TESpec.mModifierValues[0];

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	const UActorModel* Instigator = AttributeSetCompModelInstance->GetOwnerModel();

	GetWorldEventLogger(Instigator)->LogAttributeEffect(Instigator->GetModelId(), Instigator->GetClass(), Log);
}