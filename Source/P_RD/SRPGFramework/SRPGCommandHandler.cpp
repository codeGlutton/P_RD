#include "SRPGFramework/SRPGCommandHandler.h"

#include "ObjectView.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"

void ISRPGCommandHandler::GetTileActorUnderCursor(UWorld* World, ECollisionChannel Channel, OUT AActor*& Actor, OUT FTileIndex& TileIndex)
{
	check(World != nullptr);

	Actor = nullptr;
	TileIndex = FTileIndex::Invalid;

	/* 마우스 포인트 지점 아래로 Raycast 검사 */

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(World);
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController != nullptr)
	{
		FHitResult HitResult;
		if (PlayerController->GetHitResultUnderCursor(Channel, false, HitResult) == true)
		{
			Actor = HitResult.GetActor();
			if (Actor == CombatModel->GetTileMap()->GetView<AActor>())
			{
				TileIndex = CombatModel->GetTileMap()->WorldToTileIndex(HitResult.ImpactPoint);
			}
			else
			{
				IObjectView* ObjectView = Cast<IObjectView>(HitResult.GetActor());
				if (ObjectView != nullptr)
				{
					UBoardActorModel* BoardActorModel = ObjectView->GetModel<UBoardActorModel>();
					TileIndex = BoardActorModel->GetTileTransform().mIndex;
				}
			}
		}
	}
}
