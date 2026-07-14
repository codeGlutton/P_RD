/*****************************************************************//**
 * @file   PersistentData.h
 * @brief  영구적 플레이 데이터 구현 헤더
 * @author 모호재
 * @date   2026-05-10
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "PCGStage/Stage.h"
#include "Singleton/InstanceSubsystem/PersistentDataType.h"

#include "PersistentData.generated.h"

class UPlayerUnitModel;
class FViewport;

DECLARE_DELEGATE_OneParam(FOnCreateStage, const FStage& /*NewStage*/)

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
	int32 mRunCount = 0;

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
	void RegisterPlayerUnit(UPlayerUnitModel* PlayerUnit);

public:
	const FPrimaryAssetId& GetPlayerUnitId() const;
	int32 GetPlayerLevel() const;
	int32 GetDifficulty() const;

public:
	const TArray<FPrimaryAssetId>& GetSkillIds() const;
	const TArray<FPrimaryAssetId>& GetEquipmentIds() const;
	const TArray<FPrimaryAssetId>& GetDiceIds() const;

protected:
	void SyncPlayerPersistData(UPlayerUnitModel* PlayerUnit);
	void BindPlayerUnitEvent(UPlayerUnitModel* PlayerUnit);

protected:
	UPROPERTY(SaveGame)
	bool mIsNewData = true;

protected:
	UPROPERTY(Category = Player, SaveGame, VisibleAnywhere, meta = (DisplayName = "PlayerUnitId"))
	FPrimaryAssetId mPlayerUnitId;
	UPROPERTY(Category = Player, SaveGame, VisibleAnywhere, meta = (DisplayName = "PlayerLevel"))
	int32 mPlayerLevel = 1;
	UPROPERTY(Category = Player, SaveGame, VisibleAnywhere, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty = 1;

protected:
	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "MaxHP"))
	float mMaxHP = 0.f;
	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "HP"))
	float mHP = 0.f;

	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "Exp"))
	float mExp = 0.f;

	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "Money"))
	float mMoney = 0.f;

protected:
	UPROPERTY(Category = Tag, SaveGame, VisibleAnywhere, meta = (DisplayName = "TagCountMap"))
	TMap<FGameplayTag, int32> mTagCountMap;

protected:
	UPROPERTY(Category = Skill, SaveGame, VisibleAnywhere, meta = (DisplayName = "SkillIds"))
	TArray<FPrimaryAssetId> mSkillIds;

	UPROPERTY(Category = Equipment, SaveGame, VisibleAnywhere, meta = (DisplayName = "EquipmentIds"))
	TArray<FPrimaryAssetId> mEquipmentIds;

	UPROPERTY(Category = Dice, SaveGame, VisibleAnywhere, meta = (DisplayName = "DiceIds"))
	TArray<FPrimaryAssetId> mDiceIds;
};

/**
 * @brief 이번 런의 영구적 데이터
 */
