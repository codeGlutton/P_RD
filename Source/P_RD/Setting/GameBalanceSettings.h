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
	UPROPERTY(Config, Category = Attribute, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "InitializeCurveTable", ToolTip = "Attribute 초기 값 테이블", ConfigRestartRequired = true))
	TSoftObjectPtr<UCurveTable> mInitializeCurveTable;

	UPROPERTY(Config, Category = StatusEffect, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "GlobalStatusEffectSetting", ToolTip = "상태 이상 적용 시, 사용되는 전역 설정 값"))
	FGlobalStatusEffectSetting mGlobalStatusEffectSetting;
};
