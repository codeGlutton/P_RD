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
