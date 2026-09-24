#pragma once

#include "Containers/Ticker.h"
#include "HAL/PlatformTime.h"

/** A cancelable hold timer measured in wall-clock seconds, independent of world time dilation. */
class FRealTimeLongPressTimer
{
public:
	~FRealTimeLongPressTimer() { Cancel(); }

	void Start(UObject* Owner, double DurationSeconds, FSimpleDelegate Callback)
	{
		Cancel();
		const double Deadline = FPlatformTime::Seconds() + DurationSeconds;
		Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(Owner,
			[this, Deadline, Callback](float)
			{
				if (FPlatformTime::Seconds() < Deadline) return true;
				Handle.Reset();
				Callback.ExecuteIfBound();
				return false;
			}));
	}

	void Cancel()
	{
		if (Handle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(Handle);
		Handle.Reset();
	}

	bool IsPending() const { return Handle.IsValid(); }

private:
	FTSTicker::FDelegateHandle Handle;
};
