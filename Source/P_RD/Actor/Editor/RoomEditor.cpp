#include "Actor/Editor/RoomEditor.h"
#include "Components/ChildActorComponent.h"
#include "Actor/TileMap/TileMap.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"

#include "Pawn/Unit.h"
#include "Pawn/Player/PlayerUnit.h"
#include "Actor/BoardActor/Obstacle/Obstacle.h"

ARoomEditor::ARoomEditor()
{
	PrimaryActorTick.bCanEverTick = false;

	/* 루트 컴포넌트 생성 */

	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComp);

	/* 타일맵 자식 액터 컴포넌트 생성 */

	mTileMapComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("TileMapComponent"));
	mTileMapComponent->SetupAttachment(RootComp);
	mTileMapComponent->SetChildActorClass(ATileMap::StaticClass());
}

void ARoomEditor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	/* 방 배치 갱신 */

	RefreshRoom();
}

void ARoomEditor::Destroyed()
{
	/* 스폰 액터 정돈 */

	CleanUpSpawnedActors();

	Super::Destroyed();
}

void ARoomEditor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	/* 스폰 액터 정돈 */

	CleanUpSpawnedActors();

	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void ARoomEditor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	/* 방 배치 갱신 */

	RefreshRoom();
}
#endif

void ARoomEditor::CleanUpSpawnedActors()
{
	/* 이전 스폰된 액터 제거 */

	for (AActor* SpawnedActor : mSpawnedActors)
	{
		if (SpawnedActor != nullptr)
		{
			SpawnedActor->Destroy();
		}
	}

	mSpawnedActors.Empty();
}

void ARoomEditor::RefreshRoom()
{
	/* 이전 생성 액터 청소 */

	CleanUpSpawnedActors();

	/* 월드 유효성 검사 */

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	/* 타일맵 액터 검사 */

	if (mTileMapComponent == nullptr)
	{
		return;
	}

	ATileMap* TileMap = Cast<ATileMap>(mTileMapComponent->GetChildActor());
	if (TileMap == nullptr)
	{
		return;
	}

	/* 전투 방 데이터 검사 */

	if (mRoomSpawnData == nullptr)
	{
		return;
	}

	/* 장애물 배치 처리 */

	for (const FObstaclePlacementData& ObstaclePlacement : mRoomSpawnData->mObstaclePlacementDatas)
	{
		UStaticObstacleSpawnData* ObstacleSpawnData = ObstaclePlacement.mSpawnData.LoadSynchronous();
		if (ObstacleSpawnData == nullptr)
		{
			continue;
		}

		UClass* ViewClass = ObstacleSpawnData->mViewClass.LoadSynchronous();
		if (ViewClass == nullptr)
		{
			continue;
		}

		const FTransform WorldTransform = TileMap->TileToWorldTransform(ObstaclePlacement.mTransform);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags |= RF_Transient;

		AObstacle* SpawnedActor = World->SpawnActor<AObstacle>(ViewClass, WorldTransform, SpawnParams);
		if (SpawnedActor != nullptr)
		{
			SpawnedActor->AddActorWorldOffset(FVector(0, 0, SpawnedActor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
			SpawnedActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			mSpawnedActors.Add(SpawnedActor);
		}
	}

	/* 적 유닛 배치 처리 */

	for (const FEnemyUnitPlacementData& EnemyPlacement : mRoomSpawnData->mEnemyUnitPlacementDatas)
	{
		UStaticEnemyUnitSpawnData* EnemySpawnData = EnemyPlacement.mSpawnData.LoadSynchronous();
		if (EnemySpawnData == nullptr)
		{
			continue;
		}

		UClass* ViewClass = EnemySpawnData->mViewClass.LoadSynchronous();
		if (ViewClass == nullptr)
		{
			continue;
		}

		const FTransform WorldTransform = TileMap->TileToWorldTransform(EnemyPlacement.mTransform);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags |= RF_Transient;

		AUnit* SpawnedActor = World->SpawnActor<AUnit>(ViewClass, WorldTransform, SpawnParams);
		if (SpawnedActor != nullptr)
		{
			SpawnedActor->GetMesh()->SetUpdateAnimationInEditor(true);
			SpawnedActor->AddActorWorldOffset(FVector(0, 0, SpawnedActor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
			SpawnedActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			mSpawnedActors.Add(SpawnedActor);
		}
	}

	/* 아군 유닛 배치 처리 */

	if (mTestPlayerUnit == nullptr)
	{
		return;
	}

	for (const FTileTransform& PlayerTileTransform : mRoomSpawnData->mPlayerTransforms)
	{
		const FTransform WorldTransform = TileMap->TileToWorldTransform(PlayerTileTransform);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags |= RF_Transient;

		AUnit* SpawnedActor = World->SpawnActor<AUnit>(mTestPlayerUnit, WorldTransform, SpawnParams);
		if (SpawnedActor != nullptr)
		{
			SpawnedActor->GetMesh()->SetUpdateAnimationInEditor(true);
			SpawnedActor->AddActorWorldOffset(FVector(0, 0, SpawnedActor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
			SpawnedActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			mSpawnedActors.Add(SpawnedActor);
		}
	}
}
