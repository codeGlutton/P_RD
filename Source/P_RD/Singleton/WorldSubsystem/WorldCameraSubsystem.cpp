#include "Singleton/WorldSubsystem/WorldCameraSubsystem.h"
#include "Singleton/WorldSubsystem/WorldCameraModel.h"

#include "Pawn/Camera/CombatCameraPawn.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"

namespace
{
	UCameraMovementComponent* FindMainCameraMovementComponent(UWorld* World)
	{
		if (World == nullptr)
		{
			return nullptr;
		}

		APlayerController* PlayerController = World->GetFirstPlayerController();
		ACombatCameraPawn* MainCameraPawn =
			PlayerController != nullptr
			? PlayerController->GetPawn<ACombatCameraPawn>()
			: nullptr;
		return MainCameraPawn != nullptr
			? MainCameraPawn->GetCameraMovementComponent()
			: nullptr;
	}
}

void UWorldCameraSubsystem::BindModel(UObjectModel* Model)
{
	mWorldCameraModel = Cast<UWorldCameraModel>(Model);

	if (mWorldCameraModel != nullptr)
	{
		mWorldCameraModel->OnRequestZoomInMainCamera.AddWeakLambda(this, [this](const FVector& Location, float ScreenSize) {
			if (UCameraMovementComponent* CameraMovementComponent =
				FindMainCameraMovementComponent(GetWorld()))
			{
				CameraMovementComponent->StartEmphasisToWorldPositionWithZoom(ScreenSize, Location);
				if (UWorldCameraModel* WorldCameraModel = mWorldCameraModel.Get())
				{
					WorldCameraModel->NotifyMainCameraEmphasisStarted();
				}
			}
			});
		mWorldCameraModel->OnRequestZoomOutMainCamera.AddWeakLambda(this, [this]() {
			if (UCameraMovementComponent* CameraMovementComponent =
				FindMainCameraMovementComponent(GetWorld()))
			{
				CameraMovementComponent->EndEmphasis();
			}
			else
			{
				// 강조 뒤 카메라가 제거됐다면 돌아올 애니메이션도 없다.
				if (UWorldCameraModel* WorldCameraModel = mWorldCameraModel.Get())
				{
					WorldCameraModel->NotifyMainCameraReturned();
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

