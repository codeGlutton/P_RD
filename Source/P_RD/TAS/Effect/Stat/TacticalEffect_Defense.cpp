/*****************************************************************//**
 * @file   TacticalEffect_Defense.cpp
 * @brief  Defense 이펙트 구현
 * @author 이문환, 모호재
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_Defense.h"
#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"

#include "Setting/GameBalanceSettings.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

UTacticalEffect_Defense::UTacticalEffect_Defense()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetDefenseAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

void UTacticalEffect_Defense::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	FSRPGAttributeEffectEventLog Log;
	Log.mEffectAttribute = UCombatTargetAttributeSet::GetDefenseAttribute();
	Log.mMagnitude = TESpec.GetModifiedAttribute(UCombatTargetAttributeSet::GetDefenseAttribute())->mTotalMagnitude;

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	const UActorModel* Target = AttributeSetCompModelInstance->GetOwnerModel();

	GetWorldEventLogger(Target)->LogAttributeEffect(Target->GetModelId(), Target->GetClass(), Log);
}

void UTacticalEffectExecutionCalculation_GetDefense::Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute(ExecutionParams, OutExecutionOutput);

	const UGameBalanceSettings* GameBalanceSettings = GetDefault<UGameBalanceSettings>();
	checkf(GameBalanceSettings != nullptr, TEXT("게임 밸런스 세팅 nullptr"));

	UBoardCombatTargetSnapshotData* SourceSnapshotData = ExecutionParams.GetOwningSpec().GetInstigatorSnapshotData();
	checkf(SourceSnapshotData != nullptr, TEXT("소스 스냅샷 nullptr"));

	UBoardCombatTargetSnapshotData* TargetSnapshotData = ExecutionParams.GetOwningSpec().GetTargetSnapshotData();
	checkf(TargetSnapshotData != nullptr, TEXT("타겟 스냅샷 nullptr"));

	// 요새화
	const bool IsTargetFortification = TargetSnapshotData->mTags.Contains(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Fortification);
	const float TargetFortificationRatio = IsTargetFortification == true ? GameBalanceSettings->mGlobalStatusEffectSetting.mEffectRatios[EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Fortification] : 1.f;

	const float TotalDefense =
		ExecutionParams.GetOwningSpec().GetStackCount() *
		ExecutionParams.GetOwningSpec().mDynamicMagnitude *
		TargetFortificationRatio *
		SourceSnapshotData->mAttributes[UCombatTargetAttributeSet::GetDefenseFactorAttribute()];
	const float DefenseDiff = FMath::Floor(TotalDefense);

	if (DefenseDiff > 0.f)
	{
		/* 이동 증가 적용 */
		OutExecutionOutput.AddOutputModifier(FTacticalModifierEvaluatedData(UCombatTargetAttributeSet::GetDefenseAttribute(), ETacticalModOp::AddBase, DefenseDiff));

		/* 로그 적용 */
		FSRPGAttributeEffectEventLog Log;
		Log.mEffectAttribute = UCombatTargetAttributeSet::GetDefenseAttribute();
		Log.mMagnitude = DefenseDiff;

		UAttributeSetComponentModel* TargetAttributeSetCompModel = ExecutionParams.GetTargetAttributeSetComponentModel();
		const UActorModel* Target = TargetAttributeSetCompModel->GetOwnerModel();
		GetWorldEventLogger(Target)->LogAttributeEffect(Target->GetModelId(), Target->GetClass(), Log);
	}

	OutExecutionOutput.MarkDynamicMagnitudeHandledManually();
	OutExecutionOutput.MarkStackCountHandledManually();
}

UTacticalEffect_GetDefense::UTacticalEffect_GetDefense()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalEffectExecutionDefinition Definition;
	Definition.mCalculationClass = UTacticalEffectExecutionCalculation_GetDefense::StaticClass();
	mExecutions.Add(Definition);
}
