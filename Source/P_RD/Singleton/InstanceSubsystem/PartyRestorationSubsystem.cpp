#include "Singleton/InstanceSubsystem/PartyRestorationSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "Actor/Party/PartyModel.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "Engine/AssetManager.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "Simulation/Factory/ObjectModelFactory.h"

DEFINE_LOG_CATEGORY(LogPartyRestoration)

UPartyModel* UPartyRestorationSubsystem::RestorePartyFromPersistData(UWorld* World) const
{
	checkf(World != nullptr, TEXT("월드 nullptr"));

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	/* 파티 스폰 */
	
	UPartyModel* PartyModel = GetWorldModelFactory(this)->NewModel<UPartyModel>();

	/* 멤버 스폰 */

	TArray<TObjectPtr<UPlayerUnitModel>> PlayerUnitModels;
	const TArray<FPrimaryAssetId> PlayerUnitIds = GetRunMutableData()->GetPlayerUnitIds();
	PlayerUnitModels.Reserve(PlayerUnitIds.Num());
	for (const FPrimaryAssetId& PlayerUnitId : PlayerUnitIds)
	{
		UPlayerUnitModel* PlayerUnitModel = nullptr;
		UStaticPlayerUnitSpawnData* StaticPlayerUnitSpawnData = AssetManager->GetPrimaryAssetObject<UStaticPlayerUnitSpawnData>(PlayerUnitId);
		if (StaticPlayerUnitSpawnData != nullptr)
		{
			UClass* ModelClass = StaticPlayerUnitSpawnData->mModelClass.LoadSynchronous();
			checkf(ModelClass != nullptr, TEXT("플레이어 유닛 ModelClass 로드 실패"));

			PlayerUnitModel = GetWorldModelFactory(this)->NewModelDeferred<UPlayerUnitModel>(ModelClass);
			PlayerUnitModel->SetStaticSpawnData(StaticPlayerUnitSpawnData);
			PlayerUnitModel->FinishCreating();

			UE_LOG(LogPartyRestoration, Log, TEXT("플레이어 유닛 스폰"));
		}
		PlayerUnitModels.Add(PlayerUnitModel);
	}

	/* 복원 */

	GetRunMutableData()->RegisterParty(PartyModel, PlayerUnitModels);
	UE_LOG(LogPartyRestoration, Log, TEXT("파티 복원"));

	return PartyModel;
}

