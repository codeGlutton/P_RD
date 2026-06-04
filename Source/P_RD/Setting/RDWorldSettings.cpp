#include "Setting/RDWorldSettings.h"

AActor* ARDWorldSettings::GetMainCameraPoint() const
{
	return mMainCameraPoint;
}

AActor* ARDWorldSettings::GetRoomStartPoint() const
{
	return mRoomStartPoint;
}
