/*****************************************************************//**
 * @file   PersistentDataSubsystem.h
 * @brief  영구적 플레이 데이터 Subsystem 구현 헤더
 * @author 모호재
 * @date   2026-05-10
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PersistentDataSubsystem.generated.h"

class UUserPersistData;
class URunPersistData;

/**
 * @brief  영구적 플레이 데이터 Subsystem
 */
UCLASS()
class P_RD_API UPersistentDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	friend class IUserDataWriter;
	friend class IRunDataWriter;

	/* UGameInstanceSubsystem 상속 */
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	const UUserPersistData* GetUserPersistData() const;
	const URunPersistData* GetRunPersistData() const;

public:
	// TODO : 삭제
	UFUNCTION(BlueprintCallable, CallInEditor)
	void DoStageBuildTest(bool UpdateBuildStream);

protected:
	UPROPERTY(Category = User, VisibleAnywhere, meta = (DisplayName = "UserPersistData"))
	TObjectPtr<UUserPersistData> mUserPersistData;
	UPROPERTY(Category = Run, VisibleAnywhere, meta = (DisplayName = "RunPersistData"))
	TObjectPtr<URunPersistData> mRunPersistData;
};
