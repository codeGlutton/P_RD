/*****************************************************************//**
 * @file   SkillComponentModel.h
 * @brief  액티브 스킬 컴포넌트 모델 구현 정의 헤더
 * @author 모호재, 이문환
 * @date   2026-06-30
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "Actor/TileMap/TileLayer.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SkillComponentModel.generated.h"

class UTileMapModel;
class IBoardCombatTarget;
class UStaticSkillData;

struct FPresentationBarrier;
struct FActiveSkillContext;
struct FApplyEventTriggerPayload;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnChangeSkillUI, int32 /*SkillIndex*/, const UStaticSkillData* /*PreSkillData*/, const UStaticSkillData* /*NewSkillData*/);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlaySkillUI, const FActiveSkillContext& /*Context*/, const UStaticSkillData* /*SkillData*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnPlayMotionLayerUI, int32 /*MotionIndex*/, TSharedPtr<FPresentationBarrier> /*MotionEndBarrier*/, FGameplayTag /*ApplyMotionTag*/, ETileActorDirection /*LocalDirection*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEndMotionLayerUI, int32 /*MotionIndex*/);
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
	TWeakObjectPtr<UTileMapModel> mMapModel = nullptr;

public:
	int32 mDiceSum = 0;
	FTileIndex mSelfTileIndex = FTileIndex::Invalid;
	FTileIndex mTargetTileIndex = FTileIndex::Invalid;
	TArray<FTileIndex> mEffectTileIndexes;
	bool mAllowFriendlyFire = false;

public:
	int32 mSkillIndex = INDEX_NONE;
	int32 mMotionIndex = INDEX_NONE;

public:
	FOnEndSkillUI mEndCallback;

	/* 모션 임시 데이터 */
public:
	TArray<FTileIndex> mTargetTileIndexes;
	TArray<IBoardCombatTarget*> mOtherCombatTargets;
	/** @brief 여러 모션에서 한 번이라도 실제 대상으로 판정된 객체들의 합집합. */
	TArray<IBoardCombatTarget*> mResolvedCombatTargets;

public:
	ETileActorDirection mMotionTileMapDir = ETileActorDirection::Forward;
	TWeakPtr<FPresentationBarrier> mMotionEndBarrier = nullptr;
	bool mIsMotionTriggered = false;
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

public:
	/**
	 * @brief 스킬 목록(스폰 데이터 등)을 일괄 장착
	 * @param SkillList 스킬 목록
	 */
	void SetSkillFrom(const TArray<TSoftObjectPtr<UStaticSkillData>>& SkillList);
	void SetSkillFrom(const TArray<FPrimaryAssetId>& SkillList);

public:
	const TArray<FSkillEntry>& GetSkills() const;
	const FSkillEntry* GetSkill(int32 SkillIndex) const;
	void SetSkill(int32 SkillIndex, UStaticSkillData* SkillData);

public:
	/**
	* @brief 액티브 스킬을 활성화하는 함수
	* @param MapModel 참고할 맵 모델
	* @param SkillIndex 사용할 스킬의 인덱스
	* @param TargetIndex 타겟팅 타일
	* @param DiceSum 주사위 눈금 합
	*/
	void ActivateSkill(
		UTileMapModel* MapModel,
		int32 SkillIndex,
		const FTileIndex& TargetIndex,
		int32 DiceSum,
		FOnEndSkillUI Callback = FOnEndSkillUI(),
		const TArray<FTileIndex>* FixedEffectTileIndexes = nullptr,
		bool bAllowFriendlyFire = false);

protected:
	void PlayMotionLayer();
	// @brief 모션 레이어의 애니메이션 재생 (자동 회전이 필요한 경우, 회전 연출 완료 후 호출됨)
	void PlayMotionLayerAnimation(ETileActorDirection LocalDirectionToTarget);
	void TriggerMotionLayer(const FApplyEventTriggerPayload* Payload);
	void EndMotionLayer();
	void DeactivateSkill();

public:
	bool IsAnySkillActivated() const;

public:
	TArray<FTileIndex> GetAimableTiles(UTileMapModel* MapModel, int32 SkillIndex, int32 DiceSum) const;
	TArray<FTileIndex> GetEffectTiles(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& TargetIndex, int32 DiceSum) const;

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
	 * @brief 모션 애니메이션 재생 시 호출되는 대리자
	 */
	FOnPlayMotionLayerUI OnPlayMotionLayerUI;
	/**
	 * @brief 모션 애니메이션 종료 시 호출되는 대리자
	 */
	FOnEndMotionLayerUI OnEndMotionLayerUI;
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
