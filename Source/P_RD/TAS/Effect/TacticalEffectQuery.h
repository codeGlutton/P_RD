/*****************************************************************//**
 * @file   TacticalEffectQuery.h
 * @brief  이펙트 탐색 쿼리 객체 정의 헤더
 * @author 모호재
 * @date   2026-08-23
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "TacticalEffectQuery.generated.h"

class UTacticalEffect;
struct FTacticalEffectSpec;

DECLARE_DELEGATE_RetVal_OneParam(bool, FActiveTacticalEffectQueryCustomMatch, const FActiveTacticalEffect&);

/**
 * @brief 이펙트 탐색 쿼리 객체
 */
USTRUCT(BlueprintType)
struct P_RD_API FTacticalEffectQuery
{
	GENERATED_BODY()

public:
	FTacticalEffectQuery() = default;
	FTacticalEffectQuery(FActiveTacticalEffectQueryCustomMatch InCustomMatchDelegate);
	FTacticalEffectQuery(const FTacticalEffectQuery& Other) = default;
	FTacticalEffectQuery(FTacticalEffectQuery&& Other) = default;

public:
	FTacticalEffectQuery& operator=(FTacticalEffectQuery&& Other) = default;
	FTacticalEffectQuery& operator=(const FTacticalEffectQuery& Other) = default;

	bool operator==(const FTacticalEffectQuery& Other) const;
	bool operator!=(const FTacticalEffectQuery& Other) const;

public:
	bool Matches(const FActiveTacticalEffect& Effect) const;
	bool Matches(const FTacticalEffectSpec& Spec) const;

	bool IsEmpty() const;

public:
	static FTacticalEffectQuery MakeQuery_MatchAnyOwningTags(const FGameplayTagContainer& InTags);
	static FTacticalEffectQuery MakeQuery_MatchAllOwningTags(const FGameplayTagContainer& InTags);
	static FTacticalEffectQuery MakeQuery_MatchNoOwningTags(const FGameplayTagContainer& InTags);

	static FTacticalEffectQuery MakeQuery_MatchAnyEffectTags(const FGameplayTagContainer& InTags);
	static FTacticalEffectQuery MakeQuery_MatchAllEffectTags(const FGameplayTagContainer& InTags);
	static FTacticalEffectQuery MakeQuery_MatchNoEffectTags(const FGameplayTagContainer& InTags);

public:
	FActiveTacticalEffectQueryCustomMatch mCustomMatchDelegate;

public:
	UPROPERTY(Category = Query, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "OwningTagQuery"))
	FGameplayTagQuery mOwningTagQuery;

	UPROPERTY(Category = Query, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectTagQuery"))
	FGameplayTagQuery mEffectTagQuery;

	UPROPERTY(Category = Query, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ModifyingAttribute"))
	FTacticalAttribute mModifyingAttribute;

	UPROPERTY(Category = Query, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectSource"))
	TObjectPtr<const UObject> mEffectSource = nullptr;

	UPROPERTY(Category = Query, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectDefinition"))
	TSubclassOf<UTacticalEffect> mEffectDefinition = nullptr;

	TArray<FActiveTacticalEffectHandle> mIgnoreHandles;
};
