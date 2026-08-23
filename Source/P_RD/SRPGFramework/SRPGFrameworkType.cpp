#include "SRPGFramework/SRPGFrameworkType.h"

void FSRPGCombatRoundEvent::SetEventName(const FName& NewName)
{
	mEventName = NewName;
}

FName FSRPGCombatRoundEvent::GetEventName() const
{
	return mEventName;
}

int32 FSRPGCombatRoundEvent::GetLayerIndex() const
{
	return mLayerIndex;
}

bool FSRPGCombatRoundEvent::IsTriggered() const
{
	return mIsTriggered;
}

bool FSRPGCombatRoundEvent::IsActivated() const
{
	return mIsActivated;
}

void FSRPGCombatRoundEvent::TryToTrigger(TSharedRef<FPresentationBarrier> RoundBarrier, USRPGCombatModel* Model)
{
	if (mIsActivated == false)
	{
		return;
	}

	if (mIsTriggered == false && CanTrigger_Internal(Model) == true)
	{
		mIsTriggered = true;
		PreTrigger_Internal(Model);
	}

	if (mIsTriggered == true)
	{
		if (Trigger_Internal(RoundBarrier, Model) == ESRPGCombatRoundEventResult::End)
		{
			if (mIsLoop == true)
			{
				mIsTriggered = false;
				Reset_Internal(Model);
			}
			else
			{
				mIsActivated = false;
			}
		}
	}
	else
	{
		Warning_Internal(Model);
	}
}

TInstancedStruct<FSRPGCombatRoundEvent>& FSRPGCombatRoundEventContainer::operator[](int32 Index)
{
	return mEvents[Index];
}

const TInstancedStruct<FSRPGCombatRoundEvent>& FSRPGCombatRoundEventContainer::operator[](int32 Index) const
{
	return mEvents[Index];
}

TArray<TInstancedStruct<FSRPGCombatRoundEvent>>& FSRPGCombatRoundEventContainer::GetEvents()
{
	return mEvents;
}

const TArray<TInstancedStruct<FSRPGCombatRoundEvent>>& FSRPGCombatRoundEventContainer::GetEvents() const
{
	return mEvents;
}

int32 FSRPGCombatRoundEventContainer::Num() const
{
	return mEvents.Num();
}

bool FSRPGCombatRoundEventContainer::IsEmpty() const
{
	return mEvents.IsEmpty();
}

bool FSRPGCombatRoundEventContainer::IsValidIndex(int32 Index) const
{
	return mEvents.IsValidIndex(Index);
}

bool FSRPGCombatRoundEventContainer::RemoveAt(int32 Index)
{
	if (mEvents.IsValidIndex(Index) == false)
	{
		return false;
	}

	if (IsLocked() == true)
	{
		mReservedRemoveIndices.AddUnique(Index);
		return true;
	}

	mEvents.RemoveAt(Index);
	return true;
}

void FSRPGCombatRoundEventContainer::Empty()
{
	if (IsLocked() == true)
	{
		mReservedAdds.Empty();
		mReservedRemoveIndices.Empty();
		for (int32 Index = 0; Index < mEvents.Num(); ++Index)
		{
			mReservedRemoveIndices.Add(Index);
		}
		return;
	}

	mEvents.Empty();
	mReservedAdds.Empty();
	mReservedRemoveIndices.Empty();
}

void FSRPGCombatRoundEventContainer::AddEvent(TInstancedStruct<FSRPGCombatRoundEvent> Event)
{
	if (Event.IsValid() == false)
	{
		return;
	}

	if (IsLocked() == true)
	{
		mReservedAdds.Add(MoveTemp(Event));
		return;
	}

	const int32 NewLayerIndex = Event.Get<FSRPGCombatRoundEvent>().GetLayerIndex();

	int32 InsertIndex = 0;
	for (; InsertIndex < mEvents.Num(); ++InsertIndex)
	{
		const int32 ExistingLayerIndex = mEvents[InsertIndex].Get<FSRPGCombatRoundEvent>().GetLayerIndex();
		if (NewLayerIndex < ExistingLayerIndex)
		{
			break;
		}
	}

	mEvents.Insert(MoveTemp(Event), InsertIndex);
}

