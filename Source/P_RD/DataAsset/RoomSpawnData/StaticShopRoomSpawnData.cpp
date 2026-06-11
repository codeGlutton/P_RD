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

void UStaticShopRoomSpawnData::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	const TSoftClassPtr<AGameModeBase> UpdatedGameMode = GetDefault<UGamePlaySettings>()->mShopGameMode;
	if (mGameModeBase != UpdatedGameMode)
	{
		Modify();
		mGameModeBase = UpdatedGameMode;
		MarkPackageDirty();
	}
#endif
}
