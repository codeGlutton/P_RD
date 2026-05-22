#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "SaveGame/SaveGameArchive.h"

#include "SaveGame/BinarySaveGame.h"

DEFINE_LOG_CATEGORY(LogSave)

void USaveGameSubsystem::SaveUser() const
{
	if (mUserSaveGame == nullptr)
	{
		CreateUser();
	}

	SerializeObject(GetUserMutableData(), OUT mUserSaveGame->mData);
	UGameplayStatics::SaveGameToSlot(mUserSaveGame, USER_SLOT_NAME, 0);
	ClearUser();

	UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 저장"));
}

void USaveGameSubsystem::SaveUserAsync(FAsyncSaveGameToSlotDelegate Callback) const
{
	if (mUserSaveGame == nullptr)
	{
		CreateUser();
	}

	SerializeObject(GetUserMutableData(), OUT mUserSaveGame->mData);
	UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 비동기 저장 시도"));
	UGameplayStatics::AsyncSaveGameToSlot(mUserSaveGame, USER_SLOT_NAME, 0, FAsyncSaveGameToSlotDelegate::CreateLambda([MovedCallback = MoveTemp(Callback)](const FString& SlotName, const int32 UserIndex, bool IsSuccess) {
		UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 비동기 저장 완료"));
		MovedCallback.ExecuteIfBound(SlotName, UserIndex, IsSuccess);
		}));
	ClearUser();
}

void USaveGameSubsystem::LoadUser()
{
	if (UGameplayStatics::DoesSaveGameExist(USER_SLOT_NAME, 0) == false)
	{
		UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 미발견으로 로드 실패"));
		return;
	}

	mUserSaveGame = Cast<UBinarySaveGame>(UGameplayStatics::LoadGameFromSlot(USER_SLOT_NAME, 0));
	DeserializeObject(mUserSaveGame->mData, OUT GetUserMutableData());
	ClearUser();

	UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 로드"));
}

void USaveGameSubsystem::LoadUserAsync(FAsyncLoadGameFromSlotDelegate Callback) const
{
	UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 비동기 로드 시도"));
	UGameplayStatics::AsyncLoadGameFromSlot(USER_SLOT_NAME, 0, FAsyncLoadGameFromSlotDelegate::CreateLambda([this, MovedCallback = MoveTemp(Callback)](const FString& SlotName, const int32 UserIndex, USaveGame* LoadedSaveGame) {
		
		if (LoadedSaveGame == nullptr)
		{
			UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 미발견으로 로드 실패"));
			return;
		}

		mUserSaveGame = Cast<UBinarySaveGame>(LoadedSaveGame);
		DeserializeObject(mUserSaveGame->mData, OUT GetUserMutableData());
		ClearUser();

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

	SerializeObject(GetRunMutableData(), OUT mRunSaveGame->mData);
	UGameplayStatics::SaveGameToSlot(mRunSaveGame, RUN_SLOT_NAME, 0);
	ClearRun();

	UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 저장"));
}

void USaveGameSubsystem::SaveRunAsync(FAsyncSaveGameToSlotDelegate Callback) const
{
	if (mUserSaveGame == nullptr)
	{
		CreateRun();
	}

	SerializeObject(GetRunMutableData(), OUT mRunSaveGame->mData);
	UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 비동기 저장 시도"));
	UGameplayStatics::AsyncSaveGameToSlot(mRunSaveGame, RUN_SLOT_NAME, 0, FAsyncSaveGameToSlotDelegate::CreateLambda([MovedCallback = MoveTemp(Callback)](const FString& SlotName, const int32 UserIndex, bool IsSuccess) {
		UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 비동기 저장 완료"));
		MovedCallback.ExecuteIfBound(SlotName, UserIndex, IsSuccess);
		}));
	ClearRun();
}

void USaveGameSubsystem::LoadRun()
{
	if (UGameplayStatics::DoesSaveGameExist(RUN_SLOT_NAME, 0) == false)
	{
		return;
	}

	mRunSaveGame = Cast<UBinarySaveGame>(UGameplayStatics::LoadGameFromSlot(RUN_SLOT_NAME, 0));
	DeserializeObject(mRunSaveGame->mData, OUT GetRunMutableData());
	ClearRun();

	UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 로드"));
}

void USaveGameSubsystem::LoadRunAsync(FAsyncLoadGameFromSlotDelegate Callback) const
{
	UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 비동기 로드 시도"));
	UGameplayStatics::AsyncLoadGameFromSlot(RUN_SLOT_NAME, 0, FAsyncLoadGameFromSlotDelegate::CreateLambda([this, MovedCallback = MoveTemp(Callback)](const FString& SlotName, const int32 UserIndex, USaveGame* LoadedSaveGame) {

		if (LoadedSaveGame == nullptr)
		{
			UE_LOG(LogSave, Log, TEXT("런 데이터 세이브 파일 미발견으로 로드 실패"));
			return;
		}

		mRunSaveGame = Cast<UBinarySaveGame>(LoadedSaveGame);
		DeserializeObject(mRunSaveGame->mData, OUT GetRunMutableData());
		ClearRun();

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
	mUserSaveGame = Cast <UBinarySaveGame>(UGameplayStatics::CreateSaveGameObject(UBinarySaveGame::StaticClass()));
	checkf(mUserSaveGame != nullptr, TEXT("유저 데이터 저장 객체 생성 실패"));
	UE_LOG(LogSave, Log, TEXT("유저 데이터 세이브 파일 생성"));
}

void USaveGameSubsystem::CreateRun() const
{
	mRunSaveGame = Cast <UBinarySaveGame>(UGameplayStatics::CreateSaveGameObject(UBinarySaveGame::StaticClass()));
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
