#include "SRPGFramework/SRPGCommandHandler.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "InputCoreTypes.h"
#include "ObjectView.h"
#include "SRPGFramework/SRPGCommand.h"

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

		// PC: 마우스 커서 지점을 트레이스한다.
		bool bHit = PlayerController->GetHitResultUnderCursor(Channel, false, HitResult);

		// 모바일: 마우스 커서가 없어 위 트레이스가 실패한다.
		if (bHit == false)
		{
			// 1) 손가락 추적(터치 다운 시점): 엔진이 실제 터치 위치를 직접 추적하므로 DPI/역투영 보정이 필요 없다(정석).
			bHit = PlayerController->GetHitResultUnderFinger(ETouchIndex::Touch1, Channel, false, HitResult);

			// 2) 폴백: 손가락 추적이 비어있으면, 커맨드가 실어온 화면 좌표로 직접 트레이스한다.
			//    GetHitResultAtScreenPosition은 뷰포트 픽셀을 기대하므로 Slate 절대좌표를 뷰포트 픽셀로 변환해 넘긴다(고DPI 어긋남 방지).
			if (bHit == false && ScreenPosition.X >= 0.0 && ScreenPosition.Y >= 0.0)
			{
				FVector2D ViewportPixel = FVector2D::ZeroVector;
				FVector2D ViewportDPIScaled = FVector2D::ZeroVector;
				USlateBlueprintLibrary::AbsoluteToViewport(World, ScreenPosition, OUT ViewportPixel, OUT ViewportDPIScaled);
				bHit = PlayerController->GetHitResultAtScreenPosition(ViewportPixel, Channel, false, HitResult);
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

void ISRPGCommandHandler::GetTileActorForCommand(UWorld* World,
	ECollisionChannel Channel, const FSRPGWorldTraceCommand& Command,
	OUT AActor*& Actor, OUT FTileIndex& TileIndex)
{
	check(World != nullptr);

	if (Command.mResolvedTileIndex == FTileIndex::Invalid)
	{
		GetTileActorUnderCursor(World, Channel, Command.mScreenPosition,
			OUT Actor, OUT TileIndex);
		return;
	}

	Actor = nullptr;
	TileIndex = FTileIndex::Invalid;
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(World);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	if (TileMap == nullptr || !TileMap->IsValidIndex(Command.mResolvedTileIndex))
	{
		return;
	}

	TileIndex = Command.mResolvedTileIndex;
	Actor = TileMap->GetView<AActor>();
	if (UBoardActorModel* Occupant =
		TileMap->GetActorOnTile<UBoardActorModel>(TileIndex))
	{
		Actor = Occupant->GetView<AActor>();
	}
}
