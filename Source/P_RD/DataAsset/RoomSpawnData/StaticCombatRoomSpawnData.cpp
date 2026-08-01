#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "GameMode/CombatGameMode.h"

#include "Setting/GamePlaySettings.h"

UStaticCombatRoomSpawnData::UStaticCombatRoomSpawnData()
{
	mPlayerTransforms.Init(FTileTransform(), 3);
}

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

	const TSoftClassPtr<AGameModeBase> UpdatedGameMode = GetDefault<UGamePlaySettings>()->mCombatGameMode;
	if (mGameModeBase != UpdatedGameMode)
	{
#if WITH_EDITOR
		Modify();
#endif
		mGameModeBase = UpdatedGameMode;
#if WITH_EDITOR
		MarkPackageDirty();
#endif
	}
}
