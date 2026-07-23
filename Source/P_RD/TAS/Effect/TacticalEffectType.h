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

/**
 * @brief 이펙트 스택(중첩) 집계 방식.
 */
UENUM()
enum class ETacticalEffectStackingType : uint8
{
	None								UMETA(DisplayName = "No Stacking", ToolTip = "스택이 누적되지 않음"),

	AggregateBySource					UMETA(DisplayName = "Stack Per Source", ToolTip = "소스 ASC마다 스택 누적"),
	AggregateByTarget					UMETA(DisplayName = "Stack Per Target", ToolTip = "타겟 ASC마다 스택 누적"),
};

/**
 * @brief 이펙트 지속 종류
 */
UENUM()
enum class ETacticalEffectDurationType : uint8
{
	Instant								UMETA(ToolTip = "즉발이후 소멸"),
	Duration							UMETA(ToolTip = "일단 단위 기간 지속 후 소멸"),
	Infinite							UMETA(ToolTip = "인위적으로 해제 명령 전까지 무한 지속"),
};

/**
 * @brief 이펙트 지속이 Duration으로 설정될 경우, 기간 단위
 */
UENUM()
enum class ETacticalEffectDurationUnitType : uint8
{
	EveryTurn							UMETA(ToolTip = "매 턴 단위"),
	EveryRound							UMETA(ToolTip = "매 라운드 단위"),
};

/**
 * @brief 스택킹될 경우, Effect의 Duration 변화 정책
 */
UENUM()
enum class ETacticalEffectStackingDurationPolicy : uint8
{
	RefreshOnSuccessfulApplication		UMETA(ToolTip = "새로운 Duration으로 덮어쓰기"),
	NeverRefresh						UMETA(ToolTip = "초기 Duration으로만 작동하고, 변화없음"),
	ExtendDuration						UMETA(ToolTip = "누적될때마다 Duration 더하기"),
};

/**
 * @brief 스택킹될 경우, Effect의 Duration 만기 정책
 */
UENUM()
enum class ETacticalEffectStackingExpirationPolicy : uint8
{
	ClearEntireStack					UMETA(ToolTip = "모든 스택 지우기"),
	RemoveSingleStackAndRefreshDuration	UMETA(ToolTip = "단일 스택 지우고 Duration 초기화"),
	RefreshDuration						UMETA(ToolTip = "스택 변경 없이 Duration만 초기화"),
};

/**
 * @brief 속성 수정자(Modifier) 연산 종류
 * @details
 * 
 * FinalValue = Override || (((BaseValue + AddBase) * MultiplyAdditive / DivideAdditive * MultiplyCompound) + AddFinal)
 * 같은 op의 Effect는 누적되고, 각 누적된 op는 위 공식으로 Attribute 값을 최종 계산해낸다.
 * 
 * AddBase(0)          : BaseValue에 직접 합산되는 기저 보정(합산계열, 항등값 0).
 * MultiplyAdditive(1) : 배율을 "가산"으로 누적(예: +0.1 두 개면 *1.2). 곱셈계열, 항등값 1.
 * DivideAdditive(2)   : 나눗셈 배율을 가산으로 누적. 곱셈계열, 항등값 1.
 * Override(3)         : 위 계산을 무시하고 값을 통째로 덮어쓴다(최우선 단일값).
 * MultiplyCompound(4) : 배율을 "곱셈(거듭제곱)"으로 누적(스택 시 base^StackCount). 곱셈계열, 항등값 1.
 * AddFinal(5)         : 모든 곱/나눗셈 이후 최종 단계에서 합산되는 보정. 합산계열, 항등값 0.
 * Max(6)              : 단순 열거형 최대 값 표기용. 사용하지 말 것.
 */
UENUM(BlueprintType)
namespace ETacticalModOp
{
	enum Type : int
	{
		AddBase				UMETA(DisplayName = "Add (Base)"),
		MultiplyAdditive	UMETA(DisplayName = "Multiply (Additive)"),
		DivideAdditive		UMETA(DisplayName = "Divide (Additive)"),
		Override			UMETA(DisplayName = "Override"),
		MultiplyCompound	UMETA(DisplayName = "Multiply (Compound)"),
		AddFinal			UMETA(DisplayName = "Add (Final)"),

