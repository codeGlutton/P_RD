#include "Singleton/WorldSubsystem/WorldCameraSubsystem.h"
#include "Singleton/WorldSubsystem/WorldCameraModel.h"

#include "Pawn/Camera/CombatCameraPawn.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"

void UWorldCameraSubsystem::BindModel(UObjectModel* Model)
{
	mWorldCameraModel = Cast<UWorldCameraModel>(Model);

	if (mWorldCameraModel != nullptr)
	{
		mWorldCameraModel->OnRequestZoomInMainCamera.AddWeakLambda(this, [this](const FVector& Location, float ScreenSize) {
			ACombatCameraPawn* MainCameraPawn = GetWorld()->GetFirstPlayerController()->GetPawn<ACombatCameraPawn>();
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
			ACombatCameraPawn* MainCameraPawn = GetWorld()->GetFirstPlayerController()->GetPawn<ACombatCameraPawn>();
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

