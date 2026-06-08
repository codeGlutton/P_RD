/*****************************************************************//**
 * @file   IntroGameMode.h
 * @brief  인트로 시네마틱 GameMode 정의 헤더
 * @date   2026-06-08
 *********************************************************************/

#pragma once

#include "GameMode/RDGameModeBase.h"

#include "IntroGameMode.generated.h"

class UCinematicWidget;

/**
 * @brief 프론트엔드 진입 전 인트로 시네마틱을 실행하는 GameMode
 */
UCLASS()
class P_RD_API AIntroGameMode : public ARDGameModeBase
{
	GENERATED_BODY()

protected:
	void BeginRoom() override;
};
