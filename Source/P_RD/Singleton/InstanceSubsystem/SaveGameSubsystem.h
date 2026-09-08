#pragma once
#include "RDMinimal.h"
#include "Containers/Ticker.h"
#include "SaveGame/SaveCheckpoint.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentDataWriter.h"
#include "SaveGameSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSave, Log, All);
DECLARE_MULTICAST_DELEGATE(FOnSaveStatusChanged);

UCLASS()
class P_RD_API USaveGameSubsystem : public UGameInstanceSubsystem, public IUserDataWriter, public IRunDataWriter, public IOptionDataWriter
{
	GENERATED_BODY()
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;
	bool HasPlaySave() const;
	bool SaveUser() const;
	void SaveUserAsync(FAsyncSaveGameToSlotDelegate Callback) const;
	bool LoadUser() const;
	void LoadUserAsync(FAsyncLoadGameFromSlotDelegate Callback) const;
	void ClearUser() const;
	bool SaveRun() const;
	void SaveRunAsync(FAsyncSaveGameToSlotDelegate Callback) const;
	bool LoadRun() const;
	void LoadRunAsync(FAsyncLoadGameFromSlotDelegate Callback) const;
	void ClearRun() const;
	bool SaveOption() const;
	void SaveOptionAsync(FAsyncSaveGameToSlotDelegate Callback) const;
	bool LoadOption() const;
	void LoadOptionAsync(FAsyncLoadGameFromSlotDelegate Callback) const;
	void ClearOption() const;

	/** Called before party restoration or combat consumes either random stream. */
	bool BeginRoomCheckpoint(bool bIncompleteCombat);
	void CompleteCombatCheckpoint();
	void ResetRunCheckpoint();
	bool SaveEndedRun(const TArray<uint8>& PreviousRunData);
	void CommitPendingRunLogs();
	void RequestRunAutosave() const;
	void RestoreCheckpointOnNextFrontend();
	void CancelCheckpointFrontendRestore();
	bool PrepareFrontend();
	void FlushForBackground();
	bool IsUsingCombatEntryCheckpoint() const { return mRoomCheckpoint.IsCombatEntry(); }
	FText GetRunResumeDescription() const;
	bool HasSaveFailure() const { return !mFailedSlots.IsEmpty(); }
	FText GetSaveStatusText() const;
	mutable FOnSaveStatusChanged OnSaveStatusChanged;

private:
	bool MakePayload(const FString& Slot, UObject* Object, TArray<uint8>& OutData) const;
	bool SaveObject(const FString& Slot, UObject* Object) const;
	void SaveObjectAsync(const FString& Slot, UObject* Object, FAsyncSaveGameToSlotDelegate Callback) const;
	bool LoadObject(const FString& Slot, UObject* Object) const;
	bool ApplyLoadedObject(const FString& Slot, UObject* Object, bool bFound, const TArray<uint8>& Data) const;
	void LoadObjectAsync(const FString& Slot, UObject* Object, FAsyncLoadGameFromSlotDelegate Callback) const;
	void ReportSave(const FString& Slot, uint64 Revision, bool bSuccess) const;
	bool RetryFailedSaves(float DeltaTime);

	FRunRoomCheckpoint mRoomCheckpoint;
	bool mRestoreOnFrontend = false;
	FTSTicker::FDelegateHandle mRetryTicker;
	mutable TMap<FString, uint64> mRevisions;
	mutable TSet<FString> mFailedSlots;
	mutable TSet<FString> mBlockedSlots;
	mutable TSet<FString> mLoadedSlots;
	mutable TMap<FString, TArray<uint8>> mRetryPayloads;
	mutable TSet<FString> mRetryPending;
	static constexpr auto USER_SLOT_NAME = TEXT("User");
	static constexpr auto RUN_SLOT_NAME = TEXT("Run");
	static constexpr auto OPTION_SLOT_NAME = TEXT("Option");
};
