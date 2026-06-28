/*****************************************************************//**
 * @file   TacticalEffectType.h
 * @brief  Effect 연관 타입들 정의 헤더
 * @author 모호재
 * @date   2026-06-25
 *********************************************************************/

#pragma once
#include "AttributeSet/AttributeSetMinimal.h"
#include "TacticalEffectType.generated.h"

class UTacticalEffectContext;
struct FActiveTacticalEffect;

UENUM()
enum class ETacticalEffectStackingType : uint8
{
	None UMETA(DisplayName = "No Stacking"),

	AggregateBySource UMETA(DisplayName = "Stack Per Source"),
	AggregateByTarget UMETA(DisplayName = "Stack Per Target"),
};

UENUM()
enum class ETacticalEffectDurationType : uint8
{
	Instant,
	Infinite,
};

/**
 * @brief 속성 수정자 연산 종류(구 GAS EGameplayModOp 대체).
 * @details 적용식: ((BaseValue + AddBase) * MultiplyAdditive / DivideAdditive * MultiplyCompound) + AddFinal
 *          정수 값은 구 EGameplayModOp와 동일하게 유지한다(직렬화 호환 + Aggregator 배열 인덱싱/하위호환 별칭 보존).
 */
UENUM(BlueprintType)
namespace ETacticalModOp
{
	enum Type : int
	{
		AddBase				UMETA(DisplayName = "Add (Base)"),
		MultiplyAdditive	UMETA(DisplayName = "Multiply (Additive)"),
		DivideAdditive		UMETA(DisplayName = "Divide (Additive)"),

		MultiplyCompound = 4 UMETA(DisplayName = "Multiply (Compound)"),

		AddFinal			UMETA(DisplayName = "Add (Final)"),
		Max					UMETA(Hidden, DisplayName = "Invalid"),

		// 하위호환 별칭(구 EGameplayModOp와 동일 값)
		Additive = 0		UMETA(Hidden),
		Multiplicitive = 1	UMETA(Hidden),
		Division = 2		UMETA(Hidden),
		Override = 3		UMETA(DisplayName = "Override"),
	};
}

/** @brief 구 GAS GameplayEffectUtilities 대체. 모디파이어 연산의 항등값/스택 계산. */
namespace TacticalEffectUtilities
{
	/** @brief 연산별 항등값(Additive→0, Multiply/Divide→1, 나머지→0). */
	P_RD_API float GetModifierBiasByModifierOp(ETacticalModOp::Type ModOp);

	/** @brief 스택 수에 따른 모디파이어 크기(항등값 기준 보정, MultiplyCompound는 거듭제곱). */
	P_RD_API float ComputeStackedModifierMagnitude(float BaseComputedMagnitude, int32 StackCount, ETacticalModOp::Type ModOp);

	/** @brief 디버그용 연산 이름 문자열. */
	P_RD_API FString TacticalModOpToString(int32 Type);
}

USTRUCT(BlueprintType)
struct P_RD_API FTacticalEffectRemovalInfo
{
	GENERATED_BODY()

	UPROPERTY(Category = "Removal", VisibleAnywhere, meta = (DisplayName = "StackCount"))
	int32 mStackCount = 0;

	UPROPERTY(Category = "Removal", VisibleAnywhere, meta = (DisplayName = "EffectContext"))
	TObjectPtr<UTacticalEffectContext> mEffectContext;

	const FActiveTacticalEffect* mActiveEffect = nullptr;
};
