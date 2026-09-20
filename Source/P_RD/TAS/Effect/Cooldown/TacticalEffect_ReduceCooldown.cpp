#include "TAS/Effect/Cooldown/TacticalEffect_ReduceCooldown.h"
#include "TAS/Effect/Cooldown/TacticalEffect_Cooldown.h"
#include "TAS/Effect/TacticalEffectQuery.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

void UTacticalEffectExecutionCalculation_ReduceCooldown::Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute(ExecutionParams, OutExecutionOutput);

	UAttributeSetComponentModel* TargetAttributeSetCompModel = ExecutionParams.GetTargetAttributeSetComponentModel();
	checkf(TargetAttributeSetCompModel != nullptr, TEXT("타겟 컴포넌트 모델 nullptr"));

	const FTacticalEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const int32 ReductionAmount = FMath::FloorToInt(Spec.mDynamicMagnitude);

	/* 쿨다운 감소 대상 탐색 쿼리 구성 */
	
	FTacticalEffectQuery Query;
	Query.mCustomMatchDelegate.BindLambda([](const FActiveTacticalEffect& Effect) -> bool
	{
		const UTacticalEffect* EffectDef = Effect.mSpec.mEffectClass.Get();
		if (EffectDef == nullptr)
		{
			return false;
		}

		return EffectDef->IsA(UTacticalEffect_Cooldown::StaticClass()) == true;
	});

	const TArray<FActiveTacticalEffectHandle> CooldownHandles = TargetAttributeSetCompModel->GetActiveEffects(Query);

	/* 쿨다운 이펙트들의 잔여 시간 감소 */

	for (const FActiveTacticalEffectHandle& Handle : CooldownHandles)
	{
		const int32 CurrentTimeRemaining = TargetAttributeSetCompModel->GetActiveEffectTimeRemaining(Handle);
		if (CurrentTimeRemaining <= 0)
		{
			continue;
		}

		const int32 NewTimeRemaining = FMath::Max(0, CurrentTimeRemaining - ReductionAmount);
		TargetAttributeSetCompModel->SetActiveEffectTimeRemaining(NewTimeRemaining, Handle);
	}

	OutExecutionOutput.MarkDynamicMagnitudeHandledManually();
}

UTacticalEffect_ReduceCooldown::UTacticalEffect_ReduceCooldown()
{
	// 즉시 적용 후 소멸하는 즉발 이펙트 설정
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	// 쿨다운 감소 계산기 등록
	FTacticalEffectExecutionDefinition Definition;
	Definition.mCalculationClass = UTacticalEffectExecutionCalculation_ReduceCooldown::StaticClass();
	mExecutions.Add(Definition);
}

bool UTacticalEffect_ReduceCooldown::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	if (Super::CanApply(ActiveTEContainer, TESpec) == false)
	{
		return false;
	}

	// 감소시킬 크기가 0 이하이면 적용하지 않음
	if (TESpec.mDynamicMagnitude <= 0.f)
	{
		return false;
	}

	return true;
}
