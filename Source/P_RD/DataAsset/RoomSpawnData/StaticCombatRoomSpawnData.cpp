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

void UStaticCombatRoomSpawnData::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	const TSoftClassPtr<AGameModeBase> UpdatedGameMode = GetDefault<UGamePlaySettings>()->mCombatGameMode;
	if (mGameModeBase != UpdatedGameMode)
	{
		Modify();
		mGameModeBase = UpdatedGameMode;
		MarkPackageDirty();
	}
#endif
}
