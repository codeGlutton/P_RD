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
		mEmptyObstacleData = GetDefault<UGamePlaySettings>()->mEmptyObstacleData;
	}
}

void UStaticCombatRoomSpawnData::PostLoad()
{
	Super::PostLoad();

	const TSoftClassPtr<AGameModeBase> UpdatedGameMode = GetDefault<UGamePlaySettings>()->mCombatGameMode;
	const TSoftObjectPtr<UStaticObstacleSpawnData> UpdatedObstacleData = GetDefault<UGamePlaySettings>()->mEmptyObstacleData;
	if (mGameModeBase != UpdatedGameMode || mEmptyObstacleData != UpdatedObstacleData)
	{
#if WITH_EDITOR
		Modify();
#endif
		mGameModeBase = UpdatedGameMode;
		mEmptyObstacleData = UpdatedObstacleData;
#if WITH_EDITOR
		MarkPackageDirty();
#endif
	}
}
