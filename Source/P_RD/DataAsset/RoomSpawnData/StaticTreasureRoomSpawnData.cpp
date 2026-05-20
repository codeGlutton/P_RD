#include "DataAsset/RoomSpawnData/StaticTreasureRoomSpawnData.h"
#include "GameMode/TreasureGameMode.h"

#include "Setting/GamePlaySettings.h"

void UStaticTreasureRoomSpawnData::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		mGameModeBase = GetDefault<UGamePlaySettings>()->mTreasureGameMode;
	}
}
