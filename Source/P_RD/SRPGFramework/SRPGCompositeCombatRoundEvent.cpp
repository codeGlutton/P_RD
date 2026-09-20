#include "SRPGFramework/SRPGCompositeCombatRoundEvent.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"

void FSRPGCompositeCombatRoundEvent::AddEvent(TInstancedStruct<FSRPGCombatRoundEvent> Event)
{
	mEvents.AddEvent(MoveTemp(Event));
}

FSRPGCombatRoundEvent* FSRPGCompositeCombatRoundEvent::FindEvent(const FName& InEventName)
{
	return mEvents.FindEvent(InEventName);
}

const FSRPGCombatRoundEvent* FSRPGCompositeCombatRoundEvent::FindEvent(const FName& InEventName) const
{
	return mEvents.FindEvent(InEventName);
}

bool FSRPGCompositeCombatRoundEvent::CanTrigger_Internal(USRPGCombatModel* Model) const
{
	bool CanTriggerAllEvent = true;

	for (const TInstancedStruct<FSRPGCombatRoundEvent>& Event : mEvents)
	{
		if (Event.IsValid() == false || Event.Get().CanTrigger_Internal(Model) == false)
		{
			CanTriggerAllEvent = false;
		}
	}

	return CanTriggerAllEvent;
}

void FSRPGCompositeCombatRoundEvent::Warning_Internal(USRPGCombatModel* Model) const
{
	for (const TInstancedStruct<FSRPGCombatRoundEvent>& Event : mEvents)
	{
		if (Event.IsValid() == true)
		{
			Event.Get().Warning_Internal(Model);
		}
	}
}

void FSRPGCompositeCombatRoundEvent::PreTrigger_Internal(USRPGCombatModel* Model)
{
	FSRPGCombatRoundEventContainer::FScopedLock Lock(mEvents);

	for (TInstancedStruct<FSRPGCombatRoundEvent>& Event : mEvents)
	{
		if (Event.IsValid() == true)
		{
			FSRPGCombatRoundEvent& MutableEvent = Event.GetMutable();
			MutableEvent.mIsTriggered = true;

			MutableEvent.PreTrigger_Internal(Model);
		}
	}
}

ESRPGCombatRoundEventResult FSRPGCompositeCombatRoundEvent::Trigger_Internal(TSharedPtr<FPresentationBarrier> RoundBarrier, USRPGCombatModel* Model)
{
	FSRPGCombatRoundEventContainer::FScopedLock Lock(mEvents);

	bool IsAllFinished = true;
	for (TInstancedStruct<FSRPGCombatRoundEvent>& Event : mEvents)
	{
		if (Event.IsValid() == false)
		{
			continue;
		}

		FSRPGCombatRoundEvent& MutableEvent = Event.GetMutable();
		if (MutableEvent.IsTriggered() == false)
		{
			continue;
		}

		if (MutableEvent.Trigger_Internal(RoundBarrier, Model) == ESRPGCombatRoundEventResult::End)
		{
			MutableEvent.mIsTriggered = false;
		}
		else
		{
			IsAllFinished = false;
		}
	}

	return IsAllFinished == true ? ESRPGCombatRoundEventResult::End : ESRPGCombatRoundEventResult::Ongoing;
}

void FSRPGCompositeCombatRoundEvent::Reset_Internal(USRPGCombatModel* Model)
{
	FSRPGCombatRoundEventContainer::FScopedLock Lock(mEvents);

	const int32 EventNum = mEvents.Num();
	for (int32 EventIndex = 0; EventIndex < EventNum; ++EventIndex)
	{
		if (mEvents[EventIndex].IsValid() == true)
		{
			FSRPGCombatRoundEvent& MutableEvent = mEvents[EventIndex].GetMutable();
			if (MutableEvent.mIsLoop == true)
			{
				MutableEvent.Reset_Internal(Model);
			}
			else
			{
				MutableEvent.mIsActivated = false;
				mEvents.RemoveAt(EventIndex);
			}
		}
	}
}

