#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "GameMode/CombatGameMode.h"

#include "Setting/GamePlaySettings.h"

void UStaticCombatRoomSpawnData::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		mGameModeBase = GetDefault<UGamePlaySettings>()->mCombatGameMode;
	}
}
