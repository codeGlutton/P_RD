/*****************************************************************//**
 * @file   TacticalEffectType.h
 * @brief  Effect 연관 타입들 정의 헤더
 * @author 모호재
 * @date   2026-06-25
 *********************************************************************/

#pragma once
#include "AttributeSet/AttributeSetMinimal.h"
#include "TacticalEffectType.generated.h"

// 전방 선언: 헤더 의존을 줄이기 위해 포인터/참조로만 사용되는 타입들을 미리 선언한다.
class UTacticalEffectContext;  // 이펙트 적용 컨텍스트(가해자/대상 등 메타데이터)
struct FActiveTacticalEffect;  // 현재 활성화된 이펙트 인스턴스(지속/스택 추적용)

/**
 * @brief 이펙트 스택(중첩) 집계 방식.
 * @details 동일 이펙트가 여러 번 적용될 때 무엇을 키로 삼아 스택을 합칠지 결정한다.
 *          - None: 스택하지 않음(매번 독립 인스턴스).
 *          - AggregateBySource: 가해자(Source)별로 스택을 합산.
 *          - AggregateByTarget: 대상(Target)별로 스택을 합산.
 */
UENUM()
enum class ETacticalEffectStackingType : uint8
{
	None UMETA(DisplayName = "No Stacking"),

	AggregateBySource UMETA(DisplayName = "Stack Per Source"),
	AggregateByTarget UMETA(DisplayName = "Stack Per Target"),
};

/**
 * @brief 이펙트 지속 종류.
 * @details - Instant: 즉발(BaseValue를 즉시 영구 변경, 적용 후 사라짐).
 *          - Infinite: 무한 지속(명시적으로 제거되기 전까지 모디파이어로 유지).
 */
UENUM()
enum class ETacticalEffectDurationType : uint8
{
	Instant,
	Infinite,
};

/**
 * @brief 속성 수정자(Modifier) 연산 종류 — 구 GAS의 EGameplayModOp를 대체하는 자체 enum.
 *
 * @details
 * [마이그레이션 배경]
 * 본 PR(#191)은 GAS 폐기 작업의 일부로, 이펙트 모디파이어의 "연산 종류"를 가리키던
 * 엔진 제공 enum EGameplayModOp를 프로젝트 자체 enum ETacticalModOp로 치환한다.
 * 이펙트 시스템 전반(이펙트 데이터/Aggregator/적용 로직)이 이 enum 값을 참조하므로,
 * 단순 이름 교체가 아니라 "값 호환성"을 반드시 유지해야 한다.
 *
 * [정수 값을 구 EGameplayModOp와 동일하게 유지하는 이유 — 3가지]
 *  (1) 직렬화 호환: 기존 .uasset 에셋과 세이브 파일에는 모디파이어 연산이 "정수 값"으로
 *      박혀 있다. 새 enum의 정수 값이 달라지면 기존 데이터의 의미가 어긋나므로 값을 보존한다.
 *  (2) Aggregator 배열 인덱싱: Aggregator는 mMods[ETacticalModOp::Max] 형태로 op 값을 그대로
 *      배열 인덱스로 사용한다. 따라서 값은 0..(Max-1) 범위의 연속/안정 인덱스여야 한다.
 *  (3) CoreRedirect 매핑: DefaultEngine.ini의 CoreRedirect가 구 enum "이름"을 새 enum 이름으로
 *      매핑한다. 값까지 같아야 리다이렉트 후에도 기존 데이터가 동일 연산으로 해석된다.
 *
 * [모디파이어 적용 수식 — op 순서대로 누적]
 *   FinalValue = ((BaseValue + AddBase) * MultiplyAdditive / DivideAdditive * MultiplyCompound) + AddFinal
 *   - AddBase(0)          : BaseValue에 직접 합산되는 기저 보정(합산계열, 항등값 0).
 *   - MultiplyAdditive(1) : 배율을 "가산"으로 누적(예: +0.1 두 개면 *1.2). 곱셈계열, 항등값 1.
 *   - DivideAdditive(2)   : 나눗셈 배율을 가산으로 누적. 곱셈계열, 항등값 1.
 *   - Override(3)         : 위 계산을 무시하고 값을 통째로 덮어쓴다(최우선 단일값).
 *   - MultiplyCompound(4) : 배율을 "곱셈(거듭제곱)"으로 누적(스택 시 base^StackCount). 곱셈계열, 항등값 1.
 *   - AddFinal(5)         : 모든 곱/나눗셈 이후 최종 단계에서 합산되는 보정. 합산계열, 항등값 0.
 *   - Max(6)              : 유효 연산 개수 = 무효(Invalid) 센티넬. 배열 크기/경계 검사용.
 *
 * @note 값 6(=Max)은 실제 연산이 아니라 "연산 개수/무효" 표식이다. mMods 배열 크기로 쓰이며,
 *       이 값으로 인덱싱하면 안 된다(out-of-range).
 */
UENUM(BlueprintType)
namespace ETacticalModOp
{
	enum Type : int
	{
		// 값 0: BaseValue에 직접 더하는 기저 합산. (구 EGameplayModOp::Additive 자리)
		AddBase				UMETA(DisplayName = "Add (Base)"),
		// 값 1: 배율을 가산식으로 누적하는 곱셈. (구 EGameplayModOp::Multiplicitive 자리)
		MultiplyAdditive	UMETA(DisplayName = "Multiply (Additive)"),
		// 값 2: 나눗셈 배율을 가산식으로 누적. (구 EGameplayModOp::Division 자리)
		DivideAdditive		UMETA(DisplayName = "Divide (Additive)"),

		// 값 3은 Override가 차지하므로(아래 별칭 참조) 여기서는 건너뛰고 값 4를 명시 고정한다.
		// 값 4: 배율을 곱셈(거듭제곱)으로 누적 — 스택 시 base^StackCount.
		MultiplyCompound = 4 UMETA(DisplayName = "Multiply (Compound)"),

		// 값 5: 모든 곱/나눗셈 이후 최종 단계의 합산 보정.
		AddFinal			UMETA(DisplayName = "Add (Final)"),
		// 값 6: 유효 연산 개수이자 무효 센티넬. mMods[ETacticalModOp::Max]의 배열 크기로 사용.
		Max					UMETA(Hidden, DisplayName = "Invalid"),

		// ── 하위호환 별칭(구 EGameplayModOp와 "동일 정수 값") ────────────────────────
		// 구 GAS 시절의 enum 이름을 그대로 컴파일/리다이렉트되게 하기 위한 별칭이다.
		// 같은 값을 가리키므로 신규 이름과 완전히 상호 교환 가능하다.
		Additive = 0		UMETA(Hidden),  // == AddBase
		Multiplicitive = 1	UMETA(Hidden),  // == MultiplyAdditive (구 GAS 철자 오타까지 보존)
		Division = 2		UMETA(Hidden),  // == DivideAdditive
		// 값 3: 덮어쓰기. 별칭이지만 신규 enum에 대응 항목이 없어 이 이름이 정식으로 노출된다.
		Override = 3		UMETA(DisplayName = "Override"),
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
