#include "Singleton/WorldSubsystem/WorldCameraSubsystem.h"
#include "Singleton/WorldSubsystem/WorldCameraModel.h"

#include "Pawn/Camera/CombatCameraPawn.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"

void UWorldCameraSubsystem::BindModel(UObjectModel* Model)
{
	// 재바인딩 시 이전 모델에 남아 있던 약한 람다도 제거해 중복 카메라 요청을 막는다.
	if (UWorldCameraModel* PreviousModel = mWorldCameraModel.Get())
	{
		PreviousModel->OnRequestZoomInMainCamera.RemoveAll(this);
		PreviousModel->OnRequestZoomOutMainCamera.RemoveAll(this);
	}

	mWorldCameraModel = Cast<UWorldCameraModel>(Model);

	if (mWorldCameraModel != nullptr)
	{
		mWorldCameraModel->OnRequestZoomInMainCamera.AddWeakLambda(this, [this](const FVector& Location, float ScreenSize) {
			UWorld* World = GetWorld();
			if (World == nullptr || World->bIsTearingDown)
			{
				return;
			}
			APlayerController* PlayerController = World->GetFirstPlayerController();
			ACombatCameraPawn* MainCameraPawn = PlayerController != nullptr ? PlayerController->GetPawn<ACombatCameraPawn>() : nullptr;
			if (MainCameraPawn != nullptr)
			{
				UCameraMovementComponent* CameraMovementComponent = MainCameraPawn->GetCameraMovementComponent();
				if (CameraMovementComponent != nullptr)
				{
					CameraMovementComponent->StartEmphasisToWorldPositionWithZoomDelta(ScreenSize, Location);
				}
			}
			});
		mWorldCameraModel->OnRequestZoomOutMainCamera.AddWeakLambda(this, [this]() {
			UWorld* World = GetWorld();
			if (World == nullptr || World->bIsTearingDown)
			{
				return;
			}
			APlayerController* PlayerController = World->GetFirstPlayerController();
			ACombatCameraPawn* MainCameraPawn = PlayerController != nullptr ? PlayerController->GetPawn<ACombatCameraPawn>() : nullptr;
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
	if (UWorldCameraModel* BoundModel = mWorldCameraModel.Get())
	{
		BoundModel->OnRequestZoomInMainCamera.RemoveAll(this);
		BoundModel->OnRequestZoomOutMainCamera.RemoveAll(this);
	}
	mWorldCameraModel.Reset();
}

void UWorldCameraSubsystem::Deinitialize()
{
	UnbindModel(nullptr);
	Super::Deinitialize();
}

UObjectModel* UWorldCameraSubsystem::GetModel_Internal() const
{
	return mWorldCameraModel.Get();
}