FSRPGCombatRoundEvent* FSRPGCombatRoundEventContainer::FindEvent(const FName& EventName)
{
	if (EventName.IsNone() == true)
	{
		return nullptr;
	}

	for (TInstancedStruct<FSRPGCombatRoundEvent>& Event : mEvents)
	{
		if (Event.IsValid() == true && Event.Get<FSRPGCombatRoundEvent>().GetEventName() == EventName)
		{
			return Event.GetMutablePtr();
		}
	}

	return nullptr;
}

const FSRPGCombatRoundEvent* FSRPGCombatRoundEventContainer::FindEvent(const FName& EventName) const
{
	if (EventName.IsNone() == true)
	{
		return nullptr;
	}

	for (const TInstancedStruct<FSRPGCombatRoundEvent>& Event : mEvents)
	{
		if (Event.IsValid() == true && Event.Get<FSRPGCombatRoundEvent>().GetEventName() == EventName)
		{
			return Event.GetPtr();
		}
	}

	return nullptr;
}

bool FSRPGCombatRoundEventContainer::RemoveEvent(const FName& EventName)
{
	if (EventName.IsNone() == true)
	{
		return false;
	}

	bool IsRemovedAny = false;

	for (int32 Index = mEvents.Num() - 1; Index >= 0; --Index)
	{
		if (mEvents[Index].IsValid() == true && mEvents[Index].Get<FSRPGCombatRoundEvent>().GetEventName() == EventName)
		{
			if (RemoveAt(Index) == true)
			{
				IsRemovedAny = true;
			}
		}
	}

	if (IsLocked() == true)
	{
		for (int32 Index = mReservedAdds.Num() - 1; Index >= 0; --Index)
		{
			if (mReservedAdds[Index].IsValid() == true && mReservedAdds[Index].Get<FSRPGCombatRoundEvent>().GetEventName() == EventName)
			{
				mReservedAdds.RemoveAt(Index);
				IsRemovedAny = true;
			}
		}
	}

	return IsRemovedAny;
}

void FSRPGCombatRoundEventContainer::Lock()
{
	++mLockCount;
}

void FSRPGCombatRoundEventContainer::Unlock()
{
	--mLockCount;
	if (mLockCount <= 0)
	{
		mLockCount = 0;
		FlushPendingOperations();
	}
}

bool FSRPGCombatRoundEventContainer::IsLocked() const
{
	return mLockCount > 0;
}

int32 FSRPGCombatRoundEventContainer::GetLockCount() const
{
	return mLockCount;
}

void FSRPGCombatRoundEventContainer::FlushPendingOperations()
{
	/* 1. 지연된 삭제 처리 (내림차순 정렬 후 삭제) */

	if (mReservedRemoveIndices.IsEmpty() == false)
	{
		mReservedRemoveIndices.Sort([](int32 A, int32 B) {
			return A > B;
		});

		for (const int32 Index : mReservedRemoveIndices)
		{
			if (mEvents.IsValidIndex(Index) == true)
			{
				mEvents.RemoveAt(Index);
			}
		}
		mReservedRemoveIndices.Empty();
	}

	/* 2. 지연된 추가 처리 (LayerIndex 순서 정렬 삽입) */

	if (mReservedAdds.IsEmpty() == false)
	{
		for (TInstancedStruct<FSRPGCombatRoundEvent>& Event : mReservedAdds)
		{
			if (Event.IsValid() == false)
			{
				continue;
			}

			const int32 NewLayerIndex = Event.Get<FSRPGCombatRoundEvent>().GetLayerIndex();

			int32 InsertIndex = 0;
			for (; InsertIndex < mEvents.Num(); ++InsertIndex)
			{
				const int32 ExistingLayerIndex = mEvents[InsertIndex].Get<FSRPGCombatRoundEvent>().GetLayerIndex();
				if (NewLayerIndex < ExistingLayerIndex)
				{
					break;
				}
			}

			mEvents.Insert(MoveTemp(Event), InsertIndex);
		}
		mReservedAdds.Empty();
	}
}

