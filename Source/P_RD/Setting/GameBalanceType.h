/*****************************************************************//**
 * @file   GameBalanceType.h
 * @brief  게임 밸런스 타입 대한 헤더
 * @author 모호재
 * @date   2026-05-13
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameBalanceType.generated.h"

/**
 * @brief 전역 스테이지 설정 값 객체
 */
USTRUCT(BlueprintType)
struct FGlobalStageBuildSetting
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Money, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "MonsterRewardMoney"))
	FInt32Interval mMonsterRewardMoney = {10, 30};
	UPROPERTY(Category = Money, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "EliteRewardMoney"))
	FInt32Interval mEliteRewardMoney = {30, 50};
	UPROPERTY(Category = Money, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "BossRewardMoney"))
	FInt32Interval mBossRewardMoney = {100, 100};
	UPROPERTY(Category = Money, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TreasureRewardMoney"))
	FInt32Interval mTreasureRewardMoney = {40, 60};

public:
	UPROPERTY(Category = Exp, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "MonsterRewardExp"))
	float mMonsterRewardExp = 50.f;
	UPROPERTY(Category = Exp, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "EliteRewardExp"))
	float mEliteRewardExp = 75.f;
	UPROPERTY(Category = Exp, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "BossRewardExp"))
	float mBossRewardExp = 100.f;

public:
	UPROPERTY(Category = Room, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "BottomShopColumnOffset"))
	int32 mBottomShopColumnOffset = 3;
	UPROPERTY(Category = Room, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TopShopColumnOffset"))
	int32 mTopShopColumnOffset = 4;
	UPROPERTY(Category = Room, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TreasureColumnInterval"))
	int32 mTreasureColumnInterval = 9;
};

/**
 * @brief 전역 상태 이상 설정 값 객체
 */
USTRUCT(BlueprintType)
struct FGlobalStatusEffectSetting
{
	GENERATED_BODY()

public:
	/**
	 * @brief 상태이상을 적용 시에 어느정도로 배율을 적용할지
	 */
	UPROPERTY(Category = StatusEffect, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "EffectRatios"))
	TMap<FGameplayTag, float> mEffectRatios;
};
