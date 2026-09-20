#include "TAS/Calculation/TacticalEffectExecutionCalculation.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

FTacticalEffectCustomExecutionParameters::FTacticalEffectCustomExecutionParameters() :
	mOwningSpec(nullptr),
	mTargetAttributeSetComponentModel(nullptr)
{
}

FTacticalEffectCustomExecutionParameters::FTacticalEffectCustomExecutionParameters(FTacticalEffectSpec& OwningSpec, UAttributeSetComponentModel* TargetAttributeSetComponentModel) :
	mOwningSpec(&OwningSpec),
	mTargetAttributeSetComponentModel(TargetAttributeSetComponentModel)
{
	check(OwningSpec.mEffectClass != nullptr);
}

const FTacticalEffectSpec& FTacticalEffectCustomExecutionParameters::GetOwningSpec() const
{
	check(mOwningSpec != nullptr);
	return *mOwningSpec;
}

FTacticalEffectSpec* FTacticalEffectCustomExecutionParameters::GetOwningSpecForPreExecuteMod() const
{
	check(mOwningSpec != nullptr);
	return mOwningSpec;
}

UAttributeSetComponentModel* FTacticalEffectCustomExecutionParameters::GetTargetAttributeSetComponentModel() const
{
	return mTargetAttributeSetComponentModel.Get();
}

UAttributeSetComponentModel* FTacticalEffectCustomExecutionParameters::GetSourceAttributeSetComponentModel() const
{
	check(mOwningSpec != nullptr);
	return mOwningSpec->GetContext()->GetAttributeSetComponentModel();
}

FTacticalEffectCustomExecutionOutput::FTacticalEffectCustomExecutionOutput() :
	mHandledStackCountManually(false)
{
}

void FTacticalEffectCustomExecutionOutput::MarkStackCountHandledManually()
{
	mHandledStackCountManually = true;
}

bool FTacticalEffectCustomExecutionOutput::IsStackCountHandledManually() const
{
	return mHandledStackCountManually;
}

void FTacticalEffectCustomExecutionOutput::MarkDynamicMagnitudeHandledManually()
{
	mHandledDynamicMagnitudeManually = true;
}

bool FTacticalEffectCustomExecutionOutput::IsDynamicMagnitudeHandledManually() const
{
	return mHandledDynamicMagnitudeManually;
}

void FTacticalEffectCustomExecutionOutput::AddOutputModifier(const FTacticalModifierEvaluatedData& OutputMod)
{
	mOutputModifiers.Add(OutputMod);
}

const TArray<FTacticalModifierEvaluatedData>& FTacticalEffectCustomExecutionOutput::GetOutputModifiers() const
{
	return mOutputModifiers;
}

void FTacticalEffectCustomExecutionOutput::GetOutputModifiers(OUT TArray<FTacticalModifierEvaluatedData>& OutputModifiers) const
{
	OutputModifiers.Append(mOutputModifiers);
}

void UTacticalEffectExecutionCalculation::Execute(const FTacticalEffectCustomExecutionParameters& ExecutionParams, OUT FTacticalEffectCustomExecutionOutput& OutExecutionOutput) const
{
}

