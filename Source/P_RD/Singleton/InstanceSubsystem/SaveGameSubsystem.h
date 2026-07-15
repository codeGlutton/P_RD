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
class P_RD_API USaveGameSubsystem : public UGameInstanceSubsystem, public IUserDataWriter, public IRunDataWriter, public IOptionDataWriter
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
	/** @brief 디스크의 런 저장 슬롯과 메모리 직렬화 버퍼를 함께 제거한다. */
	bool DeleteRunSave() const;
	/** @brief 비동기 런 저장이 아직 파일 작업 중인지 여부 */
	bool IsRunSaveInProgress() const { return mPendingRunSaveCount > 0; }

public:
	bool SaveOption() const;
	void SaveOptionAsync(FAsyncSaveGameToSlotDelegate Callback) const;
	bool LoadOption() const;
	void LoadOptionAsync(FAsyncLoadGameFromSlotDelegate Callback) const;
	void ClearOption() const;

protected:
	void CreateUser() const;
	void CreateRun() const;
	void CreateOption() const;

protected:
	void SerializeObject(UObject* Object, OUT TArray<uint8>& Data) const;
	void DeserializeObject(const TArray<uint8>& Data, OUT UObject* Object) const;

protected:
	UPROPERTY()
	mutable TObjectPtr<UBinarySaveGame> mUserSaveGame;

	UPROPERTY()
	mutable TObjectPtr<UBinarySaveGame> mRunSaveGame;

	UPROPERTY()
	mutable TObjectPtr<UBinarySaveGame> mOptionSaveGame;

	/** @brief 완료되지 않은 비동기 런 저장 수. 저장 중 포기/삭제 경합을 막는다. */
	mutable int32 mPendingRunSaveCount = 0;

protected:
	static constexpr auto USER_SLOT_NAME = TEXT("User");
	static constexpr auto RUN_SLOT_NAME = TEXT("Run");
	static constexpr auto OPTION_SLOT_NAME = TEXT("Option");
};
