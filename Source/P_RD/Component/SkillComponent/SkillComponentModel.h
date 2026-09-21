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

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPrePlaySkillUI, const FActiveSkillContext& /*Context*/, const UStaticSkillData* /*SkillData*/, TSharedPtr<FPresentationBarrier> /*SkillPlayBarrier*/);

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPlaySkillUI, const FActiveSkillContext& /*Context*/, const UStaticSkillData* /*SkillData*/, TSharedPtr<FPresentationBarrier> /*SkillEndBarrier*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayPhaseLayerUI, const FActiveSkillContext& /*Context*/, int32 /*PhaseIndex*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEndPhaseLayerUI, int32 /*PhaseIndex*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEndSkillUI, const FActiveSkillContext& /*Context*/, const UStaticSkillData* /*SkillData*/);

// @brief 강제이동 출발 대리자. 출발 시점에 경로를 정해 이동을 시작. 이동을 시작했으면 MoveEndBarrier를 이동 종료까지 보유
DECLARE_DELEGATE_OneParam(FOnStartForcedMove, TSharedPtr<FPresentationBarrier> /*MoveEndBarrier*/);

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
	TArray<FTileIndex> mTargetTileIndexes;
	TArray<FTileIndex> mEffectTileIndexes;

public:
	ETileActorDirection mMotionLocalDir = ETileActorDirection::Forward;
	TSharedPtr<FPresentationBarrier> mSkillEndBarrier = nullptr;

public:
	int32 mSkillIndex = INDEX_NONE;
	int32 mAnimationIndex = INDEX_NONE;
	int32 mPhaseIndex = INDEX_NONE;

public:
	// @brief 스킬 종료 보류 여부. 시전자 연출은 끝났지만 피격자 밀당이 남아서 DeactivateSkill이 중단됐으면 true (밀당이 끝나면 다시 호출됨)
	bool mIsDeactivationPending = false;

public:
	FOnEndSkillUI mEndCallback;

	/* 페이즈 임시 데이터 */
public:
	TArray<FTileIndex> mFinalTileIndexes;
	TArray<IBoardCombatTarget*> mFinalCombatTargets;
};

/**
 * @brief  액티브 스킬 컴포넌트 모델
 * @details 페이즈에 밀치기/당기기가 있으면 피격자 이동이 끝날 때까지 페이즈 종료를 미룸.
 *          시전자 연출과 마지막 페이즈 중 늦게 끝나는 쪽에서 스킬 종료
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

public:
	const TArray<FSkillEntry>& GetSkills() const;
	const FSkillEntry* GetSkill(int32 SkillIndex) const;
	bool SetSkill(int32 SkillIndex, UStaticSkillData* SkillData);

	/**
	 * @brief 스킬 슬롯을 비움 (버리기)
	 * @details 장착이 아니므로 습득 가능 검사(IsAcquirableSkill) 대상이 아님
	 * @param SkillIndex 비울 스킬 슬롯 인덱스
	 */
	void RemoveSkill(int32 SkillIndex);

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
	* @return 실패 여부
	*/
	bool TryToActivateSkill(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex, FOnEndSkillUI Callback = FOnEndSkillUI());
	void ForcedActivateSkill(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex, FOnEndSkillUI Callback = FOnEndSkillUI());

protected:
	virtual bool IsAcquirableSkill_Internal(UStaticSkillData* SkillData) const;
	virtual bool CanActiveSkill_Internal(int32 SkillIndex) const;
	virtual void ConsumeResources_Internal(int32 SkillIndex);

protected:
	void PlaySkillAnimation();
	void EndSkillAnimation();

protected:
	void ActivateSkill(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex, FOnEndSkillUI Callback);

	void PreparePhaseLayer();
	void TriggerPhaseLayer(const FEventTriggerPayloadBase* Payload);
	// @brief 페이즈 종료 처리. 페이즈 배리어 소멸 시 호출
	void EndPhaseLayer();
	void FlushRemainingPhaseLayers();

	void DeactivateSkill();

	/* 랜덤 데미지 연관 */
