#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "SaveGame/BinarySaveGame.h"
#include "Misc/CoreDelegates.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY(LogSave);
#define LOCTEXT_NAMESPACE "RunSave"

void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UPersistentDataSubsystem>();
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &USaveGameSubsystem::FlushForBackground);
	FCoreDelegates::GetApplicationWillTerminateDelegate().AddUObject(this, &USaveGameSubsystem::FlushForBackground);
	mRetryTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &USaveGameSubsystem::RetryFailedSaves), 5.f);
}

void USaveGameSubsystem::Deinitialize()
{
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);
	FCoreDelegates::GetApplicationWillTerminateDelegate().RemoveAll(this);
	FTSTicker::GetCoreTicker().RemoveTicker(mRetryTicker);
	Super::Deinitialize();
}

bool USaveGameSubsystem::HasPlaySave() const
{
	return RDCheckpoint::DoesSlotExist(USER_SLOT_NAME) || RDCheckpoint::DoesSlotExist(RUN_SLOT_NAME);
}

bool USaveGameSubsystem::MakePayload(const FString& Slot, UObject* Object, TArray<uint8>& OutData) const
{
	return Slot == RUN_SLOT_NAME ? mRoomCheckpoint.MakeSaveData(Object, OutData) : RDCheckpoint::Serialize(Object, OutData);
}

void USaveGameSubsystem::ReportSave(const FString& Slot, uint64 Revision, const bool bSuccess) const
{
	if (mRevisions.FindRef(Slot) != Revision) return;
	if (bSuccess)
	{
		mFailedSlots.Remove(Slot);
		mRetryPayloads.Remove(Slot);
	}
	else
	{
		mFailedSlots.Add(Slot);
		UE_LOG(LogSave, Warning, TEXT("Save failed for %s; last valid checkpoint retained, retry scheduled."), *Slot);
	}
	OnSaveStatusChanged.Broadcast();
}

bool USaveGameSubsystem::SaveObject(const FString& Slot, UObject* Object) const
{
	const uint64 Revision = ++mRevisions.FindOrAdd(Slot);
	TArray<uint8> Data;
	const bool bPrepared = !mBlockedSlots.Contains(Slot) && MakePayload(Slot, Object, Data);
	if (bPrepared) mRetryPayloads.Add(Slot, Data);
	const bool bSaved = bPrepared && RDCheckpoint::SaveSlot(Slot, Data);
	ReportSave(Slot, Revision, bSaved);
	return bSaved;
}

void USaveGameSubsystem::SaveObjectAsync(const FString& Slot, UObject* Object, FAsyncSaveGameToSlotDelegate Callback) const
{
	const uint64 Revision = ++mRevisions.FindOrAdd(Slot);
	TArray<uint8> Data;
	if (mBlockedSlots.Contains(Slot) || !MakePayload(Slot, Object, Data))
	{
		ReportSave(Slot, Revision, false);
		Callback.ExecuteIfBound(Slot, 0, false);
		return;
	}
	mRetryPayloads.Add(Slot, Data);
	TWeakObjectPtr<const USaveGameSubsystem> WeakThis(this);
	RDCheckpoint::SaveSlotAsync(Slot, MoveTemp(Data), [WeakThis, Slot, Revision, Callback = MoveTemp(Callback)](bool bSaved) mutable
	{
		if (const USaveGameSubsystem* Self = WeakThis.Get())
		{
			Self->ReportSave(Slot, Revision, bSaved);
			Callback.ExecuteIfBound(Slot, 0, bSaved);
		}
	});
}

bool USaveGameSubsystem::LoadObject(const FString& Slot, UObject* Object) const
{
	TArray<uint8> Data;
	const bool bFound = RDCheckpoint::LoadSlot(Slot, Data);
	return ApplyLoadedObject(Slot, Object, bFound, Data);
}

bool USaveGameSubsystem::ApplyLoadedObject(const FString& Slot, UObject* Object, bool bFound, const TArray<uint8>& Data) const
{
	mLoadedSlots.Add(Slot);
	if (bFound)
	{
		// Failed migrations must not partly change live progress.
		TStrongObjectPtr<UObject> Candidate(NewObject<UObject>(GetTransientPackage(), Object->GetClass()));
		if (RDCheckpoint::Deserialize(Data, Candidate.Get()) && RDCheckpoint::Deserialize(Data, Object))
		{
			mBlockedSlots.Remove(Slot);
			mFailedSlots.Remove(Slot);
			return true;
		}
	}
	if (RDCheckpoint::DoesSlotExist(Slot))
	{
		// Never replace an unreadable installation with blank frontend autosaves.
		mBlockedSlots.Add(Slot);
		mFailedSlots.Add(Slot);
		UE_LOG(LogSave, Warning, TEXT("No valid checkpoint could be loaded for %s; existing files preserved."), *Slot);
		OnSaveStatusChanged.Broadcast();
	}
	return false;
}

