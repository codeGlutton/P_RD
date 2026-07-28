#include "Singleton/WorldSubsystem/WorldCameraModel.h"

DEFINE_LOG_CATEGORY(LogWorldCamera)

void UWorldCameraModel::RequestZoomInMainCamera(const FVector& Location, float ScreenSize)
{
	OnRequestZoomInMainCamera.Broadcast(Location, ScreenSize);
}

void UWorldCameraModel::RequestZoomOutMainCamera()
{
	OnRequestZoomOutMainCamera.Broadcast();
}

void UWorldCameraModel::RequestFocusMainCamera(const FVector& Location)
{
	OnRequestFocusMainCamera.Broadcast(Location);
}
