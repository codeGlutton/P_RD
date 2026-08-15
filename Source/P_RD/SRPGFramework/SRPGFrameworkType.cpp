#include "SRPGFramework/SRPGFrameworkType.h"

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
