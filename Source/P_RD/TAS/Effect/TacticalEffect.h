/*****************************************************************//**
 * @file   TacticalEffect.h
 * @brief  속성값 및 태그를 변경하는 객체 정의 헤더
 * @author 모호재, 김준형
 * @date   2026-06-24
 *********************************************************************/

#pragma once
#include "AttributeSet/AttributeSetMinimal.h"
#include "UObject/Object.h"
#include "TacticalEffect.generated.h"

class UBoardActorModel;
struct FTileIndex;

class UTacticalEffect;
class UTacticalEffectContext;

/**
 * @brief 속성값 수정 정보
 */
USTRUCT(BlueprintType)
struct FTacticalEffectModifiedAttribute
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Attribute", VisibleAnywhere, meta = (DisplayName = "Attribute"))
	FGameplayAttribute mAttribute;

	UPROPERTY(Category = "Attribute", VisibleAnywhere, meta = (DisplayName = "TotalMagnitude"))
	float mTotalMagnitude;
};

/**
 * @brief 태그 수정 정보
 */
USTRUCT(BlueprintType)
struct FTacticalEffectModifiedTag
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "Tag"))
	FGameplayTag mTag;

	UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "TotalMagnitude"))
	int32 mTotalMagnitude;
};

/**
 * @brief  어떤 Effect를 어떤 방식으로 누가 적용시키는지에 대한 런타임 정보 객체
 */
USTRUCT(BlueprintType)
struct FTacticalEffectSpec
{
	GENERATED_BODY()

public:
	// @brief 원본 정적 Effect 정보
	UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "EffectClass"))
	TObjectPtr<const UTacticalEffect> mEffectClass;

	// @brief 속성값 변경 요청 정보
	UPROPERTY(Category = "Cache", VisibleAnywhere, meta = (DisplayName = "ModifiedAttributes"))
	TArray<FTacticalEffectModifiedAttribute> mModifiedAttributes;

	// @brief 태그값 변경 요청 정보
	UPROPERTY(Category = "Cache", VisibleAnywhere, meta = (DisplayName = "ModifiedTags"))
	TArray<FTacticalEffectModifiedTag> mModifiedTags;

private:
	// @brief 해당 Effect가 적용되는 스택 수
	UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "StackCount"))
	int32 mStackCount;

	UPROPERTY()
	TSharedPtr<FTacticalEffectMetaData> EffectContext;
};

/**
 * @brief  속성값 및 태그를 변경하는 객체
 */
UCLASS(Blueprintable, BlueprintType)
class P_RD_API UTacticalEffect : public UObject
{
	GENERATED_BODY()

public:
	virtual void ActivateEffect(const UBoardActorModel& Caster, const FTileIndex& TargetTile, const UTacticalEffectContext* EffectContext) PURE_VIRTUAL(UTacticalEffect::ActivateEffect, return;);

protected:
	UBoardActorModel* ExtractTarget(const FTileIndex& TargetTile, const UTacticalEffectContext* EffectContext);
};