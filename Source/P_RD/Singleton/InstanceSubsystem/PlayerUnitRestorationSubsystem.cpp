#include "Singleton/InstanceSubsystem/PlayerUnitRestorationSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "Engine/AssetManager.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "Simulation/Factory/ObjectModelFactory.h"

DEFINE_LOG_CATEGORY(LogPlayerUnitRestoration)

UPlayerUnitModel* UPlayerUnitRestorationSubsystem::SpawnPlayerUnit(UWorld* World) const
{
	checkf(World != nullptr, TEXT("월드 nullptr"));

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	UStaticPlayerUnitSpawnData* StaticPlayerUnitSpawnData = AssetManager->GetPrimaryAssetObject<UStaticPlayerUnitSpawnData>(GetRunMutableData()->GetPlayerUnitId());
	
	UPlayerUnitModel* UnitModel = GetWorldModelFactory(this)->NewModelDeferred<UPlayerUnitModel>(StaticPlayerUnitSpawnData->mModelClass.Get());
	UnitModel->SetStaticSpawnData(StaticPlayerUnitSpawnData);
	UnitModel->FinishCreating();
	
	UE_LOG(LogPlayerUnitRestoration, Log, TEXT("플레이어 유닛 스폰"));

	return Cast<UPlayerUnitModel>(UnitModel);
}

void UPlayerUnitRestorationSubsystem::RegisterPlayerUnit(UPlayerUnitModel* PlayerUnit) const
{
	GetRunMutableData()->RegisterPlayerUnit(PlayerUnit);
	UE_LOG(LogPlayerUnitRestoration, Log, TEXT("플레이어 유닛 초기화"));
}
