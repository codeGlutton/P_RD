#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"

#include "SaveGame/SaveGameArchive.h"

#include "SaveGame/UserSaveGame.h"
#include "SaveGame/RunSaveGame.h"

DEFINE_LOG_CATEGORY(LogSave)

void USaveGameSubsystem::SaveUser() const
{
	if (mUserSaveGame == nullptr)
	{
		CreateUser();
	}

	ClearUser();

	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));
	SerializeObject(PersistentDataSubsystem->GetUserPersistData(), OUT mUserSaveGame->mData);

	UGameplayStatics::SaveGameToSlot(mUserSaveGame, TEXT("User"), 0);

	UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 저장"));
}

void USaveGameSubsystem::SaveUserAsync(FAsyncSaveGameToSlotDelegate Callback) const
{
	if (mUserSaveGame == nullptr)
	{
		CreateUser();
	}

	ClearUser();

	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));
	SerializeObject(PersistentDataSubsystem->GetUserPersistData(), OUT mUserSaveGame->mData);

	UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 비동기 저장 시도"));
	UGameplayStatics::AsyncSaveGameToSlot(mUserSaveGame, TEXT("User"), 0, FAsyncSaveGameToSlotDelegate::CreateLambda([MovedCallback = MoveTemp(Callback)](const FString& SlotName, const int32 UserIndex, bool IsSuccess) {
		UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 비동기 저장 완료"));
		MovedCallback.ExecuteIfBound(SlotName, UserIndex, IsSuccess);
		}));
}

void USaveGameSubsystem::LoadUser()
{
	if (UGameplayStatics::DoesSaveGameExist(TEXT("User"), 0) == false)
	{
		UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 미발견으로 로드 실패"));
		return;
	}

	mUserSaveGame = Cast<UUserSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("User"), 0));

	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));
	DeserializeObject(mUserSaveGame->mData, OUT PersistentDataSubsystem->GetUserPersistData());

	UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 로드"));
}

void USaveGameSubsystem::LoadUserAsync(FAsyncLoadGameFromSlotDelegate Callback) const
{
	UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 비동기 로드 시도"));
	UGameplayStatics::AsyncLoadGameFromSlot(TEXT("User"), 0, FAsyncLoadGameFromSlotDelegate::CreateLambda([this, MovedCallback = MoveTemp(Callback)](const FString& SlotName, const int32 UserIndex, USaveGame* LoadedSaveGame) {
		
		if (LoadedSaveGame == nullptr)
		{
			UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 미발견으로 로드 실패"));
			return;
		}

		mUserSaveGame = Cast<UUserSaveGame>(LoadedSaveGame);
		UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
		checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));
		DeserializeObject(mUserSaveGame->mData, OUT PersistentDataSubsystem->GetUserPersistData());

		UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 비동기 로드 완료"));
		MovedCallback.ExecuteIfBound(SlotName, UserIndex, LoadedSaveGame);
		}));
}

void USaveGameSubsystem::ClearUser() const
{
	if (mUserSaveGame == nullptr)
	{
		CreateUser();
	}

	mUserSaveGame->mData.Empty();
	UE_LOG(LogSave, Log, TEXT("유저 데이터 초기화"));
}

void USaveGameSubsystem::SaveRun() const
{
	if (mRunSaveGame == nullptr)
	{
		CreateRun();
	}

	ClearRun();

	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));
	SerializeObject(PersistentDataSubsystem->GetRunPersistData(), OUT mRunSaveGame->mData);

	UGameplayStatics::SaveGameToSlot(mRunSaveGame, TEXT("Run"), 0);

	UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 저장"));
}

void USaveGameSubsystem::SaveRunAsync(FAsyncSaveGameToSlotDelegate Callback) const
{
	if (mUserSaveGame == nullptr)
	{
		CreateRun();
	}

	ClearRun();

	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));
	SerializeObject(PersistentDataSubsystem->GetRunPersistData(), OUT mRunSaveGame->mData);

	UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 비동기 저장 시도"));
	UGameplayStatics::AsyncSaveGameToSlot(mRunSaveGame, TEXT("Run"), 0, FAsyncSaveGameToSlotDelegate::CreateLambda([MovedCallback = MoveTemp(Callback)](const FString& SlotName, const int32 UserIndex, bool IsSuccess) {
		UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 비동기 저장 완료"));
		MovedCallback.ExecuteIfBound(SlotName, UserIndex, IsSuccess);
		}));
}

void USaveGameSubsystem::LoadRun()
{
	if (UGameplayStatics::DoesSaveGameExist(TEXT("Run"), 0) == false)
	{
		return;
	}

	mRunSaveGame = Cast<URunSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("Run"), 0));

	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));
	DeserializeObject(mRunSaveGame->mData, OUT PersistentDataSubsystem->GetRunPersistData());

	UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 로드"));
}

void USaveGameSubsystem::LoadRunAsync(FAsyncLoadGameFromSlotDelegate Callback) const
{
	UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 비동기 로드 시도"));
	UGameplayStatics::AsyncLoadGameFromSlot(TEXT("Run"), 0, FAsyncLoadGameFromSlotDelegate::CreateLambda([this, MovedCallback = MoveTemp(Callback)](const FString& SlotName, const int32 UserIndex, USaveGame* LoadedSaveGame) {

		if (LoadedSaveGame == nullptr)
		{
			UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 미발견으로 로드 실패"));
			return;
		}

		mRunSaveGame = Cast<URunSaveGame>(LoadedSaveGame);
		UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
		checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));
		DeserializeObject(mRunSaveGame->mData, OUT PersistentDataSubsystem->GetRunPersistData());

		UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 비동기 로드 완료"));
		MovedCallback.ExecuteIfBound(SlotName, UserIndex, LoadedSaveGame);
		}));
}

void USaveGameSubsystem::ClearRun() const
{
	if (mRunSaveGame == nullptr)
	{
		CreateRun();
	}

	mRunSaveGame->mData.Empty();
	UE_LOG(LogSave, Log, TEXT("런 데이터 초기화"));
}

void USaveGameSubsystem::CreateUser() const
{
	mUserSaveGame = Cast <UUserSaveGame>(UGameplayStatics::CreateSaveGameObject(UUserSaveGame::StaticClass()));
	checkf(mUserSaveGame != nullptr, TEXT("유저 데이터 저장 객체 생성 실패"));
	UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 생성"));
}

void USaveGameSubsystem::CreateRun() const
{
	mRunSaveGame = Cast <URunSaveGame>(UGameplayStatics::CreateSaveGameObject(URunSaveGame::StaticClass()));
	checkf(mRunSaveGame != nullptr, TEXT("런 데이터 저장 객체 생성 실패"));
	UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 생성"));
}

void USaveGameSubsystem::SerializeObject(UObject* Object, OUT TArray<uint8>& Data) const
{
	checkf(Object != nullptr, TEXT("오브젝트 nullptr로, 직렬화 실패"));

	FMemoryWriter Writer = FMemoryWriter(OUT Data, true);
	FSaveGameArchive Archive = FSaveGameArchive(Writer);

	Object->Serialize(Archive);
}

void USaveGameSubsystem::DeserializeObject(const TArray<uint8>& Data, OUT UObject* Object) const
{
	checkf(Object != nullptr, TEXT("오브젝트 nullptr로, 역직렬화 실패"));

	FMemoryReader Reader = FMemoryReader(Data, true);
	FSaveGameArchive Archive = FSaveGameArchive(Reader);

	Object->Serialize(Archive);
}
