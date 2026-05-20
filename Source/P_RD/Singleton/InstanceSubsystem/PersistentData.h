/*****************************************************************//**
 * @file   PersistentData.h
 * @brief  영구적 플레이 데이터 구현 헤더
 * @author 모호재
 * @date   2026-05-10
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"
#include "PCGStage/Stage.h"

#include "PersistentData.generated.h"

DECLARE_DELEGATE_OneParam(FOnCreateStage, const FStage& /*NewStage*/)

class APlayerUnit;

/**
 * @brief 한번의 런 동안 기록된 로그 데이터
 */
USTRUCT()
struct FRunLog
{
	GENERATED_BODY()

public:
	void Clear();

public:
	UPROPERTY(Category = Discovery, SaveGame, VisibleAnywhere, meta = (DisplayName = "KilledEnemyUnits"))
	TMap<FPrimaryAssetId, int32> mKilledEnemyUnits;
	UPROPERTY(Category = Discovery, SaveGame, VisibleAnywhere, meta = (DisplayName = "AcquiredSkills"))
	TMap<FPrimaryAssetId, int32> mAcquiredSkills;
	UPROPERTY(Category = Discovery, SaveGame, VisibleAnywhere, meta = (DisplayName = "AcquiredEquipment"))
	TMap<FPrimaryAssetId, int32> mAcquiredEquipment;
	UPROPERTY(Category = Discovery, SaveGame, VisibleAnywhere, meta = (DisplayName = "AcquiredDices"))
	TMap<FPrimaryAssetId, int32> mAcquiredDices;
};

/**
 * @brief 유저의 모든 로그 데이터
 */
USTRUCT()
struct FUserLog
{
	GENERATED_BODY()

public:
	void Clear();

public:
	UPROPERTY(Category = Record, SaveGame, VisibleAnywhere, meta = (DisplayName = "RunCount"))
	int32 mRunCount;

public:
	UPROPERTY(Category = Record, SaveGame, VisibleAnywhere, meta = (DisplayName = "RunCountPerUnit"))
	TMap<FPrimaryAssetId, int32> mRunCountPerUnit;

public:
	UPROPERTY(Category = Discovery, SaveGame, VisibleAnywhere, meta = (DisplayName = "KnownEnemyUnitIds"))
	TSet<FPrimaryAssetId> mKnownEnemyUnitIds;
	UPROPERTY(Category = Discovery, SaveGame, VisibleAnywhere, meta = (DisplayName = "KnownSkillIds"))
	TSet<FPrimaryAssetId> mKnownSkillIds;
	UPROPERTY(Category = Discovery, SaveGame, VisibleAnywhere, meta = (DisplayName = "KnownEquipmentIds"))
	TSet<FPrimaryAssetId> mKnownEquipmentIds;
	UPROPERTY(Category = Discovery, SaveGame, VisibleAnywhere, meta = (DisplayName = "KnownDiceIds"))
	TSet<FPrimaryAssetId> mKnownDiceIds;
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
	const FPrimaryAssetId& GetPlayerUnitId() const;
	int32 GetPlayerLevel() const;
	int32 GetDifficulty() const;

protected:
	void ApplyPersistData(APlayerUnit* PlayerUnit);
	void BindUnitEvent(APlayerUnit* PlayerUnit);

protected:
	UPROPERTY(SaveGame)
	bool mIsNewData = true;

protected:
	UPROPERTY(Category = Player, SaveGame, VisibleAnywhere, meta = (DisplayName = "PlayerUnitId"))
	FPrimaryAssetId mPlayerUnitId;
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
	void StartRun(int32 Difficulty);
	void ClearRun();

	void MakeStageAsync(EStageLevelType Type, FOnCreateStage OnCreateStage);
	void SetCurrentRoomIndex(int32 RowIndex, int32 ColumnIndex);

public:
	const FRandomStream& GetStageBuildStream() const;
	const FRandomStream& GetEventStream() const;

public:
	const FStage& GetStage() const;
	const FRoom& GetRoom(int32 RowIndex, int32 ColumnIndex) const;
	const FRoom& GetStartRoom() const;
	const FRoom& GetCurrentRoom() const;
	void GetCurrentRoomIndex(OUT int32& RowIndex, OUT int32& ColumnIndex) const;

public:
	const FRunLog& GetRunLog() const;

public:
	bool IsActive() const;

protected:
	UPROPERTY(Category = Stream, SaveGame, VisibleAnywhere, meta = (DisplayName = "StageBuildStream"))
	FRandomStream mStageBuildStream;
	UPROPERTY(Category = Stream, SaveGame, VisibleAnywhere, meta = (DisplayName = "EventStream"))
	FRandomStream mEventStream;

protected:
	UPROPERTY(Category = Stage, SaveGame, VisibleAnywhere, meta = (DisplayName = "Stage"))
	TInstancedStruct<FStage> mStage;

protected:
	UPROPERTY(Category = Log, SaveGame, VisibleAnywhere, meta = (DisplayName = "RunLog"))
	FRunLog mRunLog;
};

/**
 * @brief 유저의 영구적 데이터
 */
UCLASS()
class P_RD_API UUserPersistData : public UObject
{
	GENERATED_BODY()

public:
	void MakeUser(const FText& Name);
	void ClearUser();
	void UpdateLog(const FPrimaryAssetId& PlayerUnitId, const FRunLog& RunLog);

public:
	const FText& GetUserName() const;

public:
	const FUserLog& GetUserLog() const;

public:
	bool IsActive() const;

protected:
	UPROPERTY(Category = User, SaveGame, VisibleAnywhere, meta = (DisplayName = "UserName"))
	FText mUserName;

	UPROPERTY(Category = Log, SaveGame, VisibleAnywhere, meta = (DisplayName = "UserLog"))
	FUserLog mUserLog;
};