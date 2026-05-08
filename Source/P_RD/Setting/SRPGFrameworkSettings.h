/*****************************************************************//**
 * @file   SRPGFrameworkSettings.h
 * @brief  SRPG 프레임워크 설정 클래스 정의 헤더
 * @author 모호재
 * @date   2026-05-06
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SRPGFrameworkSettings.generated.h"

/**
 * @brief  SRPG 프레임워크 설정 클래스
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "SRPG Framework Setting"))
class P_RD_API USRPGFrameworkSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
	/* UDeveloperSettings 상속 */
public:
	FName GetCategoryName() const override;

#if WITH_EDITOR
	FText GetSectionText() const override;
	FText GetSectionDescription() const override;
#endif
};
