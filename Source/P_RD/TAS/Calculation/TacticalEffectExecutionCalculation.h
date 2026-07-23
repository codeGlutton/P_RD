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

UCLASS(BlueprintType, Blueprintable, Abstract)
class UTacticalEffectExecutionCalculation : public UObject
{
	GENERATED_BODY()

public:
	virtual void Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, OUT FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const;
};

