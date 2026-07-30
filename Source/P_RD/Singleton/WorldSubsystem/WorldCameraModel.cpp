#include "Singleton/WorldSubsystem/WorldCameraModel.h"

DEFINE_LOG_CATEGORY(LogWorldCamera)

void UWorldCameraModel::RequestZoomInMainCamera(const FVector& Location, float ScreenSize)
{
	// 카메라 뷰가 붙어 있을 때만 강조 중으로 센다(mIsMainCameraEmphasized 주석 참고).
	mIsMainCameraEmphasized = OnRequestZoomInMainCamera.IsBound();
	OnRequestZoomInMainCamera.Broadcast(Location, ScreenSize);
}

void UWorldCameraModel::RequestZoomOutMainCamera()
{
	// 복귀를 알릴 카메라가 없으면 그 자리에서 끝난 것으로 친다.
	if (OnRequestZoomOutMainCamera.IsBound() == false)
	{
		mIsMainCameraEmphasized = false;
	}
	OnRequestZoomOutMainCamera.Broadcast();
}

void UWorldCameraModel::NotifyMainCameraReturned()
{
	mIsMainCameraEmphasized = false;
	OnMainCameraReturned.Broadcast();
}
