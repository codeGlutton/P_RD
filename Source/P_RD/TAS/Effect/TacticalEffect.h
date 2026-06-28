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
 */
USTRUCT(BlueprintType)
struct P_RD_API FTacticalEffectModifiedAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Attribute", VisibleAnywhere, meta = (DisplayName = "Attribute"))
	FTacticalAttribute mAttribute;

	UPROPERTY(Category = "Attribute", VisibleAnywhere, meta = (DisplayName = "TotalMagnitude"))
	float mTotalMagnitude = 0.f;
};

/**
 * @brief 태그 수정 정보
 */
USTRUCT(BlueprintType)
struct P_RD_API FTacticalEffectModifiedTag
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "Tag"))
	FGameplayTag mTag;

	UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "TotalMagnitude"))
	int32 mTotalMagnitude = 0;
};

/**
 * @brief  어떤 Effect를 어떤 방식으로 누가 적용시키는지에 대한 런타임 정보 객체
 */
USTRUCT(BlueprintType)
struct P_RD_API FTacticalEffectSpec
{
	GENERATED_BODY()

public:
	FTacticalEffectSpec() = default;
	FTacticalEffectSpec(const UTacticalEffect* Class, UTacticalEffectContext* EffectContext);
	FTacticalEffectSpec(const FTacticalEffectSpec& Other, UTacticalEffectContext* EffectContext);

	FTacticalEffectSpec(const FTacticalEffectSpec& Other) = default;
	FTacticalEffectSpec(FTacticalEffectSpec&& Other) = default;

public:
	FTacticalEffectSpec& operator=(FTacticalEffectSpec&& Other) = default;
	FTacticalEffectSpec& operator=(const FTacticalEffectSpec& Other) = default;

public:
	void Initialize(const UTacticalEffect* Class, UTacticalEffectContext* EffectContext);

	void SetStackCount(int32 NewStackCount);
	void SetContext(UTacticalEffectContext* NewEffectContext);

	int32 GetStackCount() const;
	UTacticalEffectContext* GetContext() const;

public:
	void CalculateModifierMagnitudes();
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

USTRUCT(BlueprintType)
struct P_RD_API FTacticalModifierInfo
{
	GENERATED_BODY()

public:
	bool operator==(const FTacticalModifierInfo& Other) const;
	bool operator!=(const FTacticalModifierInfo& Other) const;

public:
	UPROPERTY(Category = "Attribute", EditDefaultsOnly, meta = (DisplayName = "Attribute"))
	FTacticalAttribute mAttribute;

	UPROPERTY(Category = "Attribute", EditDefaultsOnly, meta = (DisplayName = "ModifierOp"))
	TEnumAsByte<ETacticalModOp::Type> mModifierOp = ETacticalModOp::Additive;

	UPROPERTY(Category = "Attribute", EditDefaultsOnly, meta = (DisplayName = "ModifierMagnitude"))
	float mModifierMagnitude;
};

/**
 * @brief  속성값 및 태그를 변경하는 객체
 */
UCLASS(Blueprintable, BlueprintType)
class P_RD_API UTacticalEffect : public UObject
{
	GENERATED_BODY()

public:
	bool CanApply(const FActiveTacticalEffectsContainer& ActiveGEContainer, const FTacticalEffectSpec& GESpec) const;

	bool OnAddedToActiveContainer(FActiveTacticalEffectsContainer& ActiveGEContainer, FActiveTacticalEffect& ActiveGE) const;
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveGEContainer, FTacticalEffectSpec& GESpec) const;
	void OnApplied(FActiveTacticalEffectsContainer& ActiveGEContainer, FTacticalEffectSpec& GESpec) const;

public:
	const FGameplayTagContainer& GetAssetTags() const 
	{ 
		return mCachedAssetTags; 
	}

	const FGameplayTagContainer& GetGrantedTags() const 
	{ 
		return mCachedGrantedTags; 
	}

public:
	UPROPERTY(Category = "Effect", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Modifiers"))
	TArray<FTacticalModifierInfo> mModifiers;

public:
	// @brief 즉시 적용인지 기간 단위 적용인지 여부
	UPROPERTY(Category = "Duration", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DurationPolicy"))
	ETacticalEffectDurationType mDurationPolicy;

	// @brief 스태킹 가능한지 여부
	UPROPERTY(Category = "Stacking", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "StackingType"))
	ETacticalEffectStackingType mStackingType;

	// @brief 스태킹에 따른 영향을 배수로 받는지 여부
	UPROPERTY(Category = "Stacking", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "FactorInStackCount", EditConditionHides, EditCondition = "mStackingType != EGameplayEffectStackingType::None"))
	bool mFactorInStackCount;

public:
	// @brief 해당 Effect를 식별하기 위한 태그 캐시
	FGameplayTagContainer mCachedAssetTags;
	// @brief 해당 Owner에게 부여하기 위한 태그 캐시
	FGameplayTagContainer mCachedGrantedTags;
};