/*****************************************************************//**
 * @file   PlayerUnitModel.h
 * @brief  플레이어 베이스 유닛 정의 헤더
 * @author 모호재
 * @date   2026-05-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "Pawn/UnitModel.h"
#include "DataAsset/UnitSpawnData/PlayerJobType.h"

#include "PlayerUnitModel.generated.h"

class UPlayerUnitModel;
class UPartyModel;
class UArtifactComponentModel;

class UPlayerUnitAttributeSet;
class ULevelAttributeSet;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangePlayerLevel, UPlayerUnitModel* /*Model*/, int32 /*PlayerLevel*/);

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

public:
	void SetOwnerParty(UPartyModel* PartyModel);
	void SetPlayerLevel(int32 PlayerLevel);

public:
	EPlayerJobType GetPlayerJobType() const;
	int32 GetPlayerLevel() const;
	int32 GetDifficulty() const override;
	bool IsPlayerUnitModel() const override;

public:
	UArtifactComponentModel* GetArtifactComponentModel() const;

public:
	FOnChangePlayerLevel OnChangePlayerLevel;

private:
	/** @brief 난이도 스케일 AttributeSet */
	UPROPERTY(Category = AttributeSet, VisibleAnywhere, meta = (DisplayName = "UnitAttributeSet"))
	TObjectPtr<UPlayerUnitAttributeSet> mUnitAttributeSet;

	/** @brief 레벨 스케일 AttributeSet */
	UPROPERTY(Category = AttributeSet, VisibleAnywhere, meta = (DisplayName = "LevelAttributeSet"))
	TObjectPtr<ULevelAttributeSet> mLevelAttributeSet;

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