UCLASS()
class P_RD_API URunPersistData : public UPlayerUnitPersistData
{
	GENERATED_BODY()

public:
	void StartRun(const FPrimaryAssetId& PlayerUnitId, int32 Difficulty);
	void ClearRun();
	void SetTutorialEnabled(bool bEnabled);
	bool ShouldRunTutorial() const;

public:
	void MakeStageAsync(EStageLevelType Type, FOnCreateStage OnCreateStage);
	void SetCurrentRoomIndex(int32 RowIndex, int32 ColumnIndex);

public:
	void CollectAssetIds(int32 RowIndex, int32 ColumnIndex, OUT TArray<FPrimaryAssetId>& PlayerIds, OUT FPrimaryAssetId& StageId, OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const;

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
	bool AddRewardSkill(const FPrimaryAssetId& SkillId);
	bool AddRewardEquipment(const FPrimaryAssetId& EquipmentId);
	bool AddRewardDice(const FPrimaryAssetId& DiceId);

public:
	const FRunLog& GetRunLog() const;

public:
	bool IsActive() const;

protected:
	/** 이 런의 첫 전투에서 튜토리얼을 보여줄지 여부. 이어하기에도 유지된다. */
	UPROPERTY(Category = Tutorial, SaveGame, VisibleAnywhere, meta = (DisplayName = "ShouldRunTutorial"))
	bool mShouldRunTutorial = false;



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
	void CompleteTutorial();

public:
	const FText& GetUserName() const;

public:
	const FUserLog& GetUserLog() const;

public:
	bool IsActive() const;
	bool IsTutorialCompleted() const;

protected:
	UPROPERTY(Category = User, SaveGame, VisibleAnywhere, meta = (DisplayName = "UserName"))
	FText mUserName;

	UPROPERTY(Category = Log, SaveGame, VisibleAnywhere, meta = (DisplayName = "UserLog"))
	FUserLog mUserLog;

	/** 완료와 건너뛰기를 모두 포함한다. true면 새 런에서 튜토리얼 방을 강제하지 않는다. */
	UPROPERTY(Category = Tutorial, SaveGame, VisibleAnywhere, meta = (DisplayName = "TutorialCompleted"))
	bool mTutorialCompleted = false;
};

USTRUCT()
struct FOptionPersistDataCache
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<USoundMix> mSoundMixObject;
	UPROPERTY()
	TArray<TObjectPtr<USoundClass>> mSoundClassObjects;
};

/**
 * @brief 옵션의 영구적 데이터
 */
UCLASS()
class P_RD_API UOptionPersistData : public UObject
{
	GENERATED_BODY()

public:
	UOptionPersistData();

public:
	void MakeCaches();

public:
	void MakeOption();
	void ClearOption();

public:
	void SetVolume(EGameVolumeType VolumeType, float Volume);
	void SetLanguage(ELanguageType LanguageType);
	void SetResolution(const FIntPoint& Resolution);
	void SetRenderResolution(int32 ShortSideHeight);
	void SetFpsLimit(int32 FpsLimit);

	void ApplyCurrentOptions();

public:
	float GetVolume(EGameVolumeType VolumeType) const;
	ELanguageType GetLanguage() const;
	const FIntPoint& GetResolution() const;
	int32 GetRenderResolutionHeight() const;
	int32 GetFpsLimit() const;

public:
	bool IsActive() const;

protected:
	UPROPERTY(Category = Option, SaveGame, VisibleAnywhere, meta = (DisplayName = "Volumes"))
	TArray<float> mVolumes;

protected:
	UPROPERTY(Category = Option, SaveGame, VisibleAnywhere, meta = (DisplayName = "LanguageType"))
	ELanguageType mLanguageType = ELanguageType::ENGLISH;

protected:
	UPROPERTY(Category = Option, SaveGame, VisibleAnywhere, meta = (DisplayName = "Resolution"))
	FIntPoint mResolution = FIntPoint::ZeroValue;

	// 3D 씬 렌더 해상도의 목표 짧은변(360/720/1080). 백버퍼 대비 비율을 r.ScreenPercentage로 적용하므로
	// UI(Slate)는 네이티브 해상도를 유지한다. 백버퍼 짧은변이 목표보다 작으면 100%로 클램프된다.
protected:
	UPROPERTY(Category = Option, SaveGame, VisibleAnywhere, meta = (DisplayName = "RenderResolutionHeight"))
	int32 mRenderResolutionHeight = 720;

protected:
	UPROPERTY(Category = Option, SaveGame, VisibleAnywhere, meta = (DisplayName = "FpsLimit"))
	int32 mFpsLimit = 60;

private:
	void ApplyRenderResolution() const;
	void HandleViewportResized(FViewport* Viewport, uint32 Unused);

	/* 캐싱 */
private:
	UPROPERTY()
	FOptionPersistDataCache mOptionPersistDataCache;

	// 뷰포트 생성/리사이즈(폴더블 접힘 전환 포함) 시 렌더 해상도 비율 재계산용 구독 핸들
private:
	FDelegateHandle mViewportResizedHandle;
};
