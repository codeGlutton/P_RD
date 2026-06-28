/*****************************************************************//**
 * @file   TacticalEffect.h
 * @brief  속성값 및 태그를 변경하는 객체 정의 헤더
 * @author 모호재, 김준형
 * @date   2026-06-24
 *********************************************************************/

#pragma once
#include "AttributeSet/AttributeSetMinimal.h"
#include "UObject/Object.h"
#include "TAS/Effect/TacticalEffectType.h"
#include "TacticalEffect.generated.h"

class UTacticalEffect;
class UTacticalEffectContext;

struct FActiveTacticalEffectsContainer;

/**
 * @brief 속성값 수정 정보
 * @details 한 Effect가 적용된 결과로 특정 Attribute가 실제로 얼마나 바뀌었는지를 집계한 런타임 기록.
 *          (모디파이어 연산을 모두 반영한 최종 누계값을 보관한다.)
 */
USTRUCT(BlueprintType)
struct P_RD_API FTacticalEffectModifiedAttribute
{
	GENERATED_BODY()

public:
	// @brief 수정 대상 속성(자체 ETAS 속성 식별자)
	UPROPERTY(Category = "Attribute", VisibleAnywhere, meta = (DisplayName = "Attribute"))
	FTacticalAttribute mAttribute;

	// @brief 해당 속성에 누적 적용된 최종 변경량
	UPROPERTY(Category = "Attribute", VisibleAnywhere, meta = (DisplayName = "TotalMagnitude"))
	float mTotalMagnitude = 0.f;
};

/**
 * @brief 태그 수정 정보
 * @details 한 Effect가 적용된 결과로 특정 GameplayTag가 몇 개나 부여/회수되었는지를 집계한 런타임 기록.
 */
USTRUCT(BlueprintType)
struct P_RD_API FTacticalEffectModifiedTag
{
	GENERATED_BODY()

public:
	// @brief 수정 대상 태그
	UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "Tag"))
	FGameplayTag mTag;

	// @brief 해당 태그의 누적 개수 변화량(부여 +, 회수 -)
	UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "TotalMagnitude"))
	int32 mTotalMagnitude = 0;
};

/**
 * @brief  어떤 Effect를 어떤 방식으로 누가 적용시키는지에 대한 런타임 정보 객체
 * @details 정적 정의(UTacticalEffect)와 적용 맥락(UTacticalEffectContext)을 묶은 "적용 인스턴스".
 *          정적 클래스의 모디파이어 정의를 바탕으로 실제 적용될 변경값(mModifierValues)을 계산해 보관한다.
 *          (구 GAS의 FGameplayEffectSpec에 대응되는 자체 구현)
 */
USTRUCT(BlueprintType)
struct P_RD_API FTacticalEffectSpec
{
	GENERATED_BODY()

public:
	FTacticalEffectSpec() = default;
	/**
	 * @brief 정적 Effect와 컨텍스트로부터 Spec을 생성한다.
	 * @param Class 원본 정적 Effect 정의
	 * @param EffectContext 적용 주체/대상 등 메타 데이터
	 */
	FTacticalEffectSpec(const UTacticalEffect* Class, UTacticalEffectContext* EffectContext);
	/**
	 * @brief 기존 Spec을 복사하되 컨텍스트만 교체해 생성한다.
	 * @param Other 복사 원본 Spec
	 * @param EffectContext 새로 지정할 컨텍스트
	 */
	FTacticalEffectSpec(const FTacticalEffectSpec& Other, UTacticalEffectContext* EffectContext);

	FTacticalEffectSpec(const FTacticalEffectSpec& Other) = default;
	FTacticalEffectSpec(FTacticalEffectSpec&& Other) = default;

public:
	FTacticalEffectSpec& operator=(FTacticalEffectSpec&& Other) = default;
	FTacticalEffectSpec& operator=(const FTacticalEffectSpec& Other) = default;

public:
	/**
	 * @brief Spec을 정적 Effect/컨텍스트로 초기화한다(모디파이어 값 사전 계산 포함).
	 * @param Class 원본 정적 Effect 정의
	 * @param EffectContext 적용 메타 데이터
	 */
	void Initialize(const UTacticalEffect* Class, UTacticalEffectContext* EffectContext);

