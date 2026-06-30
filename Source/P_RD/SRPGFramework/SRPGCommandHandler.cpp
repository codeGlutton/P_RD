#include "SRPGFramework/SRPGCommandHandler.h"

#include "ObjectView.h"

#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"

void ISRPGCommandHandler::GetTileActorUnderCursor(UWorld* World, ECollisionChannel Channel, OUT AActor*& Actor, OUT FTileIndex& TileIndex)
{
	check(World != nullptr);

	Actor = nullptr;
	TileIndex = FTileIndex::Invalid;

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(World);
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController == nullptr)
	{
		return;
	}

	FHitResult HitResult;
	if (PlayerController->GetHitResultUnderCursor(Channel, false, HitResult) == false)
	{
		return;
	}

	Actor = HitResult.GetActor();
	if (Actor == CombatModel->GetTileMap()->GetView<AActor>())
	{
		TileIndex = CombatModel->GetTileMap()->WorldToTileIndex(HitResult.ImpactPoint);
		return;
	}

	IObjectView* ObjectView = Cast<IObjectView>(HitResult.GetActor());
	if (ObjectView == nullptr)
	{
		return;
	}

	UBoardActorModel* BoardActorModel = ObjectView->GetModel<UBoardActorModel>();
	if (BoardActorModel != nullptr)
	{
		TileIndex = BoardActorModel->GetTileTransform().mIndex;
	}
}
