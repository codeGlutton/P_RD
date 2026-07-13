/*****************************************************************//**
 * @file   TacticalEffect_Movement.cpp
 * @brief  Movement 이펙트 구현
 * @author 이문환, 모호재
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_Movement.h"
#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"

#include "Setting/GameBalanceSettings.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

UTacticalEffect_Movement::UTacticalEffect_Movement()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetMovementAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

void UTacticalEffect_Movement::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	FSRPGAttributeEffectEventLog Log;
	Log.mEffectAttribute = UCombatTargetAttributeSet::GetMovementAttribute();
	Log.mMagnitude = TESpec.mModifierValues[0];

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	const UActorModel* Target = AttributeSetCompModelInstance->GetOwnerModel();

	GetWorldEventLogger(Target)->LogAttributeEffect(Target->GetModelId(), Target->GetClass(), Log);
}

void UTacticalEffectExecutionCalculation_GetMovement::Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute(ExecutionParams, OutExecutionOutput);

	const UGameBalanceSettings* GameBalanceSettings = GetDefault<UGameBalanceSettings>();
	checkf(GameBalanceSettings != nullptr, TEXT("게임 밸런스 세팅 nullptr"));

	UBoardCombatTargetSnapshotData* SourceSnapshotData = ExecutionParams.GetOwningSpec().GetInstigatorSnapshotData();
	checkf(SourceSnapshotData != nullptr, TEXT("소스 스냅샷 nullptr"));

	UBoardCombatTargetSnapshotData* TargetSnapshotData = ExecutionParams.GetOwningSpec().GetTargetSnapshotData();
	checkf(TargetSnapshotData != nullptr, TEXT("타겟 스냅샷 nullptr"));

	// 민첩성
	const bool IsTargetAgility = TargetSnapshotData->mTags.Contains(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Agility);
	const float TargetAgilityRatio = IsTargetAgility == true ? GameBalanceSettings->mGlobalStatusEffectSetting.mEffectRatios[EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Agility] : 1.f;

	const float TotalMove = 
		ExecutionParams.GetOwningSpec().GetStackCount() * 
		ExecutionParams.GetOwningSpec().mDynamicMagnitude * 
		TargetAgilityRatio *
		SourceSnapshotData->mAttributes[UCombatTargetAttributeSet::GetMovementFactorAttribute()];
	const float MoveDiff = FMath::Floor(TotalMove);
	
	if (MoveDiff > 0.f)
	{
		/* 이동 증가 적용 */
		OutExecutionOutput.AddOutputModifier(FTacticalModifierEvaluatedData(UCombatTargetAttributeSet::GetMovementAttribute(), ETacticalModOp::AddBase, MoveDiff));
	
		/* 로그 적용 */
		FSRPGAttributeEffectEventLog Log;
		Log.mEffectAttribute = UCombatTargetAttributeSet::GetMovementAttribute();
		Log.mMagnitude = MoveDiff;

		UAttributeSetComponentModel* TargetAttributeSetCompModel = ExecutionParams.GetTargetAttributeSetComponentModel();
		const UActorModel* Target = TargetAttributeSetCompModel->GetOwnerModel();
		GetWorldEventLogger(Target)->LogAttributeEffect(Target->GetModelId(), Target->GetClass(), Log);
	}

	OutExecutionOutput.MarkDynamicMagnitudeHandledManually();
	OutExecutionOutput.MarkStackCountHandledManually();
}

UTacticalEffect_GetMovement::UTacticalEffect_GetMovement()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalEffectExecutionDefinition Definition;
	Definition.mCalculationClass = UTacticalEffectExecutionCalculation_GetMovement::StaticClass();
	mExecutions.Add(Definition);
}

