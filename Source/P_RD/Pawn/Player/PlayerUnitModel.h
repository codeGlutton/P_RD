/*****************************************************************//**
 * @file   PlayerUnitModel.h
 * @brief  플레이어 베이스 유닛 정의 헤더
 * @author 모호재
 * @date   2026-05-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "Pawn/UnitModel.h"

#include "PlayerUnitModel.generated.h"

class UPlayerUnitModel;
class UPartyModel;
class UArtifactComponentModel;

class UPlayerUnitAttributeSet;

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
	void PostInitializeComponentModels() override;

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
	int32 GetPlayerLevel() const;

public:
	UArtifactComponentModel* GetArtifactComponentModel() const;

public:
	FOnChangePlayerLevel OnChangePlayerLevel;

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
