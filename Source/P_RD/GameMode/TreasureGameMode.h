/*****************************************************************//**
 * @file   TreasureGameMode.h
 * @brief  보상 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "TreasureGameMode.generated.h"

/**
 * @brief  보상 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API ATreasureGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

public:
	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};
