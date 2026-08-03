/*****************************************************************//**
 * @file   TacticalEffect_SpeedPoint.cpp
 * @brief  SpeedPoint 이펙트 구현
 * @author 모호재
 * @date   2026-08-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_SpeedPoint.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "Setting/GameBalanceSettings.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

UTacticalEffect_SpeedPoint::UTacticalEffect_SpeedPoint()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetSpeedPointAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

void UTacticalEffect_SpeedPoint::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	FSRPGAttributeEffectEventLog Log;
	Log.mEffectAttribute = UUnitAttributeSet::GetSpeedPointAttribute();
	Log.mMagnitude = TESpec.GetModifiedAttribute(UUnitAttributeSet::GetSpeedPointAttribute())->mTotalMagnitude;

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	const UActorModel* Target = AttributeSetCompModelInstance->GetOwnerModel();

	GetWorldEventLogger(Target)->LogAttributeEffect(Target->GetModelId(), Target->GetClass(), Log);
}

void UTacticalEffectExecutionCalculation_GetSpeedPoint::Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute(ExecutionParams, OutExecutionOutput);

	const UGameBalanceSettings* GameBalanceSettings = GetDefault<UGameBalanceSettings>();
	checkf(GameBalanceSettings != nullptr, TEXT("게임 밸런스 세팅 nullptr"));

	UBoardCombatTargetSnapshotData* SourceSnapshotData = ExecutionParams.GetOwningSpec().GetInstigatorSnapshotData();
	checkf(SourceSnapshotData != nullptr, TEXT("소스 스냅샷 nullptr"));

	const float TotalSpeed = 
		ExecutionParams.GetOwningSpec().GetStackCount() * 
		ExecutionParams.GetOwningSpec().mDynamicMagnitude * 
		SourceSnapshotData->mAttributes[UUnitAttributeSet::GetSpeedPointFactorAttribute()];
	const float SpeedDiff = FMath::Floor(TotalSpeed);
	
	if (SpeedDiff > 0.f)
	{
		/* 속도 포인트 증가 적용 */
		OutExecutionOutput.AddOutputModifier(FTacticalModifierEvaluatedData(UUnitAttributeSet::GetSpeedPointAttribute(), ETacticalModOp::AddBase, SpeedDiff));
	
		/* 로그 적용 */
		FSRPGAttributeEffectEventLog Log;
		Log.mEffectAttribute = UUnitAttributeSet::GetSpeedPointAttribute();
		Log.mMagnitude = SpeedDiff;

		UAttributeSetComponentModel* TargetAttributeSetCompModel = ExecutionParams.GetTargetAttributeSetComponentModel();
		const UActorModel* Target = TargetAttributeSetCompModel->GetOwnerModel();
		GetWorldEventLogger(Target)->LogAttributeEffect(Target->GetModelId(), Target->GetClass(), Log);
	}

	OutExecutionOutput.MarkDynamicMagnitudeHandledManually();
	OutExecutionOutput.MarkStackCountHandledManually();
}

UTacticalEffect_GetSpeedPoint::UTacticalEffect_GetSpeedPoint()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalEffectExecutionDefinition Definition;
	Definition.mCalculationClass = UTacticalEffectExecutionCalculation_GetSpeedPoint::StaticClass();
	mExecutions.Add(Definition);
}

bool UTacticalEffect_GetSpeedPoint::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	if (Super::CanApply(ActiveTEContainer, TESpec) == false)
	{
		return false;
	}

	const UBoardCombatTargetSnapshotData* SourceSnapshotData = TESpec.GetInstigatorSnapshotData();
	const UBoardCombatTargetSnapshotData* TargetSnapshotData = TESpec.GetTargetSnapshotData();
	if (SourceSnapshotData == nullptr || TargetSnapshotData == nullptr)
	{
		return false;
	}

	if (SourceSnapshotData->mAttributes[UUnitAttributeSet::GetSpeedPointFactorAttribute()] * TESpec.mDynamicMagnitude <= 0)
	{
		return false;
	}

	return true;
}

