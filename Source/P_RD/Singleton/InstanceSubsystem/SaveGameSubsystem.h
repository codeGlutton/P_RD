/*****************************************************************//**
 * @file   SaveGameSubsystem.h
 * @brief  게임 저장을 위한 Subsystem 구현 헤더
 * @author 모호재
 * @date   2026-05-10
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentDataWriter.h"

#include "SaveGameSubsystem.generated.h"

// USaveGameSubsystem 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogSave, Log, All)

class UBinarySaveGame;

/**
 * @brief  게임 저장을 위한 Subsystem
 */
UCLASS()
class P_RD_API USaveGameSubsystem : public UGameInstanceSubsystem, public IUserDataWriter, public IRunDataWriter
{
	GENERATED_BODY()

public:
	bool SaveUser() const;
	void SaveUserAsync(FAsyncSaveGameToSlotDelegate Callback) const;
	bool LoadUser() const;
	void LoadUserAsync(FAsyncLoadGameFromSlotDelegate Callback) const;
	void ClearUser() const;

public:
	bool SaveRun() const;
	void SaveRunAsync(FAsyncSaveGameToSlotDelegate Callback) const;
	bool LoadRun() const;
	void LoadRunAsync(FAsyncLoadGameFromSlotDelegate Callback) const;
	void ClearRun() const;

protected:
	void CreateUser() const;
	void CreateRun() const;

protected:
	void SerializeObject(UObject* Object, OUT TArray<uint8>& Data) const;
	void DeserializeObject(const TArray<uint8>& Data, OUT UObject* Object) const;

protected:
	UPROPERTY()
	mutable TObjectPtr<UBinarySaveGame> mUserSaveGame;

	UPROPERTY()
	mutable TObjectPtr<UBinarySaveGame> mRunSaveGame;

protected:
	static constexpr auto USER_SLOT_NAME = TEXT("User");
	static constexpr auto RUN_SLOT_NAME = TEXT("Run");
};
