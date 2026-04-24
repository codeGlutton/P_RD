/*****************************************************************//**
 * @file   SVNSettings.h
 * @brief  SVN과 연결하기 위한 개인 설정 세팅 창 코드 구성
 * 
 * @author 모호재
 * @date   2026-04-23
 *********************************************************************/

#pragma once

#include "GlobalInclude.h"
#include "Engine/DeveloperSettings.h"
#include "SVNSettings.generated.h"

/**
 * @brief  SVN 연결 세팅 옵션. Save폴더에 저장되는 개인별 세팅 항목
 */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "SVN Link Option Editing"))
class P_RD_API USVNSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USVNSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Delveloper Setting 상속
public:
	virtual FName GetCategoryName() const override;

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#endif

public:
	/**
	 * @brief SVN Content 폴더를 연결하기 위한 경로
	 */
	UPROPERTY(config, EditAnywhere, Category = SVNContent, meta = (DisplayName = "SVNContentDir", ToolTip = "SVN Content 폴더를 연결하기 위한 경로", ConfigRestartRequired = true))
	FDirectoryPath mSVNContentDir;
};
