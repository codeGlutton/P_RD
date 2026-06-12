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

void UStaticTreasureRoomSpawnData::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	const TSoftClassPtr<AGameModeBase> UpdatedGameMode = GetDefault<UGamePlaySettings>()->mTreasureGameMode;
	if (mGameModeBase != UpdatedGameMode)
	{
		Modify();
		mGameModeBase = UpdatedGameMode;
		MarkPackageDirty();
	}
#endif
}