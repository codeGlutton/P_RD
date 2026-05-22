#include "Singleton/InstanceSubsystem/PlayerUnitRestorationSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "Engine/AssetManager.h"
#include "Pawn/Player/PlayerUnit.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

DEFINE_LOG_CATEGORY(LogPlayerUnitRestoration)

APlayerUnit* UPlayerUnitRestorationSubsystem::SpawnPlayerUnit(UWorld* World) const
{
	checkf(World != nullptr, TEXT("월드 nullptr"));

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	const UStaticPlayerUnitSpawnData* StaticPlayerUnitSpawnData = AssetManager->GetPrimaryAssetObject<UStaticPlayerUnitSpawnData>(GetRunMutableData()->GetPlayerUnitId());
	AUnit* Unit = World->SpawnActor<AUnit>(StaticPlayerUnitSpawnData->mClass.Get());

	return Cast<APlayerUnit>(Unit);
}

void UPlayerUnitRestorationSubsystem::RegisterPlayerUnit(APlayerUnit* PlayerUnit) const
{
	GetRunMutableData()->RegisterPlayerUnit(PlayerUnit);
}
