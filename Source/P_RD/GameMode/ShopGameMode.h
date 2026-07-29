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
class UShopUIModel;
struct FShopItemList;

/**
 * @brief  상점 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API AShopGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

public:
	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/**
	 * @brief 상점 화면이 읽을 뷰모델.
	 *
	 * @details
	 * 보상과 같은 규칙이다 -- 게임모드가 SetShop() 으로 밀고, 위젯은 GetShop()
	 * 으로 읽고, 사고 나가는 것은 의도만 보낸다. 골드를 깎고 품절을 매기는 것은
	 * 전부 이쪽이다.
	 *
	 * 방마다 여는 화면이 아니라 이 방의 화면이라, 월드 위젯이 아니라 여기에
	 * 둔다. @return 뷰모델
	 */
	UFUNCTION(BlueprintPure, Category = "Shop")
	UShopUIModel* GetShopUIModel() const { return mShopUIModel; }

protected:
	void InitializeRoom() override;
	void BeginRoom() override;

private:
	/**
	 * @brief 상점 스폰포인트 위치에 타일맵을 깐다.
	 */
	void SpawnTileMap();

	/** @brief 지금 파는 것과 가진 돈을 화면에 내린다. */
	void PushShopUIData() const;

	/** @brief 한 칸을 샀다. 돈을 깎고 품절로 매긴 뒤 다시 내린다. */
	UFUNCTION() void HandleBuyRequested(int32 SlotIndex);

	/** @brief 나간다. 지도를 열어 다음 방을 고르게 한다. */
	UFUNCTION() void HandleLeaveRequested();

	/** @brief 지금 가진 돈. @return 없으면 0 */
	int32 GetPartyGold() const;

private:
	// @brief 상점방 타일맵 모델 (뷰 액터는 모델 팩토리가 함께 스폰)
	UPROPERTY(Transient)
	TObjectPtr<UTileMapModel> mTileMap;

	UPROPERTY(Transient)
	TObjectPtr<UShopUIModel> mShopUIModel;

	/**
	 * @brief 이미 팔린 칸.
	 *
	 * @details
	 * 방 데이터(FShopRoom)를 지우지 않는다. 그쪽은 저장되는 값이라 여기서
	 * 손대면 되돌릴 길이 없다 -- 이 방에 다시 들어올 일이 없더라도, 사는 것을
	 * 무르는 규칙이 나중에 생기면 그때 곤란해진다.
	 */
	TSet<int32> mSoldSlots;
};