		Max					UMETA(Hidden, DisplayName = "Invalid"),
	};
}

/**
 * @brief 구 GAS의 GameplayEffectUtilities를 대체하는 모디파이어 연산 보조 함수 모음.
 * @details GAS 폐기에 따라 엔진 유틸리티에 있던 "연산별 항등값/스택 크기 계산" 로직을
 *          ETacticalModOp 기준으로 재구현한다(구현은 TacticalEffectType.cpp).
 *          연산을 "합산계열(항등값 0)"과 "곱셈계열(항등값 1)"로 나눠 다루는 것이 핵심이다.
 */
namespace TacticalEffectUtilities
{
	/**
	 * @brief 주어진 연산의 항등값(identity/bias)을 반환한다.
	 * @details 항등값이란 "그 연산에 적용해도 결과를 바꾸지 않는 값"이다.
	 *          - 합산계열(AddBase/AddFinal): 0  (x + 0 = x)
	 *          - 곱셈계열(MultiplyAdditive/DivideAdditive/MultiplyCompound): 1  (x * 1 = x)
	 *          모디파이어가 없을 때의 기준값/스택 보정의 베이스로 사용된다.
	 * @param ModOp 항등값을 구할 연산 종류.
	 * @return 해당 연산의 항등값(합산계열 0, 곱셈계열 1).
	 */
	P_RD_API float GetModifierBiasByModifierOp(ETacticalModOp::Type ModOp);

	/**
	 * @brief 스택(중첩) 수를 반영한 모디파이어 최종 크기를 계산한다.
	 * @details 항등값을 기준으로 (계산값 - 항등값)을 StackCount만큼 반영한다.
	 *          - 합산/일반 배율계열: 항등값 + (BaseComputedMagnitude - 항등값) * StackCount 형태의 선형 누적.
	 *          - MultiplyCompound: 선형이 아니라 거듭제곱 — base^StackCount 로 복리처럼 누적된다.
	 * @param BaseComputedMagnitude 스택 1개 기준으로 계산된 모디파이어 크기.
	 * @param StackCount 현재 스택(중첩) 개수.
	 * @param ModOp 연산 종류(항등값/누적 방식 결정).
	 * @return 스택이 반영된 최종 모디파이어 크기.
	 */
	P_RD_API float ComputeStackedModifierMagnitude(float BaseComputedMagnitude, int32 StackCount, ETacticalModOp::Type ModOp);

	/**
	 * @brief 연산 종류를 디버그용 사람이 읽는 문자열로 변환한다.
	 * @param Type ETacticalModOp::Type의 정수 값(로그/디버거 호환을 위해 int32로 받음).
	 * @return "AddBase", "MultiplyCompound" 등 연산 이름 문자열.
	 */
	P_RD_API FString TacticalModOpToString(int32 Type);
}

/**
 * @brief 이펙트 제거 시 콜백/통지에 전달되는 제거 정보 묶음.
 * @details 어떤 이펙트가 몇 스택으로 제거되었는지, 그리고 어떤 컨텍스트/활성 인스턴스에서
 *          제거되었는지를 한데 모아 전달한다(구 GAS의 제거 콜백 페이로드 대체).
 */
USTRUCT(BlueprintType)
struct P_RD_API FTacticalEffectRemovalInfo
{
	GENERATED_BODY()

	/** @brief 제거 시점의 스택(중첩) 개수. */
	UPROPERTY(Category = "Removal", VisibleAnywhere, meta = (DisplayName = "StackCount"))
	int32 mStackCount = 0;

	/** @brief 제거된 이펙트의 적용 컨텍스트(가해자/대상 등 메타데이터). */
	UPROPERTY(Category = "Removal", VisibleAnywhere, meta = (DisplayName = "EffectContext"))
	TObjectPtr<UTacticalEffectContext> mEffectContext;

	/** @brief 제거된 활성 이펙트 인스턴스에 대한 비소유(non-owning) 포인터. */
	const FActiveTacticalEffect* mActiveEffect = nullptr;
};
