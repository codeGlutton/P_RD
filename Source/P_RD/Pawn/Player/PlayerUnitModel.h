/*****************************************************************//**
 * @file   PlayerUnitModel.h
 * @brief  플레이어 베이스 유닛 정의 헤더
 * @author 모호재
 * @date   2026-05-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "Pawn/UnitModel.h"
#include "DataAsset/RarityRate.h"

#include "PlayerUnitModel.generated.h"

class UPlayerUnitModel;
class UPartyModel;
class UArtifactComponentModel;

class UPlayerUnitAttributeSet;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangePlayerLevel, UPlayerUnitModel* /*Model*/, int32 /*NewPlayerLevel*/);

/** @brief 경험치가 한 레벨 구간에서 어떻게 소비되는지 나타내는 비변경 계산 결과. */
struct P_RD_API FPlayerExpProgressStep
{
	int32 mLevelBefore = 1;
	int32 mLevelAfter = 1;
	float mExpBefore = 0.f;
	float mExpAfter = 0.f;
	float mMaxExp = 0.f;
	float mCarryExp = 0.f;
	bool mDidLevelUp = false;
};

/** @brief 현재 경험치에 보상 경험치를 더했을 때의 전체 진행 결과. */
struct P_RD_API FPlayerExpProgression
{
	int32 mLevelBefore = 1;
	int32 mLevelAfter = 1;
	float mExpBefore = 0.f;
	float mExpAfter = 0.f;
	float mMaxExpAfter = 0.f;
	float mExperienceGain = 0.f;
	bool mReachedLevelCap = false;
	TArray<FPlayerExpProgressStep> mSteps;
};

/**
 * @brief 한 번의 레벨 증가 알림에 필요한 완결된 데이터.
 * @details 희귀도 커브가 없거나 유효하지 않으면 mHasSkillRarityRate가 false다.
 */
struct P_RD_API FPlayerLevelUpEvent
{
	int32 mPreviousLevel = 1;
	int32 mNewLevel = 1;
	float mConsumedExpThreshold = 0.f;
	float mRemainingExperience = 0.f;
	float mNextMaxExp = 0.f;
	bool mHasSkillRarityRate = false;
	FRarityRate mSkillRarityRate;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayerLevelUp, UPlayerUnitModel* /*Model*/, const FPlayerLevelUpEvent& /*Event*/);

/**
 * @brief 플레이어 베이스 유닛 모델 입니다.
 * @details
 * 플레이어만 런 보유 주사위와 레벨/난이도 기반 Attribute 초기화를 갖는다. 적 유닛 공통 베이스에 이 계약을 올리지 않는다.
 */
UCLASS(abstract)
class P_RD_API UPlayerUnitModel : public UUnitModel
{
	GENERATED_BODY()

public:
	UPlayerUnitModel();

	/* UUnitModel 상속 */
public:
	int32 GetBoardActorLevel() const override;
	EUnitJobType GetUnitJobType() const override;
	int32 GetDifficulty() const override;
	bool IsPlayerUnitModel() const override;

	/* 파티 함수 */
public:
	void SetOwnerParty(UPartyModel* PartyModel);
	void SetPlayerLevel(int32 PlayerLevel);

public:
	UPartyModel* GetOwnerParty() const;
	int32 GetPlayerLevel() const;
	float GetMaxExpForPlayerLevel(int32 PlayerLevel) const;
	int32 GetMaxPlayerLevel() const;
	TArray<float> GetExperienceThresholds() const;

public:
	/** @brief 현재 상태를 변경하지 않고 경험치 보상 적용 결과를 계산한다. */
	FPlayerExpProgression PreviewExperienceGain(float ExperienceGain) const;

	/**
	 * @brief 명시적인 임계치 배열로 경험치 진행 결과를 계산하는 순수 함수.
	 * @details Thresholds[N-1]은 N레벨의 최대 경험치이며 마지막 항목의 레벨이 상한이다.
	 */
	static FPlayerExpProgression CalculateExperienceProgression(
		int32 StartingLevel,
		float StartingExp,
		float ExperienceGain,
		const TArray<float>& Thresholds);

	/** @brief AttributeSet에 반영된 Exp를 공통 진행 규칙으로 정규화한다. */
	void ResolveExperienceChange(float OldValue, float NewValue);

	/** @brief 상한을 검사한 뒤 한 레벨 올리고 완결된 이벤트를 방송한다. */
	bool LevelUp(float RemainingExperience = 0.f);

public:
	UArtifactComponentModel* GetArtifactComponentModel() const;

public:
	FOnChangePlayerLevel OnChangePlayerLevel;
	FOnPlayerLevelUp OnPlayerLevelUp;

private:
	/** @brief 난이도 스케일 AttributeSet */
	UPROPERTY(Category = AttributeSet, VisibleAnywhere, meta = (DisplayName = "UnitAttributeSet"))
	TObjectPtr<UPlayerUnitAttributeSet> mUnitAttributeSet;

	/** @brief 아티펙트 컴포넌트 모델 */
	UPROPERTY(Category = Artifact, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "ArtifactCompModel"))
	TObjectPtr<UArtifactComponentModel> mArtifactCompModel;

protected:
	UPROPERTY(Category = Party, VisibleAnywhere, meta = (DisplayName = "OwnerParty"))
	TWeakObjectPtr<UPartyModel> mOwnerParty;

protected:
	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PlayerLevel"))
	int32 mPlayerLevel = 1;

private:
	void RefreshMaxExperienceThreshold();
	bool mIsResolvingExperience = false;
};
