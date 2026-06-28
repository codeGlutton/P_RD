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
	checkf(StaticPlayerUnitSpawnData != nullptr, TEXT("플레이어 유닛 스폰 데이터 nullptr"));

	// mModelClass는 TSoftClassPtr라 .Get()은 이미 로드된 경우만 유효(미로드 시 nullptr → 팩토리가 추상 UPlayerUnitModel로 폴백해 크래시).
	// 반드시 동기 로드해서 구체 BP 클래스를 넘긴다.
	UClass* ModelClass = StaticPlayerUnitSpawnData->mModelClass.LoadSynchronous();
	checkf(ModelClass != nullptr, TEXT("플레이어 유닛 ModelClass 로드 실패 — DataAsset의 mModelClass 확인"));

	UPlayerUnitModel* UnitModel = GetWorldModelFactory(this)->NewModelDeferred<UPlayerUnitModel>(ModelClass);
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
