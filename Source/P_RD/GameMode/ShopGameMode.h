/*****************************************************************//**
 * @file   ShopGameMode.h
 * @brief  상점 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "ShopGameMode.generated.h"

class UTileMapModel;

/**
 * @brief  상점 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API AShopGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

public:
	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

protected:
	void InitializeRoom() override;

private:
	/**
	 * @brief 상점 스폰포인트 위치에 타일맵을 깐다.
	 */
	void SpawnTileMap();

private:
	// @brief 상점방 타일맵 모델 (뷰 액터는 모델 팩토리가 함께 스폰)
	UPROPERTY(Transient)
	TObjectPtr<UTileMapModel> mTileMap;
};
