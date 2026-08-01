/*****************************************************************//**
 * @file   TacticalEffectExecutionCalculation.h
 * @brief  Effect 계산기 정의 헤더
 * @author 모호재
 * @date   2026-07-13
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "TAS/Effect/TacticalEffectType.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Aggregator/TacticalAggregator.h"
#include "TacticalEffectExecutionCalculation.generated.h"

class UAttributeSetComponentModel;

/**
 * @brief 커스텀 계산기 로직을 구현할 때, Execution 내에서 참고할 수 있는 정보 파라미터 묶음
 */
USTRUCT(BlueprintType)
struct FTacticalEffectCustomExecutionParameters
{
	GENERATED_BODY()

public:
	FTacticalEffectCustomExecutionParameters();
	FTacticalEffectCustomExecutionParameters(FTacticalEffectSpec& OwningSpec, UAttributeSetComponentModel* TargetAttributeSetComponentModel);

public:
	const FTacticalEffectSpec& GetOwningSpec() const;
	FTacticalEffectSpec* GetOwningSpecForPreExecuteMod() const;

	UAttributeSetComponentModel* GetTargetAttributeSetComponentModel() const;
	UAttributeSetComponentModel* GetSourceAttributeSetComponentModel() const;

private:
	FTacticalEffectSpec* mOwningSpec = nullptr;
	TWeakObjectPtr<UAttributeSetComponentModel> mTargetAttributeSetComponentModel = nullptr;
};

/**
 * @brief 커스텀 계산기 로직을 구현할 때, Execution 내에서 처리한 결과를 보관하는 객체
 */
USTRUCT(BlueprintType)
struct FTacticalEffectCustomExecutionOutput
{
	GENERATED_BODY()

public:
	FTacticalEffectCustomExecutionOutput();

public:
	void MarkStackCountHandledManually();
	bool IsStackCountHandledManually() const;
	void MarkDynamicMagnitudeHandledManually();
	bool IsDynamicMagnitudeHandledManually() const;

public:
	void AddOutputModifier(const FTacticalModifierEvaluatedData& OutputMod);
	const TArray<FTacticalModifierEvaluatedData>& GetOutputModifiers() const;
	void GetOutputModifiers(OUT TArray<FTacticalModifierEvaluatedData>& OutputModifiers) const;
	TArray<FTacticalModifierEvaluatedData>& GetOutputModifiersRef() 
	{ 
		return mOutputModifiers;
	}

private:
	UPROPERTY()
	TArray<FTacticalModifierEvaluatedData> mOutputModifiers;
	
	UPROPERTY()
	uint32 mHandledStackCountManually : 1 = false;
	UPROPERTY()
	uint32 mHandledDynamicMagnitudeManually : 1 = false;
};

/**
 * @brief 커스텀 계산기 객체. Instant Effect로만 사용됨
 */
UCLASS(BlueprintType, Blueprintable, Abstract)
class UTacticalEffectExecutionCalculation : public UObject
{
	GENERATED_BODY()

public:
	virtual void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, OUT FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const;
};

