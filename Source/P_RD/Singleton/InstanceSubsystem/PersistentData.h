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
class UStaticSkillData;

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
 * @brief 파티 한 명의 영구적 데이터
 *
 * @details
 * UObject 가 아니라 USTRUCT 다. 세이브가 FObjectAndNameAsStringProxyArchive 를
 * 쓰는데, 이건 UObject 참조를 경로 문자열로 적는다. 런타임에 만든 하위
 * 오브젝트는 경로가 없어 다음에 열 때 통째로 사라진다. 런 로그와 스테이지가
 * 이미 구조체인 것도 같은 이유다.
 *
 * 값만 들고 있고 행동은 URunPersistData 가 한다. 구조체를 배열에 담으면
 * 배열이 늘어날 때 주소가 바뀌므로, 유닛 이벤트에 이 안의 값을 바로 걸 수
 * 없기 때문이다 -- 런이 자리 번호로 찾아 넣는다.
 */
USTRUCT()
struct P_RD_API FPartyMemberPersistData
{
	GENERATED_BODY()

	/** @brief 아직 유닛에서 값을 한 번도 안 떠 왔다. */
	UPROPERTY(SaveGame)
	bool mIsNewData = true;

	UPROPERTY(Category = Player, SaveGame, VisibleAnywhere, meta = (DisplayName = "PlayerUnitId"))
	FPrimaryAssetId mPlayerUnitId;

	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "MaxHP"))
	float mMaxHP = 0.f;
	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "HP"))
	float mHP = 0.f;

	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "Exp"))
	float mExp = 0.f;

	UPROPERTY(Category = Attribute, SaveGame, VisibleAnywhere, meta = (DisplayName = "Money"))
	float mMoney = 0.f;

	UPROPERTY(Category = Tag, SaveGame, VisibleAnywhere, meta = (DisplayName = "TagCountMap"))
	TMap<FGameplayTag, int32> mTagCountMap;

	UPROPERTY(Category = Skill, SaveGame, VisibleAnywhere, meta = (DisplayName = "SkillIds"))
	TArray<FPrimaryAssetId> mSkillIds;

	UPROPERTY(Category = Equipment, SaveGame, VisibleAnywhere, meta = (DisplayName = "EquipmentIds"))
	TArray<FPrimaryAssetId> mEquipmentIds;

	UPROPERTY(Category = Dice, SaveGame, VisibleAnywhere, meta = (DisplayName = "DiceIds"))
	TArray<FPrimaryAssetId> mDiceIds;
};

/**
 * @brief 이번 런의 영구적 데이터
 *
 * @details
 * 예전에는 이 클래스가 UPlayerUnitPersistData 를 상속했다. 런이 곧 플레이어
 * 한 명이었다는 뜻이다. 처음에 셋을 고르게 되면서 그 전제가 깨졌다.
 *
 * 이제 런은 파티를 가진다. 런 전체에 하나뿐인 것(레벨, 난이도, 스테이지,
 * 로그, 난수 씨앗)만 런이 직접 들고, 사람마다 다른 것은 파티 칸에 들어간다.
 */
UCLASS()
class P_RD_API URunPersistData : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 새 런을 시작한다.
	 * @param PartyUnitIds 데리고 갈 유닛들. 고른 차례가 파티 칸 순서다.
	 * @param Difficulty   이번 런의 난이도
	 */
	void StartRun(const TArray<FPrimaryAssetId>& PartyUnitIds, int32 Difficulty);
	void ClearRun();

public:
	/** @brief 스폰된 유닛을 제 자리 파티 칸에 잇는다. 못 찾으면 아무 일도 없다. */
	void RegisterPlayerUnit(UPlayerUnitModel* PlayerUnit);

public:
	const TArray<FPartyMemberPersistData>& GetParty() const;
	/** @brief 파티 전원의 유닛 식별자. 스폰과 자산 예약이 이걸 쓴다. */
	TArray<FPrimaryAssetId> GetPartyUnitIds() const;
	/** @brief 앞장선 한 명. 기록처럼 대표 하나만 필요한 자리에서 쓴다. */
	const FPrimaryAssetId& GetPlayerUnitId() const;

public:
	int32 GetPlayerLevel() const;
	int32 GetDifficulty() const;

public:
	void MakeStageAsync(EStageLevelType Type, FOnCreateStage OnCreateStage);
	void SetCurrentRoomIndex(int32 RowIndex, int32 ColumnIndex);
	void ClearCurrentCombatRoom(const FTileTransform& Transform);

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
	const FRunLog& GetRunLog() const;

public:
	bool IsActive() const;

protected:
	/** @brief 파티 칸 번호를 찾는다. 없으면 INDEX_NONE. */
	int32 FindMemberIndex(const FPrimaryAssetId& PlayerUnitId) const;

	void SetupMember(OUT FPartyMemberPersistData& Member, const FPrimaryAssetId& PlayerUnitId);
	void SyncMember(int32 MemberIndex, UPlayerUnitModel* PlayerUnit);
	void BindMemberEvent(int32 MemberIndex, UPlayerUnitModel* PlayerUnit);

	void OnChangePlayerSkill(int32 SkillIndex, const UStaticSkillData* PreSkillData, const UStaticSkillData* NewSkillData, int32 MemberIndex);

protected:
	UPROPERTY(Category = Party, SaveGame, VisibleAnywhere, meta = (DisplayName = "Party"))
	TArray<FPartyMemberPersistData> mParty;

protected:
	UPROPERTY(Category = Player, SaveGame, VisibleAnywhere, meta = (DisplayName = "PlayerLevel"))
	int32 mPlayerLevel = 1;
	UPROPERTY(Category = Player, SaveGame, VisibleAnywhere, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty = 1;

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

private:
	// 뷰포트 생성/리사이즈(폴더블 접힘 전환 포함) 시 렌더 해상도 비율 재계산용 구독 핸들
	FDelegateHandle mViewportResizedHandle;
};
