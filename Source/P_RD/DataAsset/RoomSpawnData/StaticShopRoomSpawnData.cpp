#include "DataAsset/RoomSpawnData/StaticShopRoomSpawnData.h"
#include "GameMode/ShopGameMode.h"

#include "Setting/GamePlaySettings.h"

void UStaticShopRoomSpawnData::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		mGameModeBase = GetDefault<UGamePlaySettings>()->mShopGameMode;
	}
}
