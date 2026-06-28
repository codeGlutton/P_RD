#include "TAS/Effect/TacticalEffectType.h"

float TacticalEffectUtilities::GetModifierBiasByModifierOp(ETacticalModOp::Type ModOp)
{
	// 인덱스: 0=AddBase/Additive, 1=MultiplyAdditive, 2=DivideAdditive, 3=Override, 4=MultiplyCompound, 5=AddFinal
	static const float ModifierOpBiases[ETacticalModOp::Max] = { 0.f, 1.f, 1.f, 0.f, 0.f, 0.f };
	check(ModOp >= 0 && ModOp < ETacticalModOp::Max);

	return ModifierOpBiases[ModOp];
}

float TacticalEffectUtilities::ComputeStackedModifierMagnitude(float BaseComputedMagnitude, int32 StackCount, ETacticalModOp::Type ModOp)
{
	const float OperationBias = GetModifierBiasByModifierOp(ModOp);

	StackCount = FMath::Clamp<int32>(StackCount, 0, StackCount);

	float StackMag = BaseComputedMagnitude;

	// Override는 스택과 무관. 나머지는 항등값을 빼고 스택을 곱한 뒤 다시 더한다.
	if (ModOp != ETacticalModOp::Override)
	{
		StackMag -= OperationBias;
		if (ModOp == ETacticalModOp::MultiplyCompound)
		{
			StackMag = FMath::Pow(StackMag, StackCount);
		}
		else
		{
			StackMag *= StackCount;
		}
		StackMag += OperationBias;
	}

	return StackMag;
}

FString TacticalEffectUtilities::TacticalModOpToString(int32 Type)
{
	switch (Type)
	{
	case ETacticalModOp::AddBase:			return TEXT("AddBase");
	case ETacticalModOp::MultiplyAdditive:	return TEXT("MultiplyAdditive");
	case ETacticalModOp::DivideAdditive:	return TEXT("DivideAdditive");
	case ETacticalModOp::Override:			return TEXT("Override");
	case ETacticalModOp::MultiplyCompound:	return TEXT("MultiplyCompound");
	case ETacticalModOp::AddFinal:			return TEXT("AddFinal");
	default:								return TEXT("Invalid");
	}
}
