#include "SaveGame/SaveCheckpoint.h"

#include "Async/Async.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "SaveGame/BinarySaveGame.h"
#include "SaveGame/SaveGameArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Tasks/Pipe.h"

namespace
{
	constexpr uint32 SaveMagic = 0x52445356;
	constexpr uint32 SaveVersion = 1;
	constexpr int32 HeaderSize = sizeof(uint32) * 4 + sizeof(uint64);
	constexpr int32 MaxPayloadSize = 64 * 1024 * 1024;
	UE::Tasks::FPipe SavePipe(TEXT("RunCheckpointIO"));

	bool ReadBank(const FString& Slot, TArray<uint8>& OutData, uint64& OutGeneration)
	{
		if (!UGameplayStatics::DoesSaveGameExist(Slot, 0)) return false;
		TArray<uint8> Bytes;
		if (!UGameplayStatics::LoadDataFromSlot(Bytes, Slot, 0)
			|| Bytes.Num() < HeaderSize || Bytes.Num() > HeaderSize + MaxPayloadSize)
		{
			return false;
		}
		FMemoryReader Reader(Bytes);
		uint32 Magic = 0, Version = 0, Size = 0, Checksum = 0;
		uint64 Generation = 0;
		Reader << Magic << Version << Generation << Size << Checksum;
		if (Reader.IsError() || Magic != SaveMagic || Version != SaveVersion
			|| Size == 0 || Size > MaxPayloadSize || Size != Bytes.Num() - HeaderSize
			|| FCrc::MemCrc32(Bytes.GetData() + HeaderSize, Size,
				FCrc::MemCrc32(&Generation, sizeof(Generation))) != Checksum)
		{
			return false;
		}
		OutData.Reset(Size);
		OutData.Append(Bytes.GetData() + HeaderSize, Size);
		OutGeneration = Generation;
		return true;
	}

	bool WriteSlot(const FString& Slot, const TArray<uint8>& Data)
	{
		if (Slot.IsEmpty() || Data.IsEmpty() || Data.Num() > MaxPayloadSize)
		{
			return false;
		}
		TArray<uint8> Existing;
		uint64 GenerationA = 0, GenerationB = 0;
		const bool bHasA = ReadBank(Slot + TEXT("_A"), Existing, GenerationA);
		const bool bHasB = ReadBank(Slot + TEXT("_B"), Existing, GenerationB);
		const FString Destination = Slot + ((!bHasA || (bHasB && GenerationA <= GenerationB))
			? TEXT("_A") : TEXT("_B"));
		uint32 Magic = SaveMagic, Version = SaveVersion, Size = Data.Num();
		uint64 Generation = FMath::Max(GenerationA, GenerationB) + 1;
		uint32 Checksum = FCrc::MemCrc32(Data.GetData(), Data.Num(), FCrc::MemCrc32(&Generation, sizeof(Generation)));
		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes);
		Writer << Magic << Version << Generation << Size << Checksum;
		Writer.Serialize(const_cast<uint8*>(Data.GetData()), Data.Num());
		if (Writer.IsError() || !UGameplayStatics::SaveDataToSlot(Bytes, Destination, 0))
		{
			return false;
		}
		uint64 VerifiedGeneration = 0;
		return ReadBank(Destination, Existing, VerifiedGeneration)
			&& VerifiedGeneration == Generation && Existing == Data;
	}
}

bool RDCheckpoint::Serialize(UObject* Object, TArray<uint8>& OutData)
{
	OutData.Reset();
	if (!IsValid(Object)) return false;
	FMemoryWriter Writer(OutData, true);
	FSaveGameArchive Archive(Writer);
	Object->Serialize(Archive);
	return !Archive.IsError() && !OutData.IsEmpty();
}

bool RDCheckpoint::Deserialize(const TArray<uint8>& Data, UObject* Object)
{
	if (!IsValid(Object) || Data.IsEmpty() || Data.Num() > MaxPayloadSize) return false;
	FMemoryReader Reader(Data, true);
	FSaveGameArchive Archive(Reader);
	Object->Serialize(Archive);
	return !Archive.IsError() && Reader.Tell() == Data.Num();
}

bool RDCheckpoint::SaveSlot(const FString& Slot, const TArray<uint8>& Data)
{
	bool bSucceeded = false;
	SavePipe.Launch(UE_SOURCE_LOCATION, [&]() { bSucceeded = WriteSlot(Slot, Data); }).Wait();
	return bSucceeded;
}