public:
	virtual bool CanPreview(int32 SkillIndex) const;
	/**
	 * @brief 랜덤 확률을 데미지를 추출하는 함수
	 * @param Min 최소 데미지
	 * @param Max 최대 데미지
	 * @return Min, Max 사이의 랜덤한 값
	 */
	virtual int32 GetRandomDamage(int32 Min, int32 Max) const;
	/**
	 * @brief 크리티컬 여부를 확인하는 함수
	 * @param Threshold 크리티컬 확정 커트 라인
	 * @return 크리티컬 여부
	 */
	virtual bool IsCritical(int32 Threshold) const;

	/* 강제이동 대기열 */
public:
	/**
	 * @brief 밀치기/당기기 요청을 대기열에 넣음
	 * @details 이펙트 적용 중에 들어온 요청은 바로 밀당하지 않고 모아 둠.
	 *          이펙트 적용이 끝나면 모아놨던 요청을 하나씩 꺼내서 차례로 밀당하고, 모두 끝나면 페이즈 종료
	 * @param Start 차례가 오면 실행할 출발 대리자 (그때의 배치로 경로를 정해 이동 시작)
	 * @return 대기열에 넣었으면 true. 이펙트 적용 중이 아니면 false (호출자가 바로 밀면 됨)
	 */
	bool EnqueueForcedMove(FOnStartForcedMove Start);

private:
	// @brief 대기열에서 다음 이동 요청을 꺼내서 출발. 모든 요청을 처리하면 배리어를 풀어서 페이즈 종료
	void StartNextForcedMove();
	// @brief 밀당이 끝나기를 기다리는 중인지 여부 (대기열이 페이즈 배리어를 쥐고 있으면 기다리는 중)
	bool IsWaitingForcedMove() const { return mForcedMovePhaseBarrier.IsValid(); }

private:
	// @brief 출발 대기 중인 이동 요청 (등록 순서 = 출발 순서)
	TArray<FOnStartForcedMove> mPendingForcedMoves;
	// @brief 대기열이 잡고 있는 페이즈 배리어. 마지막 요청까지 끝나면 놓음
	TSharedPtr<FPresentationBarrier> mForcedMovePhaseBarrier;
	// @brief 밀당 요청을 대기열에 넣어야 하는지 여부. 페이즈 이펙트 적용 루프 동안만 켜짐 (꺼져 있으면 Push/Pull이 그 자리에서 바로 밈)
	bool mShouldEnqueueForcedMove = false;

	/* 추가 API */
public:
	bool IsAnySkillActivated() const;
	const FActiveSkillContext& GetActiveSkillContext() const;

public:
	TArray<FTileIndex> GetAimableTiles(const UTileMapModel* MapModel, int32 SkillIndex) const;
	TArray<FTileIndex> GetTargetTiles(const UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex) const;
	TArray<FTileIndex> GetEffectTiles(const UTileMapModel* MapModel, int32 SkillIndex, const TArray<FTileIndex>& TargetTileIndexes) const;
	TArray<FTileIndex> GetEffectTiles(const UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& AimedTileIndex) const;

public:
	bool IsCooldown(int32 SkillIndex) const;
	ETacticalEffectDurationUnitType GetCooldownUnit(int32 SkillIndex) const;
	int32 GetStaticCooldownDuration(int32 SkillIndex) const;
	int32 GetCooldownDuration(int32 SkillIndex) const;
	int32 GetRemainingCooldownTime(int32 SkillIndex) const;

public:
	static constexpr int32 DEFAULT_SKILL_POOL_SIZE = 5;

public:
	/**
	 * @brief 스킬 변경 시 호출되는 대리자
	 */
	FOnChangeSkillUI OnChangeSkillUI;

	/**
	 * @brief 스킬 실행 전 호출되는 대리자
	 */
	FOnPrePlaySkillUI OnPrePlaySkillUI;

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
