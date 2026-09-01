/*****************************************************************//**
 * @file   GameBalanceSettings.h
 * @brief  게임 밸런스 설정 클래스 정의 헤더
 * @author 모호재
 * @date   2026-05-06
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Setting/GameBalanceType.h"
#include "GameBalanceSettings.generated.h"

/**
 * @brief  게임 밸런스 설정 클래스
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Game Balance Setting"))
class P_RD_API UGameBalanceSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	/* UDeveloperSettings 상속 */
public:
	FName GetCategoryName() const override;

#if WITH_EDITOR
	FText GetSectionText() const override;
	FText GetSectionDescription() const override;
#endif

	/* 스테이지 구성 */
public:
	UPROPERTY(Config, Category = Stage, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "GlobalStageBuildSetting", ToolTip = "Stage 생성 시, 사용되는 전역 설정 값"))
	FGlobalStageBuildSetting mGlobalStageBuildSetting;

	UPROPERTY(Config, Category = Stage, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "StageBuildSettingTable", ToolTip = "Stage 생성 시, 사용되는 설정 값 테이블", RequiredAssetDataTags = "RowStructure=/Script/P_RD.StageBuilderParams"))
	TSoftObjectPtr<UDataTable> mStageBuildSettingTable;

	/* 전투 구성 */
public:
	UPROPERTY(Config, Category = Combat, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "InitializeCurveTable", ToolTip = "Attribute 초기 값 테이블", ConfigRestartRequired = true))
	TSoftObjectPtr<UCurveTable> mInitializeCurveTable;

	UPROPERTY(Config, Category = Combat, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "GlobalStatusEffectSetting", ToolTip = "상태 이상 적용 시, 사용되는 전역 설정 값"))
	FGlobalStatusEffectBalanceSetting mGlobalStatusEffectSetting;

public:
	UPROPERTY(Config, Category = Level, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "PlayerMaxLevel", ToolTip = "플레이어 최대 레벨 설정 값"))
	int32 mPlayerMaxLevel = 10;

public:
	UPROPERTY(Config, Category = SpeedPoint, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "RequiredSpeedPointForTurn", ToolTip = "턴을 소유하기 위해 소모되는 스피드 포인트 값"))
	int32 mRequiredSpeedPointForTurn = 10;

	UPROPERTY(Config, Category = SpeedPoint, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "RechargedSpeedPointLimits", ToolTip = "스피드 포인트 충전값 최소 최댓 값"))
	FInt32Interval mRechargedSpeedPointLimits = {5, 15};

	UPROPERTY(Config, Category = ActionPoint, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "RechargedActionPointLimits", ToolTip = "액션 포인트 충전값 최소 최댓 값"))
	FInt32Interval mRechargedActionPointLimits = {1, 20};

	/* 힐 구성 */
public:
	UPROPERTY(Config, Category = SystemHeal, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "BreakTimeHealRatio", ToolTip = "휴식 시, 회복되는 힐 비율"))
	float mBreakTimeHealRatio = 0.5f;
	UPROPERTY(Config, Category = SystemHeal, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "StageClearHealRatio", ToolTip = "클리어 시, 회복되는 힐 비율"))
	float mStageClearHealRatio = 1.f;
};