void RDCheckpoint::SaveSlotAsync(const FString& Slot, TArray<uint8> Data, TFunction<void(bool)> Completion)
{
	SavePipe.Launch(UE_SOURCE_LOCATION, [Slot, Data = MoveTemp(Data), Completion = MoveTemp(Completion)]() mutable
	{
		const bool bSucceeded = WriteSlot(Slot, Data);
		AsyncTask(ENamedThreads::GameThread, [Completion = MoveTemp(Completion), bSucceeded]() mutable
		{
			Completion(bSucceeded);
		});
	});
}

bool RDCheckpoint::LoadSlot(const FString& Slot, TArray<uint8>& OutData)
{
	if (Slot.IsEmpty()) return false;
	bool bFound = false;
	SavePipe.Launch(UE_SOURCE_LOCATION, [&]()
	{
		TArray<uint8> A, B;
		uint64 GenerationA = 0, GenerationB = 0;
		const bool bHasA = ReadBank(Slot + TEXT("_A"), A, GenerationA);
		const bool bHasB = ReadBank(Slot + TEXT("_B"), B, GenerationB);
		bFound = bHasA || bHasB;
		if (bFound) OutData = bHasA && (!bHasB || GenerationA >= GenerationB) ? MoveTemp(A) : MoveTemp(B);
	}).Wait();
	if (bFound) return true;
	// Preserve installations made before checked, alternating slots were introduced.
	if (!UGameplayStatics::DoesSaveGameExist(Slot, 0)) return false;
	if (UBinarySaveGame* Legacy = Cast<UBinarySaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0)))
	{
		OutData = Legacy->mData;
		return !OutData.IsEmpty();
	}
	return false;
}

bool RDCheckpoint::DoesSlotExist(const FString& Slot)
{
	return UGameplayStatics::DoesSaveGameExist(Slot + TEXT("_A"), 0)
		|| UGameplayStatics::DoesSaveGameExist(Slot + TEXT("_B"), 0)
		|| UGameplayStatics::DoesSaveGameExist(Slot, 0);
}

void RDCheckpoint::LoadSlotAsync(const FString& Slot, TFunction<void(bool, TArray<uint8>)> Completion)
{
	SavePipe.Launch(UE_SOURCE_LOCATION, [Slot, Completion = MoveTemp(Completion)]() mutable
	{
		TArray<uint8> A, B;
		uint64 GenerationA = 0, GenerationB = 0;
		const bool bHasA = ReadBank(Slot + TEXT("_A"), A, GenerationA);
		const bool bHasB = ReadBank(Slot + TEXT("_B"), B, GenerationB);
		const bool bFound = bHasA || bHasB;
		TArray<uint8> Data = bHasA && (!bHasB || GenerationA >= GenerationB) ? MoveTemp(A) : MoveTemp(B);
		AsyncTask(ENamedThreads::GameThread, [Slot, bFound, Data = MoveTemp(Data), Completion = MoveTemp(Completion)]() mutable
		{
			bool bLoaded = bFound;
			if (!bLoaded && UGameplayStatics::DoesSaveGameExist(Slot, 0))
			{
				if (UBinarySaveGame* Legacy = Cast<UBinarySaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0)))
				{
					Data = Legacy->mData;
					bLoaded = !Data.IsEmpty();
				}
			}
			Completion(bLoaded, MoveTemp(Data));
		});
	});
}

bool FRunRoomCheckpoint::Capture(UObject* Run, const bool bIncompleteCombat)
{
	Reset();
	if (!bIncompleteCombat) return true;
	bUseEntry = RDCheckpoint::Serialize(Run, EntryData);
	return bUseEntry;
}

bool FRunRoomCheckpoint::MakeSaveData(UObject* Run, TArray<uint8>& OutData) const
{
	if (bUseEntry)
	{
		OutData = EntryData;
		return !OutData.IsEmpty();
	}
	return RDCheckpoint::Serialize(Run, OutData);
}

bool FRunRoomCheckpoint::Restore(UObject* Run) const
{
	return !bUseEntry || RDCheckpoint::Deserialize(EntryData, Run);
}

void FRunRoomCheckpoint::Reset()
{
	EntryData.Reset();
	bUseEntry = false;
	bCompletionDeferred = false;
}

void FRunRoomCheckpoint::CompleteCombat(bool bExitPending)
{
	if (bUseEntry && bExitPending) bCompletionDeferred = true;
	else Reset();
}

bool FRunRoomCheckpoint::CancelExit()
{
	if (!bCompletionDeferred) return false;
	// Combat finished while an entry save was in flight, but the player is staying in the room.
	Reset();
	return true;
}
