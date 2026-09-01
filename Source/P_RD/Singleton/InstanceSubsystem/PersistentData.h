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

class UPartyModel;
class UPlayerUnitModel;
class FViewport;
class UStaticSkillData;
class UNiagaraSystem;

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
	UPROPERTY(Category = Record, SaveGame, VisibleAnywhere, meta = (DisplayName = "UseCountPerUnit"))
	TMap<FPrimaryAssetId, int32> mUseCountPerUnit;

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
	UPROPERTY(Category = Record, SaveGame, VisibleAnywhere, meta = (DisplayName = "UseCountPerUnit"))
	TMap<FPrimaryAssetId, int32> mUseCountPerUnit;

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
	void MakeUnit(const FPrimaryAssetId& PlayerUnitId);
	void MakeUnit(UPlayerUnitModel* PlayerUnit);
	void ClearUnit();

public:
	void RegisterPlayerUnit(UPlayerUnitModel* PlayerUnit);
	void UnregisterPlayerUnit(UPlayerUnitModel* PlayerUnit);

public:
	const FPrimaryAssetId& GetPlayerUnitId() const;
	int32 GetPlayerLevel() const;
	float GetExperience() const;

public:
	const TArray<FPrimaryAssetId>& GetSkillIds() const;

public:
	virtual void Serialize(FArchive& Ar) override;

protected:
	void SyncPlayerPersistData( UPlayerUnitModel* PlayerUnit);
	void BindPlayerUnitEvent(UPlayerUnitModel* PlayerUnit);
	void UnbindPlayerUnitEvent(UPlayerUnitModel* PlayerUnit);

protected:
	UPROPERTY(Category = Player, SaveGame, VisibleAnywhere, meta = (DisplayName = "PlayerUnitId"))
	FPrimaryAssetId mPlayerUnitId;
	UPROPERTY(Category = Player, SaveGame, VisibleAnywhere, meta = (DisplayName = "PlayerLevel"))
	int32 mPlayerLevel = 1;

protected:
	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "MaxHP"))
	float mMaxHP = 0.f;
	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "HP"))
	float mHP = 0.f;

	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "Exp"))
	float mExp = 0.f;

protected:
	UPROPERTY(Category = Tag, SaveGame, VisibleAnywhere, meta = (DisplayName = "TagCountMap"))
	TMap<FGameplayTag, int32> mTagCountMap;

	UPROPERTY(Category = Skill, SaveGame, VisibleAnywhere, meta = (DisplayName = "SkillIds"))
	TArray<FPrimaryAssetId> mSkillIds;
};

/**
 * @brief 파티의 영구적 데이터
 */
UCLASS()
class P_RD_API UPartyPersistData : public UObject
{
	GENERATED_BODY()

public:
	UPartyPersistData();

	/* UObject 상속 */
public:
	void Serialize(FArchive& Ar) override;

public:
	void RegisterParty(UPartyModel* Party, TArray<TObjectPtr<UPlayerUnitModel>>& Players);

public:
	TArray<FPrimaryAssetId> GetPlayerUnitIds() const;
	int32 GetDifficulty() const;
	const TArray<FPrimaryAssetId>& GetArtifactIds() const;

protected:
	void SyncPartyPersistData(UPartyModel* Party, TArray<TObjectPtr<UPlayerUnitModel>>& Players);
	void BindPartyEvent(UPartyModel* Party, TArray<TObjectPtr<UPlayerUnitModel>>& Players);

protected:
	UPROPERTY(Category = Party, VisibleAnywhere, meta = (DisplayName = "PartyPlayers"))
	TArray<TObjectPtr<UPlayerUnitPersistData>> mPartyPlayers;

	UPROPERTY(Category = Artifact, SaveGame, VisibleAnywhere, meta = (DisplayName = "ArtifactIds"))
	TArray<FPrimaryAssetId> mArtifactIds;

protected:
	UPROPERTY(Category = Party, SaveGame, VisibleAnywhere, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty = 1;

	UPROPERTY(Category = Party, SaveGame, VisibleAnywhere, meta = (DisplayName = "Money"))
	float mMoney = 0.f;
};

USTRUCT()
struct FRunPersistDataCache
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<TObjectPtr<UNiagaraSystem>> mEffectVFXs;
	UPROPERTY()
	TArray<TObjectPtr<UCurveBase>> mEffectCurves;
};

