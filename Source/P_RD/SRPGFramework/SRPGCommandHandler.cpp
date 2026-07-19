#include "SRPGFramework/SRPGCommandHandler.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "InputCoreTypes.h"
#include "ObjectView.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Actor/TileMap/TileMapModel.h"

#include "Actor/BoardActor/BoardActorModel.h"

void ISRPGCommandHandler::GetTileActorUnderCursor(UWorld* World, ECollisionChannel Channel, const FVector2D& ScreenPosition, OUT AActor*& Actor, OUT FTileIndex& TileIndex)
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
		bool bHit = false;

		// UI가 명시적인 화면 좌표를 실어 보냈다면 현재 마우스/손가락 위치보다 항상 우선한다.
		// 드래그 스냅은 손가락 아래 좌표가 아니라 HUD가 고른 유효 타일 중심을 보내므로,
		// GetHitResultUnderCursor를 먼저 쓰면 보이는 착지칸과 실제 판정칸이 달라진다.
		if (ScreenPosition.X >= 0.0 && ScreenPosition.Y >= 0.0)
		{
			FVector2D ViewportPixel = FVector2D::ZeroVector;
			FVector2D ViewportDPIScaled = FVector2D::ZeroVector;
			USlateBlueprintLibrary::AbsoluteToViewport(
				World, ScreenPosition, OUT ViewportPixel, OUT ViewportDPIScaled);
			bHit = PlayerController->GetHitResultAtScreenPosition(
				ViewportPixel, Channel, false, HitResult);
		}
		else
		{
			// 좌표를 싣지 않는 레거시 호출만 실제 커서/첫 번째 손가락을 사용한다.
			bHit = PlayerController->GetHitResultUnderCursor(Channel, false, HitResult);
			if (bHit == false)
			{
				bHit = PlayerController->GetHitResultUnderFinger(
					ETouchIndex::Touch1, Channel, false, HitResult);
			}
		}

		if (bHit == true)
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

void ISRPGCommandHandler::GetTileActorUnderCursor(UWorld* World, ECollisionChannel Channel, OUT AActor*& Actor, OUT FTileIndex& TileIndex)
{
	GetTileActorUnderCursor(World, Channel, FVector2D(-1.0, -1.0), OUT Actor, OUT TileIndex);
}
