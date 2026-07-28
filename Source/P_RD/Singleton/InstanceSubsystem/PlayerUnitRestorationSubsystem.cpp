#include "Singleton/InstanceSubsystem/PlayerUnitRestorationSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "Engine/AssetManager.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "Simulation/Factory/ObjectModelFactory.h"

DEFINE_LOG_CATEGORY(LogPlayerUnitRestoration)

/**
 * @brief 유닛 하나를 세운다.
 * @param World       스폰할 월드
 * @param PlayerUnitId 세울 유닛의 식별자
 * @return 만들어진 유닛 모델. 데이터를 못 찾으면 nullptr
 */
UPlayerUnitModel* UPlayerUnitRestorationSubsystem::SpawnOne(UWorld* World, const FPrimaryAssetId& PlayerUnitId) const
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	UStaticPlayerUnitSpawnData* StaticPlayerUnitSpawnData = AssetManager->GetPrimaryAssetObject<UStaticPlayerUnitSpawnData>(PlayerUnitId);
	if (StaticPlayerUnitSpawnData == nullptr)
	{
		UE_LOG(LogPlayerUnitRestoration, Warning, TEXT("플레이어 유닛 스폰 데이터를 못 찾음"));
		return nullptr;
	}

	// mModelClass는 TSoftClassPtr라 .Get()은 이미 로드된 경우만 유효(미로드 시 nullptr → 팩토리가 추상 UPlayerUnitModel로 폴백해 크래시).
	// 반드시 동기 로드해서 구체 BP 클래스를 넘긴다.
	UClass* ModelClass = StaticPlayerUnitSpawnData->mModelClass.LoadSynchronous();
	checkf(ModelClass != nullptr, TEXT("플레이어 유닛 ModelClass 로드 실패 — DataAsset의 mModelClass 확인"));

	UPlayerUnitModel* UnitModel = GetWorldModelFactory(this)->NewModelDeferred<UPlayerUnitModel>(ModelClass);
	UnitModel->SetStaticSpawnData(StaticPlayerUnitSpawnData);
	UnitModel->FinishCreating();

	return Cast<UPlayerUnitModel>(UnitModel);
}

/**
 * @brief 저장된 파티 전원을 스폰한다.
 *
 * @details
 * 못 세운 사람은 조용히 빠진다. 데이터가 하나 없다고 나머지 둘까지 방에 못
 * 들어가게 하면, 자산 하나가 빠진 날 게임이 통째로 멈춘다.
 * @param World 스폰할 월드
 * @return 저장본에 적힌 차례대로의 유닛들
 */
TArray<UPlayerUnitModel*> UPlayerUnitRestorationSubsystem::SpawnPartyUnits(UWorld* World) const
{
	checkf(World != nullptr, TEXT("월드 nullptr"));

	TArray<UPlayerUnitModel*> Units;
	for (const FPrimaryAssetId& PlayerUnitId : GetRunMutableData()->GetPartyUnitIds())
	{
		if (UPlayerUnitModel* Unit = SpawnOne(World, PlayerUnitId))
		{
			Units.Add(Unit);
		}
	}

	UE_LOG(LogPlayerUnitRestoration, Log, TEXT("플레이어 유닛 %d명 스폰"), Units.Num());
	return Units;
}

void UPlayerUnitRestorationSubsystem::RegisterPlayerUnit(UPlayerUnitModel* PlayerUnit) const
{
	GetRunMutableData()->RegisterPlayerUnit(PlayerUnit);
	UE_LOG(LogPlayerUnitRestoration, Log, TEXT("플레이어 유닛 초기화"));
}
