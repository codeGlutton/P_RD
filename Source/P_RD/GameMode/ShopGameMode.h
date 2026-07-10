/*****************************************************************//**
 * @file   ShopGameMode.h
 * @brief  상점 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "ShopGameMode.generated.h"

/**
 * @brief  상점 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API AShopGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

protected:
	void InitializeRoom() override;
};
