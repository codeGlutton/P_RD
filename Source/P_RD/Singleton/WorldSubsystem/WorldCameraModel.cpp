#include "Singleton/WorldSubsystem/WorldCameraModel.h"

DEFINE_LOG_CATEGORY(LogWorldCamera)

void UWorldCameraModel::RequestZoomInMainCamera(const FVector& Location, float ScreenSize)
{
	OnRequestZoomInMainCamera.Broadcast(Location, ScreenSize);
}

void UWorldCameraModel::RequestZoomOutMainCamera()
{
	const bool HasCameraView = OnRequestZoomOutMainCamera.IsBound();
	OnRequestZoomOutMainCamera.Broadcast();

	// 강조를 시작한 뷰가 이미 사라졌다면 복귀 애니메이션도 없다.
	if (HasCameraView == false)
	{
		NotifyMainCameraReturned();
	}
}

void UWorldCameraModel::NotifyMainCameraEmphasisStarted()
{
	mIsMainCameraEmphasized = true;
}

void UWorldCameraModel::NotifyMainCameraReturned()
{
	if (mIsMainCameraEmphasized == false)
	{
		return;
	}

	mIsMainCameraEmphasized = false;
	OnMainCameraReturned.Broadcast();
}
