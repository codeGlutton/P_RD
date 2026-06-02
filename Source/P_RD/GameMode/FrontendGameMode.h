/*****************************************************************//**
 * @file   FrontendGameMode.h
 * @brief  프론트엔드 GameMode 정의 헤더
 * @author Codex
 * @date   2026-06-02
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"

#include "FrontendGameMode.generated.h"

/**
 * @brief 타이틀 메인 화면을 표시하는 프론트엔드 GameMode
 */
UCLASS()
class P_RD_API AFrontendGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

public:
	AFrontendGameMode();

protected:
	void InitializeCommonRoom() override;
	void BeginRoom() override;
};