/**
 * @brief 이번 런의 영구적 데이터
 */
UCLASS()
class P_RD_API URunPersistData : public UPartyPersistData
{
	GENERATED_BODY()

	/* UObject 상속 */
public:
	void Serialize(FArchive& Ar) override;

public:
	void MakeCaches();

public:
	void StartRun(const TArray<FPrimaryAssetId>& PlayerUnitIds, int32 Difficulty);
	void ClearRun();
	bool AddRewardSkill(const FPrimaryAssetId& SkillId);
	bool AddRewardEquipment(const FPrimaryAssetId& EquipmentId);

public:
	void MakeStageAsync(EStageLevelType Type, FOnCreateStage OnCreateStage);
	void SetCurrentRoomIndex(int32 RowIndex, int32 ColumnIndex);
	void SetRoomClearData(const FRoomClearData& ClearData);

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
	const TArray<FPrimaryAssetId>& GetRewardSkillIds() const;
	const TArray<FPrimaryAssetId>& GetRewardEquipmentIds() const;

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

	/**
	 * @brief 런 동안 획득했지만 아직 특정 용병 슬롯에 장착하지 않은 공용 보관함.
	 * @details 동일 스킬 소유를 허용하므로 배열에서 중복을 제거하지 않는다.
	 */
	UPROPERTY(Category = Reward, SaveGame, VisibleAnywhere, meta = (DisplayName = "RewardSkillIds"))
	TArray<FPrimaryAssetId> mRewardSkillIds;
	UPROPERTY(Category = Reward, SaveGame, VisibleAnywhere, meta = (DisplayName = "RewardEquipmentIds"))
	TArray<FPrimaryAssetId> mRewardEquipmentIds;

protected:
	UPROPERTY(Category = Log, SaveGame, VisibleAnywhere, meta = (DisplayName = "RunLog"))
	FRunLog mRunLog;

	/* 캐싱 */
private:
	UPROPERTY()
	FRunPersistDataCache mRunPersistDataCache;
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
	void UpdateLog(const FRunLog& RunLog);

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
	void SetOverallQuality(EOverallQualityType QualityType);
	void SetFpsLimit(int32 FpsLimit);
	void SetCameraShakeEnabled(bool IsEnabled);
	void SetEffectVFXEnabled(bool IsEnabled);

	void ApplyCurrentOptions();

public:
	float GetVolume(EGameVolumeType VolumeType) const;
	ELanguageType GetLanguage() const;
	EOverallQualityType GetOverallQuality() const;
	int32 GetFpsLimit() const;
	bool IsCameraShakeEnabled() const;
	bool IsEffectVFXEnabled() const;

public:
	bool IsActive() const;

private:
	void ApplyScreenPercentage() const;
	void OnResizeViewport(FViewport* Viewport, uint32 Unused);

	/* 사운드 옵션 */
protected:
	UPROPERTY(Category = Option, SaveGame, VisibleAnywhere, meta = (DisplayName = "Volumes"))
	TArray<float> mVolumes;

	/* 언어 옵션 */
protected:
	UPROPERTY(Category = Option, SaveGame, VisibleAnywhere, meta = (DisplayName = "LanguageType"))
	ELanguageType mLanguageType = ELanguageType::ENGLISH;

	/* 그래픽 옵션 */
protected:
	UPROPERTY(Category = Option, SaveGame, VisibleAnywhere, meta = (DisplayName = "OverallQuality"))
	EOverallQualityType mOverallQuality = EOverallQualityType::Medium;

	UPROPERTY(Category = Option, SaveGame, VisibleAnywhere, meta = (DisplayName = "FpsLimit"))
	int32 mFpsLimit = 60;

	UPROPERTY(Category = Option, SaveGame, VisibleAnywhere, meta = (DisplayName = "CameraShakeEnabled"))
	bool mCameraShakeEnabled = true;

	UPROPERTY(Category = Option, SaveGame, VisibleAnywhere, meta = (DisplayName = "EffectVFXEnabled"))
	bool mEffectVFXEnabled = true;

	/* 캐싱 */
private:
	UPROPERTY()
	FOptionPersistDataCache mOptionPersistDataCache;
};