void USaveGameSubsystem::LoadObjectAsync(const FString& Slot, UObject* Object, FAsyncLoadGameFromSlotDelegate Callback) const
{
	TWeakObjectPtr<const USaveGameSubsystem> WeakThis(this);
	TWeakObjectPtr<UObject> WeakObject(Object);
	RDCheckpoint::LoadSlotAsync(Slot, [WeakThis, WeakObject, Slot, Callback = MoveTemp(Callback)](bool bFound, TArray<uint8> Data) mutable
	{
		const USaveGameSubsystem* Self = WeakThis.Get();
		UObject* Target = WeakObject.Get();
		if (!Self || !Target) return;
		const bool bLoaded = Self->ApplyLoadedObject(Slot, Target, bFound, Data);
		TStrongObjectPtr<UBinarySaveGame> Loaded(bLoaded ? NewObject<UBinarySaveGame>() : nullptr);
		if (bLoaded) Loaded->mData = MoveTemp(Data);
		Callback.ExecuteIfBound(Slot, 0, Loaded.Get());
	});
}

bool USaveGameSubsystem::SaveUser() const { return SaveObject(USER_SLOT_NAME, GetUserMutableData()); }
void USaveGameSubsystem::SaveUserAsync(FAsyncSaveGameToSlotDelegate Callback) const { SaveObjectAsync(USER_SLOT_NAME, GetUserMutableData(), MoveTemp(Callback)); }
bool USaveGameSubsystem::LoadUser() const { return LoadObject(USER_SLOT_NAME, GetUserMutableData()); }
void USaveGameSubsystem::LoadUserAsync(FAsyncLoadGameFromSlotDelegate Callback) const { LoadObjectAsync(USER_SLOT_NAME, GetUserMutableData(), MoveTemp(Callback)); }
void USaveGameSubsystem::ClearUser() const {}
bool USaveGameSubsystem::SaveRun() const { return SaveObject(RUN_SLOT_NAME, GetRunMutableData()); }
void USaveGameSubsystem::SaveRunAsync(FAsyncSaveGameToSlotDelegate Callback) const { SaveObjectAsync(RUN_SLOT_NAME, GetRunMutableData(), MoveTemp(Callback)); }
bool USaveGameSubsystem::LoadRun() const { return LoadObject(RUN_SLOT_NAME, GetRunMutableData()); }
void USaveGameSubsystem::LoadRunAsync(FAsyncLoadGameFromSlotDelegate Callback) const { LoadObjectAsync(RUN_SLOT_NAME, GetRunMutableData(), MoveTemp(Callback)); }
void USaveGameSubsystem::ClearRun() const {}
bool USaveGameSubsystem::SaveOption() const { return SaveObject(OPTION_SLOT_NAME, GetOptionMutableData()); }
void USaveGameSubsystem::SaveOptionAsync(FAsyncSaveGameToSlotDelegate Callback) const { SaveObjectAsync(OPTION_SLOT_NAME, GetOptionMutableData(), MoveTemp(Callback)); }
bool USaveGameSubsystem::LoadOption() const
{
	const bool bLoaded = LoadObject(OPTION_SLOT_NAME, GetOptionMutableData());
	GetOptionMutableData()->ApplyCurrentOptions();
	return bLoaded;
}
void USaveGameSubsystem::LoadOptionAsync(FAsyncLoadGameFromSlotDelegate Callback) const
{
	LoadObjectAsync(OPTION_SLOT_NAME, GetOptionMutableData(), FAsyncLoadGameFromSlotDelegate::CreateWeakLambda(this,
		[this, Callback = MoveTemp(Callback)](const FString& Slot, int32 Index, USaveGame* Save) mutable
		{
			GetOptionMutableData()->ApplyCurrentOptions();
			Callback.ExecuteIfBound(Slot, Index, Save);
		}));
}
void USaveGameSubsystem::ClearOption() const {}

