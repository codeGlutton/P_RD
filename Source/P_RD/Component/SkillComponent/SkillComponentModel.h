/*****************************************************************//**
 * @file   SkillComponentModel.h
 * @brief  액티브 스킬 컴포넌트 모델 구현 정의 헤더
 * @author 모호재, 이문환
 * @date   2026-06-30
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "Actor/TileMap/TileLayer.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SkillComponentModel.generated.h"

class UTileMapModel;
class IBoardCombatTarget;
class UStaticSkillData;

struct FPresentationBarrier;
struct FActiveSkillContext;
struct FEventTriggerPayloadBase;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnChangeSkillUI, int32 /*SkillIndex*/, const UStaticSkillData* /*PreSkillData*/, const UStaticSkillData* /*NewSkillData*/);

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPlaySkillUI, const FActiveSkillContext& /*Context*/, const UStaticSkillData* /*SkillData*/, TSharedPtr<FPresentationBarrier> /*SkillEndBarrier*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayPhaseLayerUI, const FActiveSkillContext& /*Context*/, int32 /*PhaseIndex*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEndPhaseLayerUI, int32 /*PhaseIndex*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEndSkillUI, const FActiveSkillContext& /*Context*/, const UStaticSkillData* /*SkillData*/);

/**
 * @brief 한 슬롯에 장착된 스킬과 그로 인해 설치된 런타임 객체 추적
 */
USTRUCT(BlueprintType)
struct FSkillEntry
{
	GENERATED_BODY()

public:
	FSkillEntry() = default;
	FSkillEntry(UStaticSkillData* Data);

public:
	bool IsValid() const;

public:
	// @brief 장착된 고정 스킬 데이터
	UPROPERTY(Category = "Static", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Data"))
	TObjectPtr<UStaticSkillData> mData = nullptr;

public:
	// @brief 쿨다운 이펙트 핸들
	UPROPERTY(Category = "Runtime", EditAnywhere, meta = (DisplayName = "CooldownHandle"))
	FActiveTacticalEffectHandle mCooldownHandle;
};

USTRUCT(BlueprintType)
struct FActiveSkillContext
{
	GENERATED_BODY()

public:
	void Clear();
	bool IsValid() const;

	/* 스킬 임시 데이터 */
public:
	TScriptInterface<IBoardCombatTarget> mInstigator = nullptr;
	TWeakObjectPtr<UTileMapModel> mMapModel = nullptr;

public:
	FTileIndex mSelfTileIndex = FTileIndex::Invalid;
	FTileIndex mAimedTileIndex = FTileIndex::Invalid;
	TArray<FTileIndex> mEffectTileIndexes;

public:
	ETileActorDirection mMotionLocalDir = ETileActorDirection::Forward;
	TSharedPtr<FPresentationBarrier> mSkillEndBarrier = nullptr;

public:
	int32 mSkillIndex = INDEX_NONE;
	int32 mAnimationIndex = INDEX_NONE;
	int32 mPhaseIndex = INDEX_NONE;

public:
	FOnEndSkillUI mEndCallback;

	/* 페이즈 임시 데이터 */
public:
	TArray<FTileIndex> mTargetTileIndexes;
	TArray<IBoardCombatTarget*> mOtherCombatTargets;
};

/**
 * @brief  액티브 스킬 컴포넌트 모델
 */
UCLASS()
class P_RD_API USkillComponentModel : public UComponentModel
{
	GENERATED_BODY()

public:
	USkillComponentModel();

	/* 스킬 세팅 */
public:
	/**
	 * @brief 스킬 목록(스폰 데이터 등)을 일괄 장착
	 * @param SkillList 스킬 목록
	 */
	void SetSkillFrom(const TArray<TSoftObjectPtr<UStaticSkillData>>& SkillList);
	void SetSkillFrom(const TArray<FPrimaryAssetId>& SkillList);

public:
	bool IsAcquirableSkill(UStaticSkillData* SkillData) const;

	const TArray<FSkillEntry>& GetSkills() const;
	const FSkillEntry* GetSkill(int32 SkillIndex) const;
	bool SetSkill(int32 SkillIndex, UStaticSkillData* SkillData);

	/* 스킬 실행 */
public:
	/**
	 * @brief 액티브 스킬이 사용가능한지 체크하는 함수
	 * @param SkillIndex 사용할 스킬의 인덱스
	 * @return 사용가능 여부
	 */
	bool CanActiveSkill(int32 SkillIndex) const;

	/**
	* @brief 액티브 스킬을 활성화하는 함수
	* @param MapModel 참고할 맵 모델
	* @param SkillIndex 사용할 스킬의 인덱스
	* @param AimedTileIndex 조준 타일
	*/
	bool ActivateSkill(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex, FOnEndSkillUI Callback = FOnEndSkillUI());
	void ForcedActivateSkill(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex, FOnEndSkillUI Callback = FOnEndSkillUI());

protected:
	virtual bool IsAcquirableSkill_Internal(UStaticSkillData* SkillData) const;
	virtual bool CanActiveSkill_Internal(int32 SkillIndex) const;
	virtual void ConsumeResources_Internal(int32 SkillIndex);
	virtual void ActivateSkill_Internal(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex, FOnEndSkillUI Callback);

protected:
	void PlaySkillAnimation();
	void EndSkillAnimation();

protected:
	void PreparePhaseLayer();
	void TriggerPhaseLayer(const FEventTriggerPayloadBase* Payload);
	void FlushRemainingPhaseLayers();

	void DeactivateSkill();

public:
	bool IsAnySkillActivated() const;
	const FActiveSkillContext& GetActiveSkillContext() const;

public:
	TArray<FTileIndex> GetAimableTiles(UTileMapModel* MapModel, int32 SkillIndex) const;
	TArray<FTileIndex> GetTargetTiles(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex) const;
	TArray<FTileIndex> GetEffectTiles(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex) const;

public:
	bool IsCooldown(int32 SkillIndex) const;
	ETacticalEffectDurationUnitType GetCooldownUnit(int32 SkillIndex) const;
	int32 GetStaticCooldownDuration(int32 SkillIndex) const;
	int32 GetCooldownDuration(int32 SkillIndex) const;
	int32 GetRemainingCooldownTime(int32 SkillIndex) const;

public:
	/**
	 * @brief 스킬 변경 시 호출되는 대리자
	 */
	FOnChangeSkillUI OnChangeSkillUI;

	/**
	 * @brief 스킬 실행 시 호출되는 대리자
	 */
	FOnPlaySkillUI OnPlaySkillUI;
	/**
	 * @brief 페이즈 재생 시 호출되는 대리자
	 */
	FOnPlayPhaseLayerUI OnPlayPhaseLayerUI;
	/**
	 * @brief 페이즈 종료 시 호출되는 대리자
	 */
	FOnEndPhaseLayerUI OnEndPhaseLayerUI;
	/**
	 * @brief 스킬 종료 시 호출되는 대리자
	 */
	FOnEndSkillUI OnEndSkillUI;

protected:
	UPROPERTY(Category = "Entry", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillEntries"))
	TArray<FSkillEntry> mSkillEntries;

protected:
	FActiveSkillContext mActiveSkillContext;
};
