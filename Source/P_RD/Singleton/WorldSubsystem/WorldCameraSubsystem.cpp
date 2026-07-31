#include "Singleton/WorldSubsystem/WorldCameraSubsystem.h"
#include "Singleton/WorldSubsystem/WorldCameraModel.h"

#include "Pawn/Camera/CombatCameraPawn.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"

void UWorldCameraSubsystem::BindModel(UObjectModel* Model)
{
	mWorldCameraModel = Cast<UWorldCameraModel>(Model);

	if (mWorldCameraModel != nullptr)
	{
		/*
		 * 컨트롤러는 널일 수 있다.
		 *
		 * 월드가 정리될 때 몽타쥬 종료 대리자 파괴 -> 배리어 해제 -> 스킬 종료
		 * -> 줌아웃 요청 순서로 이 람다가 불리는데, 그 시점에는 플레이어
		 * 컨트롤러가 이미 사라져 있을 수 있다 -- 스킬 연출 도중 게임을 끄면
		 * 실제로 그렇게 터졌다. 카메라를 움직여 줄 사람이 없으면 조용히
		 * 지나간다.
		 */
		mWorldCameraModel->OnRequestZoomInMainCamera.AddWeakLambda(this, [this](const FVector& Location, float ScreenSize) {
			APlayerController* PlayerController = GetWorld() != nullptr
				? GetWorld()->GetFirstPlayerController() : nullptr;
			ACombatCameraPawn* MainCameraPawn = PlayerController != nullptr
				? PlayerController->GetPawn<ACombatCameraPawn>() : nullptr;
			if (MainCameraPawn != nullptr)
			{
				UCameraMovementComponent* CameraMovementComponent = MainCameraPawn->GetCameraMovementComponent();
				if (CameraMovementComponent != nullptr)
				{
					CameraMovementComponent->StartEmphasisToWorldPositionWithZoom(ScreenSize, Location);
				}
			}
			});
		mWorldCameraModel->OnRequestZoomOutMainCamera.AddWeakLambda(this, [this]() {
			APlayerController* PlayerController = GetWorld() != nullptr
				? GetWorld()->GetFirstPlayerController() : nullptr;
			ACombatCameraPawn* MainCameraPawn = PlayerController != nullptr
				? PlayerController->GetPawn<ACombatCameraPawn>() : nullptr;
			if (MainCameraPawn != nullptr)
			{
				UCameraMovementComponent* CameraMovementComponent = MainCameraPawn->GetCameraMovementComponent();
				if (CameraMovementComponent != nullptr)
				{
					CameraMovementComponent->EndEmphasis();
				}
			}
			});
	}
}

void UWorldCameraSubsystem::UnbindModel(UObjectModel* Model)
{
	mWorldCameraModel.Reset();
}

UObjectModel* UWorldCameraSubsystem::GetModel_Internal() const
{
	return mWorldCameraModel.Get();
}