bool USaveGameSubsystem::BeginRoomCheckpoint(const bool bIncompleteCombat)
{
	mRestoreOnFrontend = false;
	return mRoomCheckpoint.Capture(GetRunMutableData(), bIncompleteCombat);
}
void USaveGameSubsystem::CompleteCombatCheckpoint() { mRoomCheckpoint.CompleteCombat(mRestoreOnFrontend); }
void USaveGameSubsystem::ResetRunCheckpoint() { mRoomCheckpoint.Reset(); mRestoreOnFrontend = false; }
bool USaveGameSubsystem::SaveEndedRun(const TArray<uint8>& PreviousRunData)
{
	FRunRoomCheckpoint PreviousCheckpoint = mRoomCheckpoint;
	ResetRunCheckpoint();
	if (SaveRun()) return true;
	// The caller rolls back RAM. Do not retry a rejected abandonment later in the background.
	mRoomCheckpoint = MoveTemp(PreviousCheckpoint);
	++mRevisions.FindOrAdd(RUN_SLOT_NAME);
	TArray<uint8> RetryData = PreviousRunData;
	if (mRoomCheckpoint.IsCombatEntry()) mRoomCheckpoint.MakeSaveData(nullptr, RetryData);
	mRetryPayloads.Add(RUN_SLOT_NAME, MoveTemp(RetryData));
	return false;
}
void USaveGameSubsystem::RequestRunAutosave() const { SaveRunAsync(FAsyncSaveGameToSlotDelegate()); }
void USaveGameSubsystem::CommitPendingRunLogs()
{
	URunPersistData* Run = GetRunMutableData();
	if (Run->GetPendingCompletedRuns().IsEmpty() || mBlockedSlots.Contains(USER_SLOT_NAME)) return;
	UUserPersistData* User = GetUserMutableData();
	for (const FPendingCompletedRun& Pending : Run->GetPendingCompletedRuns())
	{
		User->ApplyRunLogOnce(Pending.TransactionId, Pending.Log);
	}
	// Keep the journal across future runs when User cannot be written. Replaying is idempotent.
	if (SaveUser())
	{
		Run->ClearCompletedRunLogs();
		RequestRunAutosave();
	}
}
void USaveGameSubsystem::RestoreCheckpointOnNextFrontend() { mRestoreOnFrontend = mRoomCheckpoint.IsCombatEntry(); }
void USaveGameSubsystem::CancelCheckpointFrontendRestore()
{
	mRestoreOnFrontend = false;
	if (mRoomCheckpoint.CancelExit()) RequestRunAutosave();
}
bool USaveGameSubsystem::PrepareFrontend()
{
	if (mRestoreOnFrontend && !mRoomCheckpoint.Restore(GetRunMutableData())) return false;
	ResetRunCheckpoint();
	CommitPendingRunLogs();
	return true;
}

void USaveGameSubsystem::FlushForBackground()
{
	if (!IsInGameThread() || mLoadedSlots.Num() < 3) return;
	// These writes wait behind older requests before the lifecycle callback returns.
	SaveRun();
	SaveUser();
	SaveOption();
	CommitPendingRunLogs();
}

bool USaveGameSubsystem::RetryFailedSaves(float DeltaTime)
{
	const TSet<FString> Failed = mFailedSlots;
	for (const FString& Slot : Failed)
	{
		const TArray<uint8>* Payload = mRetryPayloads.Find(Slot);
		if (!Payload || mBlockedSlots.Contains(Slot) || mRetryPending.Contains(Slot)) continue;
		const uint64 Revision = mRevisions.FindRef(Slot);
		mRetryPending.Add(Slot);
		TWeakObjectPtr<USaveGameSubsystem> WeakThis(this);
		RDCheckpoint::SaveSlotAsync(Slot, *Payload, [WeakThis, Slot, Revision](bool bSaved)
		{
			if (USaveGameSubsystem* Self = WeakThis.Get())
			{
				Self->mRetryPending.Remove(Slot);
				Self->ReportSave(Slot, Revision, bSaved);
			}
		});
	}
	return true;
}

FText USaveGameSubsystem::GetRunResumeDescription() const
{
	return IsUsingCombatEntryCheckpoint()
		? LOCTEXT("RestartCombat", "현재 전투는 방 입장 시점부터 다시 시작하며, 아군 상태와 소지품도 함께 복원됩니다.")
		: LOCTEXT("ResumeProgress", "현재 진행을 저장한 뒤 타이틀로 돌아갑니다.");
}

FText USaveGameSubsystem::GetSaveStatusText() const
{
	if (!mBlockedSlots.IsEmpty()) return LOCTEXT("UnreadableSave", "저장 데이터를 읽을 수 없습니다. 기존 파일은 보존되며 새 저장은 중단되었습니다.");
	return HasSaveFailure() ? LOCTEXT("SaveFailed", "저장하지 못했습니다. 자동으로 다시 시도합니다.") : FText::GetEmpty();
}
#undef LOCTEXT_NAMESPACE