	/**
	 * @brief 적용 스택 수를 설정한다.
	 * @param NewStackCount 새 스택 수
	 */
	void SetStackCount(int32 NewStackCount);
	/**
	 * @brief 적용 컨텍스트를 교체한다.
	 * @param NewEffectContext 새 컨텍스트
	 */
	void SetContext(UTacticalEffectContext* NewEffectContext);

	/** @brief 현재 적용 스택 수를 반환한다. @return 스택 수 */
	int32 GetStackCount() const;
	/** @brief 적용 컨텍스트를 반환한다. @return 컨텍스트 객체 */
	UTacticalEffectContext* GetContext() const;

public:
	/**
	 * @brief 정적 모디파이어 정의로부터 각 모디파이어의 변경값(mModifierValues)을 산출한다.
	 * @details 스택/연산 적용 전, 모디파이어별 기본 크기를 미리 계산해 캐싱하는 단계.
	 */
	void CalculateModifierMagnitudes();
	/**
	 * @brief 지정 인덱스 모디파이어의 사전 계산된 변경값을 반환한다.
	 * @param ModifierIdx 모디파이어 배열 인덱스
	 * @return 해당 모디파이어의 변경값(스택 미반영)
	 */
	float GetModifierMagnitude(int32 ModifierIdx) const;

public:
	// @brief 원본 정적 Effect 정보
	UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "EffectClass"))
	TObjectPtr<const UTacticalEffect> mEffectClass;

	// @brief 원본 Effect가 지정한 Attribute에 대한 변경 값 (스택은 미반영)
	UPROPERTY()
	TArray<float> mModifierValues;

	// @brief 적용 배율
	UPROPERTY()
	float mDynamicMagnitude = 1.f;

	// @brief 영구 Effect 여부
	UPROPERTY()
	bool mIsInfinite = false;

private:
	// @brief 해당 Effect가 적용되는 스택 수
	UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "StackCount"))
	int32 mStackCount = 0;

	// @brief 기본 메타 데이터
	UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "EffectContext"))
	TObjectPtr<UTacticalEffectContext> mEffectContext;
};

/**
 * @brief 단일 속성 수정자(모디파이어)의 정적 정의.
 * @details "어떤 속성을(mAttribute) 어떤 연산으로(mModifierOp) 얼마만큼(mModifierMagnitude)" 바꿀지를 기술한다.
 *          본 PR에서 연산 종류 타입이 GAS의 EGameplayModOp → 자체 ETacticalModOp로 치환되었다(GAS 폐기 작업의 일부).
 */
USTRUCT(BlueprintType)
struct P_RD_API FTacticalModifierInfo
{
	GENERATED_BODY()

public:
	/** @brief 모든 필드가 동일한지 비교한다. @param Other 비교 대상 @return 동일하면 true */
	bool operator==(const FTacticalModifierInfo& Other) const;
	/** @brief 동등하지 않은지 비교한다. @param Other 비교 대상 @return 다르면 true */
	bool operator!=(const FTacticalModifierInfo& Other) const;

public:
	// @brief 수정 대상 속성
	UPROPERTY(Category = "Attribute", EditDefaultsOnly, meta = (DisplayName = "Attribute"))
	FTacticalAttribute mAttribute;

	// @brief 적용 연산 종류(구 EGameplayModOp 대체). Aggregator가 이 op 값을 mMods[op] 배열 인덱스로 사용하므로
	//        ETacticalModOp의 정수값은 구 enum과 동일하게 유지된다(직렬화/리다이렉트 호환).
	//        기본값 Additive(=0, AddBase의 하위호환 별칭)는 단순 합산 연산을 의미한다.
	UPROPERTY(Category = "Attribute", EditDefaultsOnly, meta = (DisplayName = "ModifierOp"))
	TEnumAsByte<ETacticalModOp::Type> mModifierOp = ETacticalModOp::Additive;

	// @brief 연산에 사용할 크기 값(예: Additive면 더할 양, MultiplyAdditive면 더할 배율 등)
	UPROPERTY(Category = "Attribute", EditDefaultsOnly, meta = (DisplayName = "ModifierMagnitude"))
	float mModifierMagnitude;
};

