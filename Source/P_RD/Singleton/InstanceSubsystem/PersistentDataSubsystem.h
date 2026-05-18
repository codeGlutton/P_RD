/*****************************************************************//**
 * @file   PersistentDataSubsystem.h
 * @brief  영구적 플레이 데이터 Subsystem 구현 헤더
 * @author 모호재
 * @date   2026-05-10
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PCGStage/Stage.h"

#include "PersistentDataSubsystem.generated.h"

DECLARE_DELEGATE_OneParam(FOnCreateStage, const FStage& /*NewStage*/)

class APlayerUnit;

/**
 * @brief 유저의 영구적 데이터
 */
UCLASS()
class P_RD_API UUserPersistData : public UObject
{
	GENERATED_BODY()

public:
	void MakeNewUser(FName Name);

public:
	const FName& GetUserName() const;
	int32 GetRunCount() const;

protected:
	UPROPERTY(Category = User, SaveGame, VisibleAnywhere, meta = (DisplayName = "UserName"))
	FName mUserName;
	UPROPERTY(Category = User, SaveGame, VisibleAnywhere, meta = (DisplayName = "RunCount"))
	int32 mRunCount;
};

/**
 * @brief 플레이어 유닛의 영구적 데이터
 */
UCLASS()
class P_RD_API UPlayerUnitPersistData : public UObject
{
	GENERATED_BODY()

public:
	void RegisterUnit(APlayerUnit* PlayerUnit);

public:
	int32 GetPlayerLevel() const;
	int32 GetDifficulty() const;

protected:
	void ApplyPersistData(APlayerUnit* PlayerUnit);
	void BindUnitEvent(APlayerUnit* PlayerUnit);

protected:
	UPROPERTY(SaveGame)
	bool mIsNewData = true;

protected:
	UPROPERTY(Category = Player, SaveGame, VisibleAnywhere, meta = (DisplayName = "PlayerLevel"))
	int32 mPlayerLevel;
	UPROPERTY(Category = Player, SaveGame, VisibleAnywhere, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty;

protected:
	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "MaxHP"))
	float mMaxHP;
	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "HP"))
	float mHP;

	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "Exp"))
	float mExp;

	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "Money"))
	float mMoney;

protected:
	UPROPERTY(Category = Tag, SaveGame, VisibleAnywhere, meta = (DisplayName = "TagCountMap"))
	TMap<FGameplayTag, int32> mTagCountMap;

protected:
	UPROPERTY(Category = Equipment, SaveGame, VisibleAnywhere, meta = (DisplayName = "EquipmentIds"))
	TArray<FPrimaryAssetId> mEquipmentIds;
};

/**
 * @brief 이번 런의 영구적 데이터
 */
UCLASS()
class P_RD_API URunPersistData : public UPlayerUnitPersistData
{
	GENERATED_BODY()

public:
	void MakeNewRun(int32 Difficulty);
	void MakeStageAsync(EStageLevelType Type, FOnCreateStage OnCreateStage);

public:
	const FRandomStream& GetStageBuildStream() const;
	const FRandomStream& GetEventStream() const;

public:
	const FStage& GetStage() const;

protected:
	void ClearRunData();

protected:
	UPROPERTY(Category = Stream, SaveGame, VisibleAnywhere, meta = (DisplayName = "StageBuildStream"))
	FRandomStream mStageBuildStream;
	UPROPERTY(Category = Stream, SaveGame, VisibleAnywhere, meta = (DisplayName = "EventStream"))
	FRandomStream mEventStream;

protected:
	UPROPERTY(Category = Stage, SaveGame, VisibleAnywhere, meta = (DisplayName = "Stage"))
	FStage mStage;
};

/**
 * @brief  영구적 플레이 데이터 Subsystem
 */
UCLASS()
class P_RD_API UPersistentDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/* UGameInstanceSubsystem 상속 */
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	UUserPersistData* GetUserPersistData();
	URunPersistData* GetRunPersistData();

protected:
	UPROPERTY(Category = User, VisibleAnywhere, meta = (DisplayName = "UserPersistData"))
	TObjectPtr<UUserPersistData> mUserPersistData;
	UPROPERTY(Category = Run, VisibleAnywhere, meta = (DisplayName = "RunPersistData"))
	TObjectPtr<URunPersistData> mRunPersistData;
};
