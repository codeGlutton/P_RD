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
class ULevelAttributeSet;
class UDicePoolModel;

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
	/** @brief 플레이어 유닛 서브오브젝트를 생성한다; 런 데이터 주입은 GameMode/Subsystem 단계에서 따로 수행된다. */
	UPlayerUnitModel();

	/* UUnitModel 상속 */
public:
	void PostInitializeComponentModels() override;

protected:
	UUserWidget* GetInfoPanel() const override;

public:
	EPlayerJobType GetPlayerJobType() const;
	int32 GetPlayerLevel() const;
	int32 GetDifficulty() const override;
	bool IsPlayerUnitModel() const override;

public:
	/** @brief 플레이어 보유 주사위 컴포넌트입니다. 적은 주사위가 없어 AUnit이 아닌 APlayerUnit에 둡니다. */
	UDicePoolModel* GetDicePool() const;

public:
	FOnChangePlayerLevel OnChangePlayerLevel;

private:
	/** @brief 레벨 스케일 AttributeSet. 기존 GAS 초기화 경로가 참조하므로 제거 전까지 유지한다. */
	/*UPROPERTY(Category = GAS, VisibleAnywhere, meta = (DisplayName = "LevelAttributeSet"))
	TObjectPtr<ULevelAttributeSet> mLevelAttributeSet;*/

	/** @brief 런타임 보유 주사위 묶음. 전투 HUD는 이 객체를 직접 소유하지 않고 어댑터를 통해 읽는다. */
	UPROPERTY(Category = Dice, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "DiceComp"))
	TObjectPtr<UDicePoolModel> mDicePool;

protected:
	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PlayerLevel"))
	int32 mPlayerLevel = 1;
};