/**
 * @brief  속성값 및 태그를 변경하는 객체
 * @details GAS의 UGameplayEffect를 대체하는 자체 Effect 정의(정적 데이터 자산).
 *          모디파이어 목록/지속 정책/스태킹 규칙/부여 태그를 기술하며, 적용 시 FTacticalEffectSpec으로 인스턴스화된다.
 */
UCLASS(Blueprintable, BlueprintType)
class P_RD_API UTacticalEffect : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 현재 활성 Effect 상태에서 이 Spec을 적용할 수 있는지 판정한다(스태킹/조건 검사).
	 * @param ActiveGEContainer 대상의 활성 Effect 컨테이너
	 * @param GESpec 적용하려는 Effect Spec
	 * @return 적용 가능하면 true
	 */
	bool CanApply(const FActiveTacticalEffectsContainer& ActiveGEContainer, const FTacticalEffectSpec& GESpec) const;

	/**
	 * @brief Effect가 활성 컨테이너에 추가될 때 호출되는 훅(Infinite/지속형 진입 처리).
	 * @param ActiveGEContainer 활성 Effect 컨테이너
	 * @param ActiveGE 새로 추가된 활성 Effect
	 * @return 추가가 유효하게 처리되면 true
	 */
	bool OnAddedToActiveContainer(FActiveTacticalEffectsContainer& ActiveGEContainer, FActiveTacticalEffect& ActiveGE) const;
	/**
	 * @brief Effect의 모디파이어 연산을 대상 속성에 실제로 실행(적용)한다.
	 * @param ActiveGEContainer 활성 Effect 컨테이너
	 * @param GESpec 실행할 Effect Spec
	 */
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveGEContainer, FTacticalEffectSpec& GESpec) const;
	/**
	 * @brief Effect 적용 진입점(지속 정책에 따라 즉시 실행 또는 활성 컨테이너 등록으로 분기).
	 * @param ActiveGEContainer 활성 Effect 컨테이너
	 * @param GESpec 적용할 Effect Spec
	 */
	void OnApplied(FActiveTacticalEffectsContainer& ActiveGEContainer, FTacticalEffectSpec& GESpec) const;

public:
	/** @brief 이 Effect를 식별하기 위한 에셋 태그 캐시를 반환한다. @return 에셋 태그 컨테이너 */
	const FGameplayTagContainer& GetAssetTags() const
	{
		return mCachedAssetTags;
	}

	/** @brief 이 Effect가 대상에게 부여하는 태그 캐시를 반환한다. @return 부여 태그 컨테이너 */
	const FGameplayTagContainer& GetGrantedTags() const
	{
		return mCachedGrantedTags;
	}

public:
	// @brief 이 Effect가 적용할 속성 수정자 목록(각 항목이 속성/연산/크기를 정의)
	UPROPERTY(Category = "Effect", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Modifiers"))
	TArray<FTacticalModifierInfo> mModifiers;

public:
	// @brief 즉시 적용(Instant)인지 기간 단위 적용(Infinite)인지 여부
	UPROPERTY(Category = "Duration", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DurationPolicy"))
	ETacticalEffectDurationType mDurationPolicy;

	// @brief 스태킹 정책(없음/소스별/타겟별)
	UPROPERTY(Category = "Stacking", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "StackingType"))
	ETacticalEffectStackingType mStackingType;

	// @brief 스택 수를 모디파이어 크기에 배수로 반영할지 여부(true면 ComputeStackedModifierMagnitude로 스택 보정)
	// NOTE: EditCondition 문자열의 "EGameplayEffectStackingType"은 구 GAS enum 이름의 잔재이나,
	//       DefaultEngine.ini의 CoreRedirect로 ETacticalEffectStackingType에 매핑되어 동작한다(에디터 전용 메타라 코드 동작 무관).
	UPROPERTY(Category = "Stacking", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "FactorInStackCount", EditConditionHides, EditCondition = "mStackingType != EGameplayEffectStackingType::None"))
	bool mFactorInStackCount;

public:
	// @brief 해당 Effect를 식별하기 위한 에셋 태그 캐시(GetAssetTags 반환원)
	FGameplayTagContainer mCachedAssetTags;
	// @brief 해당 Owner에게 부여하기 위한 태그 캐시(GetGrantedTags 반환원)
	FGameplayTagContainer mCachedGrantedTags;
};