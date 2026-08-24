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

	UBoardCombatTargetSnapshotData* TargetSnapshotData = ExecutionParams.GetOwningSpec().GetTargetSnapshotData();
	checkf(TargetSnapshotData != nullptr, TEXT("타겟 스냅샷 nullptr"));

	// 신속
	const bool IsTargetHaste = TargetSnapshotData->mEffectCounts.Contains(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Haste);
	const float TargetHasteRatio = IsTargetHaste == true ? GameBalanceSettings->mGlobalStatusEffectSetting.mEffectRatios[EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Haste] : 1.f;


	// 둔화
	const bool IsTargetSlow = TargetSnapshotData->mEffectCounts.Contains(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Slow);
	const float TargetSlowRatio = IsTargetSlow == true ? GameBalanceSettings->mGlobalStatusEffectSetting.mEffectRatios[EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Slow] : 1.f;


	const float TotalSpeed = 
		ExecutionParams.GetOwningSpec().GetStackCount() * 
		ExecutionParams.GetOwningSpec().mDynamicMagnitude * 
		TargetHasteRatio * 
		TargetSlowRatio * 
		SourceSnapshotData->mAttributes[UUnitAttributeSet::GetSpeedPointFactorAttribute()];
	float SpeedDiff = FMath::Floor(TotalSpeed);
	
	OnPreGetSpeedPoint(ExecutionParams, OutExecutionOutput, OUT SpeedDiff);
	if (SpeedDiff > 0.f)
	{
		/* 속도 포인트 증가 적용 */
		OutExecutionOutput.AddOutputModifier(FTacticalModifierEvaluatedData(UUnitAttributeSet::GetSpeedPointAttribute(), ETacticalModOp::AddBase, SpeedDiff));
		OnPostGetSpeedPoint(ExecutionParams, OutExecutionOutput, SpeedDiff);

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

void UTacticalEffectExecutionCalculation_GetSpeedPoint::OnPreGetSpeedPoint(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput, OUT float& SpeedPoint) const
{
}

void UTacticalEffectExecutionCalculation_GetSpeedPoint::OnPostGetSpeedPoint(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput, float SpeedPoint) const
{
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

void UTacticalEffectExecutionCalculation_RechargeSpeedPoint::OnPreGetSpeedPoint(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput, OUT float& SpeedPoint) const
{
	Super::OnPreGetSpeedPoint(ExecutionParams, OutExecutionOutput, OUT SpeedPoint);

	const UGameBalanceSettings* GameBalanceSettings = GetDefault<UGameBalanceSettings>();
	checkf(GameBalanceSettings != nullptr, TEXT("게임 밸런스 세팅 nullptr"));

	SpeedPoint = GameBalanceSettings->mRechargedSpeedPointLimits.Clamp(SpeedPoint);
}

void UTacticalEffectExecutionCalculation_RechargeSpeedPoint::OnPostGetSpeedPoint(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput, float SpeedPoint) const
{
	Super::OnPostGetSpeedPoint(ExecutionParams, OutExecutionOutput, SpeedPoint);

	OutExecutionOutput.AddOutputModifier(FTacticalModifierEvaluatedData(UUnitAttributeSet::GetLastRechargedSpeedPointAttribute(), ETacticalModOp::Override, SpeedPoint));
}

UTacticalEffect_RechargeSpeedPoint::UTacticalEffect_RechargeSpeedPoint()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalEffectExecutionDefinition Definition;
	Definition.mCalculationClass = UTacticalEffectExecutionCalculation_RechargeSpeedPoint::StaticClass();
	mExecutions.Add(Definition);
}
