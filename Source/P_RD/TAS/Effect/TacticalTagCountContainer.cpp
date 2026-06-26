#include "TAS/Effect/TacticalTagCountContainer.h"

bool FTacticalTagCountContainer::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	return mTagCountMap.FindRef(TagToCheck) > 0;
}

bool FTacticalTagCountContainer::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (TagContainer.Num() == 0)
	{
		return true;
	}

	bool AllMatch = true;
	for (const FGameplayTag& Tag : TagContainer)
	{
		if (mTagCountMap.FindRef(Tag) <= 0)
		{
			AllMatch = false;
			break;
		}
	}
	return AllMatch;
}

bool FTacticalTagCountContainer::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (TagContainer.Num() == 0)
	{
		return false;
	}

	bool AnyMatch = false;
	for (const FGameplayTag& Tag : TagContainer)
	{
		if (mTagCountMap.FindRef(Tag) > 0)
		{
			AnyMatch = true;
			break;
		}
	}
	return AnyMatch;
}

void FTacticalTagCountContainer::UpdateTagCount(const FGameplayTagContainer& Container, int32 CountDelta)
{
	if (CountDelta != 0)
	{
		bool UpdatedAny = false;
		TArray<FDeferredTagChangeDelegate> DeferredTagChangeDelegates;
		for (auto TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
		{
			UpdatedAny |= UpdateTagMapDeferredParentRemoval_Internal(*TagIt, CountDelta, DeferredTagChangeDelegates);
		}

		// mExplicitTags.FillParentTags(); (FGameplayTagContainer has no FillParentTags)

		for (FDeferredTagChangeDelegate& Delegate : DeferredTagChangeDelegates)
		{
			Delegate.Execute();
		}
	}
}

bool FTacticalTagCountContainer::UpdateTagCount(const FGameplayTag& Tag, int32 CountDelta)
{
	if (Tag.IsValid() && CountDelta != 0)
	{
		return UpdateTagMap_Internal(Tag, CountDelta);
	}
	return false;
}

bool FTacticalTagCountContainer::UpdateTagCount_DeferredParentRemoval(const FGameplayTag& Tag, int32 CountDelta, TArray<FDeferredTagChangeDelegate>& DeferredTagChangeDelegates)
{
	if (CountDelta != 0)
	{
		return UpdateTagMapDeferredParentRemoval_Internal(Tag, CountDelta, DeferredTagChangeDelegates);
	}
	return false;
}

void FTacticalTagCountContainer::SetOwner(UAttributeSetComponentModel* ASC)
{
	mOwner = ASC;
}

bool FTacticalTagCountContainer::SetTagCount(const FGameplayTag& Tag, int32 NewCount)
{
	if (Tag.IsValid() == false)
	{
		return false;
	}

	int32 ExistingCount = 0;
	const int32 Idx = mItems.Find(Tag);
	if (Idx != INDEX_NONE)
	{
		ExistingCount = mItems[Idx].mCount;
	}

	int32 CountDelta = NewCount - ExistingCount;
	if (CountDelta != 0)
	{
		return UpdateTagMap_Internal(Tag, CountDelta);
	}

	return false;
}

int32 FTacticalTagCountContainer::GetTagCount(FGameplayTag TagToCheck) const
{
	if (const int32* Ptr = mTagCountMap.Find(TagToCheck))
	{
		return *Ptr;
	}
	return 0;
}

int32 FTacticalTagCountContainer::GetExplicitTagCount(const FGameplayTag& Tag) const
{
	const int32 Idx = mItems.Find(Tag);
	if (Idx != INDEX_NONE)
	{
		return mItems[Idx].mCount;
	}
	return 0;
}

const FGameplayTagContainer& FTacticalTagCountContainer::GetExplicitGameplayTags() const
{
	return mExplicitTags;
}

void FTacticalTagCountContainer::Reset(bool ResetCallbacks)
{
	mTagCountMap.Reset();

	if (ResetCallbacks == true)
	{
		mTagEventMap.Reset();
		OnAnyTagChangeDelegate.Clear();
	}
}

void FTacticalTagCountContainer::FillParentTags()
{
	// mExplicitTags.FillParentTags();
}

void FTacticalTagCountContainer::Notify_StackCountChange(const FGameplayTag& Tag)
{
	FGameplayTagContainer TagAndParentsContainer = Tag.GetGameplayTagParents();
	TagAndParentsContainer.AddTag(Tag);
	for (auto CompleteTagIt = TagAndParentsContainer.CreateConstIterator(); CompleteTagIt; ++CompleteTagIt)
	{
		const FGameplayTag& CurTag = *CompleteTagIt;
		FDelegateInfo* DelegateInfo = mTagEventMap.Find(CurTag);
		if (DelegateInfo != nullptr)
		{
			int32 TagCount = mTagCountMap.FindOrAdd(CurTag);
			DelegateInfo->OnAnyChange.Broadcast(CurTag, TagCount);
		}
	}
}

FOnTacticalEffectTagCountChanged& FTacticalTagCountContainer::RegisterGameplayTagEvent(const FGameplayTag& Tag, EGameplayTagEventType::Type EventType)
{
	FDelegateInfo& Info = mTagEventMap.FindOrAdd(Tag);
	if (EventType == EGameplayTagEventType::NewOrRemoved)
	{
		return Info.OnNewOrRemove;
	}
	return Info.OnAnyChange;
}

FOnTacticalEffectTagCountChanged& FTacticalTagCountContainer::RegisterGenericGameplayEvent()
{
	return OnAnyTagChangeDelegate;
}

bool FTacticalTagCountContainer::UpdateTagMap_Internal(const FGameplayTag& Tag, int32 CountDelta)
{
	checkf(Tag.IsValid() == true, TEXT("변동되는 태그가 없음"));
	checkf(CountDelta != 0, TEXT("태그 변동 수가 0개"));

	if (UpdateExplicitTags(Tag, CountDelta, false) == false)
	{
		return false;
	}

	TArray<FDeferredTagChangeDelegate> DeferredTagChangeDelegates;
	bool SignificantChange = GatherTagChangeDelegates(Tag, CountDelta, DeferredTagChangeDelegates);
	for (FDeferredTagChangeDelegate& Delegate : DeferredTagChangeDelegates)
	{
		/* 태그 변경이 끝나고 난 뒤, 뒤늦게 모아둔 변동 대리자 호출 */

		Delegate.Execute();
	}

	return SignificantChange;
}

bool FTacticalTagCountContainer::UpdateTagMapDeferredParentRemoval_Internal(const FGameplayTag& Tag, int32 CountDelta, TArray<FDeferredTagChangeDelegate>& DeferredTagChangeDelegates)
{
	if (UpdateExplicitTags(Tag, CountDelta, true) == false)
	{
		return false;
	}

	return GatherTagChangeDelegates(Tag, CountDelta, DeferredTagChangeDelegates);
}

bool FTacticalTagCountContainer::UpdateExplicitTags(const FGameplayTag& Tag, int32 CountDelta, bool DeferParentTagsOnRemove)
{
	int32 TagCountIndex = mItems.Find(Tag);

	if (TagCountIndex != INDEX_NONE)
	{
		FTacticalTagCountItem& TagCountItem = mItems[TagCountIndex];
		TagCountItem.mCount += CountDelta;
		
		if (TagCountItem.mCount <= 0)
		{
			mItems.RemoveAtSwap(TagCountIndex);
			mExplicitTags.RemoveTag(Tag);
		}
	}
	else if (CountDelta > 0)
	{
		mItems.Add(FTacticalTagCountItem(Tag, CountDelta));
		mExplicitTags.AddTag(Tag);
	}
	else
	{
		return false;
	}

	return true;
}

bool FTacticalTagCountContainer::GatherTagChangeDelegates(const FGameplayTag& Tag, int32 CountDelta, TArray<FDeferredTagChangeDelegate>& TagChangeDelegates)
{
	FGameplayTagContainer TagAndParentsContainer = Tag.GetGameplayTagParents();
	TagAndParentsContainer.AddTag(Tag);
	bool CreatedSignificantChange = false;
	for (auto CompleteTagIt = TagAndParentsContainer.CreateConstIterator(); CompleteTagIt; ++CompleteTagIt)
	{
		/* 부모 자식 모두 태그 갱신 */

		const FGameplayTag& CurTag = *CompleteTagIt;
		int32& TagCountRef = mTagCountMap.FindOrAdd(CurTag);

		const int32 OldCount = TagCountRef;
		int32 NewTagCount = FMath::Max(OldCount + CountDelta, 0);

		TagCountRef = NewTagCount;

		/* 대리자 호출 부분 */

		const bool SignificantChange = (OldCount == 0 || NewTagCount == 0);
		CreatedSignificantChange |= SignificantChange;
		if (SignificantChange)
		{
			/* 추가 삭제 대리자 호출 필요 */

			TagChangeDelegates.AddDefaulted();
			TagChangeDelegates.Last().BindLambda([Delegate = OnAnyTagChangeDelegate, CurTag, NewTagCount]()
				{
					Delegate.Broadcast(CurTag, NewTagCount);
				});
		}

		FDelegateInfo* DelegateInfo = mTagEventMap.Find(CurTag);
		if (DelegateInfo != nullptr)
		{
			TagChangeDelegates.AddDefaulted();
			TagChangeDelegates.Last().BindLambda([Delegate = DelegateInfo->OnAnyChange, CurTag, NewTagCount]()
				{
					Delegate.Broadcast(CurTag, NewTagCount);
				});

			if (SignificantChange == true)
			{
				/* 추가 삭제 대리자 호출 필요 */

				TagChangeDelegates.AddDefaulted();
				TagChangeDelegates.Last().BindLambda([Delegate = DelegateInfo->OnNewOrRemove, CurTag, NewTagCount]()
					{
						Delegate.Broadcast(CurTag, NewTagCount);
					});
			}
		}
	}

	return CreatedSignificantChange;
}
