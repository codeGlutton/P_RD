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
class UObjectModel;
struct FActiveTacticalEffect;

/**
 * @brief 태그 이벤트 호출 타입
 */
UENUM(BlueprintType)
namespace ETacticalTagEventType
{
	enum Type : int
	{
		NewOrRemoved,
		AnyCountChange
	};
}

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
 * @brief Effect 연관 헬퍼 함수를 묶는 namespace
 */
namespace TacticalEffectUtilities
{
	/**
	 * @brief 각 연산자에 대한 초기 누적 값을 반환한다.
	 * @param ModOp 연산자 종류
	 * @return 초기 누적값
	 */
	P_RD_API float GetModifierBiasByModifierOp(ETacticalModOp::Type ModOp);

	/**
	 * @brief 스택(중첩) 수를 반영한 연산자에 대한 Effect 최종 크기를 계산한다.
	 * @param BaseComputedMagnitude 스택 1개 기준으로 계산된 모디파이어 크기.
	 * @param StackCount 현재 스택(중첩) 개수.
	 * @param ModOp 연산 종류.
	 * @return 해당 연산자에 대한 Effect 결과 합
	 *
	 * @details
	 * 합산계열(AddBase/AddFinal)은 0 + a + b + ...으로 연산
	 * 곱셈계열(MultiplyAdditive/DivideAdditive/MultiplyCompound)은 1 * a * b * ...로 연산
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
 * @details 
 * 어떤 이펙트가 몇 스택으로 제거되었는지, 그리고 어떤 컨텍스트/활성 인스턴스에서 제거되었는지를 한데 모아 전달
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

/**
 * @brief 이벤트 호출 시 참조용 정보
 */
USTRUCT(BlueprintType)
struct FTacticalEventData
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Payload", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EventTag"))
	FGameplayTag mEventTag;

	UPROPERTY(Category = "Payload", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Instigator"))
	TObjectPtr<const UObjectModel> mInstigator;

	UPROPERTY(Category = "Payload", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EventMagnitude"))
	float mEventMagnitude = 0.f;
};
