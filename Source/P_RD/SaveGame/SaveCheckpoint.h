#pragma once

#include "CoreMinimal.h"

/** Checked payloads in alternating slots keep the last successful save if a write is interrupted. */
namespace RDCheckpoint
{
	P_RD_API bool Serialize(UObject* Object, TArray<uint8>& OutData);
	P_RD_API bool Deserialize(const TArray<uint8>& Data, UObject* Object);
	P_RD_API bool SaveSlot(const FString& Slot, const TArray<uint8>& Data);
	P_RD_API bool LoadSlot(const FString& Slot, TArray<uint8>& OutData);
	P_RD_API void LoadSlotAsync(const FString& Slot, TFunction<void(bool, TArray<uint8>)> Completion);
	P_RD_API bool DoesSlotExist(const FString& Slot);
	P_RD_API void SaveSlotAsync(const FString& Slot, TArray<uint8> Data, TFunction<void(bool)> Completion);
}

/** A combat checkpoint includes the complete serialized run, including both random streams. */
struct P_RD_API FRunRoomCheckpoint
{
	bool Capture(UObject* Run, bool bIncompleteCombat);
	bool MakeSaveData(UObject* Run, TArray<uint8>& OutData) const;
	bool Restore(UObject* Run) const;
	void CompleteCombat(bool bExitPending);
	bool CancelExit();
	void Reset();
	bool IsCombatEntry() const { return bUseEntry; }

private:
	TArray<uint8> EntryData;
	bool bUseEntry = false;
	bool bCompletionDeferred = false;
};
