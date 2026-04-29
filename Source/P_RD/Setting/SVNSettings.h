/*****************************************************************//**
 * @file   SVNSettings.h
 * @brief  SVN과 연결하기 위한 개인 설정 세팅 창 연관 헤더
 * @author 모호재
 * @date   2026-04-23
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SVNSettings.generated.h"

/**
 * @brief  SVN 연결 세팅 옵션
 * @details 
 * SVN 코드를 설정하기 위해, 에디터의 개인 에디터 세팅 창에 뜰 정보 구성 객체입니다. \n
 * Ignore폴더인 Save폴더 내부에 설정이 저장되어 형상 관리 대상으로 지정되지 않습니다.
 */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "SVN Link Option Editing"))
class P_RD_API USVNSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USVNSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/* UDeveloperSettings 상속 */
public:
	FName GetCategoryName() const override;

#if WITH_EDITOR
	FText GetSectionText() const override;
	FText GetSectionDescription() const override;
#endif

public:
	/**
	 * @brief SVN Content 폴더를 Content 내부에 연결하기 위한 경로
	 * @details
	 * SVN Content를 가져오기 위해 Content 폴더에 Junction 생성합니다.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = SVNContent, meta = (DisplayName = "SVNContentDir", ToolTip = "SVN Content 폴더를 연결하기 위한 경로", ConfigRestartRequired = true))
	FDirectoryPath mSVNContentDir;
};
