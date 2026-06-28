/*****************************************************************//**
 * @file   TacticalTagCountContainer.h
 * @brief  수동식 태그 카운트 컨테이너 정의 헤더
 * @author 모호재
 * @date   2026-06-25
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "GameplayTagContainer.h"
#include "TacticalTagCountContainer.generated.h"

class UAttributeSetComponentModel;
struct FBoardCombatTargetSnapshotData;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTacticalEffectTagCountChanged, const FGameplayTag /*Tag*/, int32 /*Count*/);
DECLARE_DELEGATE(FDeferredTagChangeDelegate);

USTRUCT()
struct FTacticalTagCountItem
{
	GENERATED_USTRUCT_BODY()

	FTacticalTagCountItem() :
		mTag(FGameplayTag()),
		mCount(0)
	{
	}

	FTacticalTagCountItem(const FGameplayTag& Tag, const int32 Count = 1) : 
		mTag(Tag),
		mCount(Count)
	{
	}

public:
	bool operator == (const FTacticalTagCountItem& Other) const
	{
		return mTag.MatchesTagExact(Other.mTag);
	}
	bool operator == (const FGameplayTag& OtherTag) const
	{
		return mTag.MatchesTagExact(OtherTag);
	}

public:
	UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "Tag"))
	FGameplayTag mTag;

	UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "Count"))
	int32 mCount;
};

USTRUCT(BlueprintType)
struct FTacticalTagCountContainer
{
	GENERATED_BODY()

private:
	struct FDelegateInfo
	{
		FOnTacticalEffectTagCountChanged	OnNewOrRemove;
		FOnTacticalEffectTagCountChanged	OnAnyChange;
	};

public:
	FTacticalTagCountContainer() = default;

public:
	bool operator==(const FTacticalTagCountContainer& Other) const
	{
		if (mItems.Num() != Other.mItems.Num())
		{
			return false;
		}

		int32 Idx = INDEX_NONE;
		for (const FTacticalTagCountItem& Item : mItems)
		{
			Idx = Other.mItems.Find(Item);
			if (Idx == INDEX_NONE || Item.mCount != Other.mItems[Idx].mCount)
			{
				return false;
			}
		}
		return true;
	}

public:
	bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const;
	bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const;
	bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const;

public:
	void CaptureAllTags(FBoardCombatTargetSnapshotData& Snapshot) const;

public:
	void UpdateTagCount(const FGameplayTagContainer& Container, int32 CountDelta);
	bool UpdateTagCount(const FGameplayTag& Tag, int32 CountDelta);
	bool UpdateTagCount_DeferredParentRemoval(const FGameplayTag& Tag, int32 CountDelta, TArray<FDeferredTagChangeDelegate>& DeferredTagChangeDelegates);

public:
	void SetOwner(UAttributeSetComponentModel* ASC);
	bool SetTagCount(const FGameplayTag& Tag, int32 NewCount);
	int32 GetTagCount(FGameplayTag TagToCheck) const;
	int32 GetExplicitTagCount(const FGameplayTag& Tag) const;
	const FGameplayTagContainer& GetExplicitGameplayTags() const;

public:
	void Reset(bool ResetCallbacks = true);
	void FillParentTags();
	void Notify_StackCountChange(const FGameplayTag& Tag);

public:
	FOnTacticalEffectTagCountChanged& RegisterGameplayTagEvent(const FGameplayTag& Tag, EGameplayTagEventType::Type EventType = EGameplayTagEventType::NewOrRemoved);
	FOnTacticalEffectTagCountChanged& RegisterGenericGameplayEvent();

private:
	bool UpdateTagMap_Internal(const FGameplayTag& Tag, int32 CountDelta);
	bool UpdateTagMapDeferredParentRemoval_Internal(const FGameplayTag& Tag, int32 CountDelta, TArray<FDeferredTagChangeDelegate>& DeferredTagChangeDelegates);
	bool UpdateExplicitTags(const FGameplayTag& Tag, int32 CountDelta, bool DeferParentTagsOnRemove);
	bool GatherTagChangeDelegates(const FGameplayTag& Tag, int32 CountDelta, TArray<FDeferredTagChangeDelegate>& TagChangeDelegates);

private:
	// @brief 오너 객체
	UPROPERTY(Category = "Owner", VisibleAnywhere, meta = (DisplayName = "Owner"))
	TObjectPtr<UAttributeSetComponentModel> mOwner = nullptr;

private:
	// @brief 수동으로 넣은 태그 횟수를 담은 배열
	UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "Items"))
	TArray<FTacticalTagCountItem> mItems;

	// @brief 수동으로 넣은 태그 컨테이너
	UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "ExplicitTags"))
	FGameplayTagContainer mExplicitTags;

	// @brief 모든 태그 횟수를 가리키는 맵
	UPROPERTY(Category = "Tag", VisibleAnywhere, meta = (DisplayName = "TagCountMap"))
	TMap<FGameplayTag, int32> mTagCountMap;

	// @brief 모든 변경 대리자
	FOnTacticalEffectTagCountChanged OnAnyTagChangeDelegate;
	// @brief 특정 태그 변경 대리자
	TMap<FGameplayTag, FDelegateInfo> mTagEventMap;
};
