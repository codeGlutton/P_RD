/*****************************************************************//**
 * @file   SaveGameSubsystem.h
 * @brief  게임 저장을 위한 Subsystem 구현 헤더
 * @author 모호재
 * @date   2026-05-10
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "SaveGameSubsystem.generated.h"

// USaveGameSubsystem 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogSave, Log, All)

class UUserSaveGame;
class URunSaveGame;

/**
 * @brief  게임 저장을 위한 Subsystem
 */
UCLASS()
class P_RD_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void SaveUser() const;
	void LoadUser();
	void ClearUser() const;

public:
	void SaveRun() const;
	void LoadRun();
	void ClearRun() const;

protected:
	void CreateUser() const;
	void CreateRun() const;

protected:
	void SerializeObject(UObject* Object, OUT TArray<uint8>& Data) const;
	void DeserializeObject(const TArray<uint8>& Data, OUT UObject* Object) const;

protected:
	UPROPERTY()
	mutable TObjectPtr<UUserSaveGame> mUserSaveGame;

	UPROPERTY()
	mutable TObjectPtr<URunSaveGame> mRunSaveGame;
};
