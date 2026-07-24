#include "TAS/Effect/TacticalEffectType.h"

float TacticalEffectUtilities::GetModifierBiasByModifierOp(ETacticalModOp::Type ModOp)
{
	check(ModOp >= 0 && ModOp < ETacticalModOp::Max);
	static const float ModifierOpBiases[ETacticalModOp::Max] = { 0.f, 1.f, 1.f, 0.f, 0.f, 0.f };

	return ModifierOpBiases[ModOp];
}

float TacticalEffectUtilities::ComputeStackedModifierMagnitude(float BaseComputedMagnitude, int32 StackCount, ETacticalModOp::Type ModOp)
{
	// 이 연산의 초기값 계산
	const float OperationBias = GetModifierBiasByModifierOp(ModOp);

	StackCount = FMath::Clamp<int32>(StackCount, 0, StackCount);
	float StackMag = BaseComputedMagnitude;

	/*
	 * Override(덮어쓰기)는 스택과 무관하게 항상 같은 값을 강제하므로 보정하지 않는다.
	 * 나머지는 "항등값을 빼서 순수 증분만 남긴 뒤" 스택을 적용하고 다시 항등값을 더한다.
	 */
	if (ModOp != ETacticalModOp::Override)
	{
		// 순수 증가 수치만 추출 (곱셈계열은 -1, 합산계열은 -0)
		StackMag -= OperationBias;
		if (ModOp == ETacticalModOp::MultiplyCompound)
		{
			// 복리로 스택 누적
			StackMag = FMath::Pow(StackMag, StackCount);
		}
		else
		{
			// 선형으로 스택 누적
			StackMag *= StackCount;
		}
		// 다시 초기값 더해 합산
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
