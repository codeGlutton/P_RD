/*****************************************************************//**
 * @file   TacticalEffect_HP.cpp
 * @brief  체력(HP) 가감 이펙트 구현
 * @author 이문환
 * @date   2026-06-30
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_HP.h"
#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"

#include "Setting/GameBalanceSettings.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

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
	Info.mAttribute = UCombatTargetAttributeSet::GetHPAttribute();
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
	Log.mEffectAttribute = UCombatTargetAttributeSet::GetHPAttribute();
	Log.mMagnitude = TESpec.GetModifiedAttribute(UCombatTargetAttributeSet::GetHPAttribute())->mTotalMagnitude;

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	const UActorModel* Instigator = AttributeSetCompModelInstance->GetOwnerModel();

	GetWorldEventLogger(Instigator)->LogAttributeEffect(Instigator->GetModelId(), Instigator->GetClass(), Log);
}

void UTacticalEffectExecutionCalculation_Attack::Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute(ExecutionParams, OutExecutionOutput);

	const UGameBalanceSettings* GameBalanceSettings = GetDefault<UGameBalanceSettings>();
	checkf(GameBalanceSettings != nullptr, TEXT("게임 밸런스 세팅 nullptr"));

	UBoardCombatTargetSnapshotData* SourceSnapshotData = ExecutionParams.GetOwningSpec().GetInstigatorSnapshotData();
	checkf(SourceSnapshotData != nullptr, TEXT("소스 스냅샷 nullptr"));

	UBoardCombatTargetSnapshotData* TargetSnapshotData = ExecutionParams.GetOwningSpec().GetTargetSnapshotData();
	checkf(TargetSnapshotData != nullptr, TEXT("타겟 스냅샷 nullptr"));

	// 약화
	const bool IsOwnerWeakness = SourceSnapshotData->mTags.Contains(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness);
	const float OwnerWeaknessRatio = IsOwnerWeakness == true ? GameBalanceSettings->mGlobalStatusEffectSetting.mEffectRatios[EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness] : 1.f;

	// 취약
	const bool IsTargetVulnerability = TargetSnapshotData->mTags.Contains(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Vulnerability);
	const float TargetVulnerabilityRatio = IsTargetVulnerability == true ? GameBalanceSettings->mGlobalStatusEffectSetting.mEffectRatios[EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Vulnerability] : 1.f;

	// 최종 공격력과 방어력
	const int32 TotalAttack =
		FMath::Floor(
			ExecutionParams.GetOwningSpec().GetStackCount() *
			ExecutionParams.GetOwningSpec().mDynamicMagnitude *
			TargetVulnerabilityRatio *
			OwnerWeaknessRatio *
			SourceSnapshotData->mAttributes[UCombatTargetAttributeSet::GetAttackFactorAttribute()]
		);
	const int32 TotalDefense =
		FMath::Floor(
			TargetSnapshotData->mAttributes[UCombatTargetAttributeSet::GetDefenseAttribute()]
		);

	/* 방어력 까기 */
	{
		const float DefenseDiff = -FMath::Min(TotalAttack, TotalDefense);
		if (DefenseDiff < 0)
		{
			OutExecutionOutput.AddOutputModifier(FTacticalModifierEvaluatedData(UCombatTargetAttributeSet::GetDefenseAttribute(), ETacticalModOp::AddBase, DefenseDiff));

			/* 로그 적용 */
			FSRPGAttributeEffectEventLog Log;
			Log.mEffectAttribute = UCombatTargetAttributeSet::GetDefenseAttribute();
			Log.mMagnitude = DefenseDiff;

			UAttributeSetComponentModel* TargetAttributeSetCompModel = ExecutionParams.GetTargetAttributeSetComponentModel();
			const UActorModel* Target = TargetAttributeSetCompModel->GetOwnerModel();
			GetWorldEventLogger(Target)->LogAttributeEffect(Target->GetModelId(), Target->GetClass(), Log);
		}
	}
	/* 체력 까기 */
	{
		const float HPDiff = TotalDefense - TotalAttack;
		if (HPDiff < 0)
		{
			OutExecutionOutput.AddOutputModifier(FTacticalModifierEvaluatedData(UCombatTargetAttributeSet::GetHPAttribute(), ETacticalModOp::AddBase, HPDiff));

			/* 로그 적용 */
			FSRPGAttributeEffectEventLog Log;
			Log.mEffectAttribute = UCombatTargetAttributeSet::GetHPAttribute();
			Log.mMagnitude = HPDiff;

			UAttributeSetComponentModel* TargetAttributeSetCompModel = ExecutionParams.GetTargetAttributeSetComponentModel();
			const UActorModel* Target = TargetAttributeSetCompModel->GetOwnerModel();
			GetWorldEventLogger(Target)->LogAttributeEffect(Target->GetModelId(), Target->GetClass(), Log);
		}
	}

	OutExecutionOutput.MarkDynamicMagnitudeHandledManually();
	OutExecutionOutput.MarkStackCountHandledManually();
}

UTacticalEffect_Attack::UTacticalEffect_Attack()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalEffectExecutionDefinition Definition;
	Definition.mCalculationClass = UTacticalEffectExecutionCalculation_Attack::StaticClass();
	mExecutions.Add(Definition);
}
