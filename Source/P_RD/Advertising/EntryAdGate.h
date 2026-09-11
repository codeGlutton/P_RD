#pragma once

#include "CoreMinimal.h"

/** One continuation per entry request; duplicate/stale Android callbacks cannot start another run. */
class FEntryAdGate
{
public:
	int32 Begin(FSimpleDelegate Callback)
	{
		if (ActiveTicket != 0) return 0;
		ActiveTicket = ++NextTicket;
		Continuation = MoveTemp(Callback);
		return ActiveTicket;
	}

	void Complete(int32 Ticket)
	{
		if (Ticket == 0 || Ticket != ActiveTicket) return;
		FSimpleDelegate Callback = MoveTemp(Continuation);
		ActiveTicket = 0;
		Callback.ExecuteIfBound();
	}

	void Cancel()
	{
		Continuation.Unbind();
		ActiveTicket = 0;
	}

private:
	int32 NextTicket = 0;
	int32 ActiveTicket = 0;
	FSimpleDelegate Continuation;
};
