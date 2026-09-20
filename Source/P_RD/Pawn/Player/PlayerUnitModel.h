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

struct FPlayerLevelUpEvent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangePlayerLevel, UPlayerUnitModel* /*Model*/, int32 /*NewPlayerLevel*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayerLevelUp, UPlayerUnitModel* /*Model*/, const FPlayerLevelUpEvent& /*Event*/);

/**
 * @brief 한 번의 레벨 증가에 대한 데이터
 */
USTRUCT()
struct P_RD_API FPlayerLevelUpData
{
	GENERATED_BODY()

public:
	int32 mPreLevel = 1;
	int32 mCurLevel = 1;
	float mMaxExp = 0.f;
	float mPreExp = 0.f;
	float mCurExp = 0.f;
	float mCarryExp = 0.f;
};

/**
 * @brief 한 번의 레벨 증가 알림에 필요한 완결된 데이터
 * @details 희귀도 커브가 없거나 유효하지 않으면 mHasRarityRate가 false
 */
USTRUCT()
struct P_RD_API FPlayerLevelUpEvent
{
	GENERATED_BODY()

public:
	FPlayerLevelUpData mData;

public:
	bool mHasRarityRate = false;
	FRarityRate mSkillRarityRate;
};

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

public:
	UArtifactComponentModel* GetArtifactComponentModel() const;

public:
	TArray<FPlayerLevelUpData> PredictLevelChange(float ExpGain) const;
	void PostChangeExperience(float OldExp, float NewExp);

protected:
	TArray<FPlayerLevelUpData> CalculateLevelChange(int32 StartLevel, float StartExp, float ExpGain) const;
	void LevelUp(const FPlayerLevelUpData& LevelUpData);

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
};
