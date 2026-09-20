#include "Animation/Notify/EventTriggerPayload.h"

void FCameraZoomEventData::CalculateCameraZoomScaleAndLocation(const TArray<FVector>& Locations, OUT float& OutZoomScale, OUT FVector& OutLocation) const
{
	if (Locations.IsEmpty() == true)
	{
		return;
	}

	static const FRotator CameraRot(-45.f, 45.f, 0.f);
	static const FVector CameraUp = CameraRot.RotateVector(FVector::UpVector);
	static const FVector CameraRight = CameraRot.RotateVector(FVector::RightVector);

	float MinRight = TNumericLimits<float>::Max();
	float MaxRight = TNumericLimits<float>::Lowest();
	float MinUp = TNumericLimits<float>::Max();
	float MaxUp = TNumericLimits<float>::Lowest();
	FVector LocationSum = FVector::ZeroVector;

	for (const FVector& Location : Locations)
	{
		LocationSum += Location;

		const float Up = FVector::DotProduct(Location, CameraUp);
		const float Right = FVector::DotProduct(Location, CameraRight);

		MinRight = FMath::Min(MinRight, Right);
		MaxRight = FMath::Max(MaxRight, Right);
		MinUp = FMath::Min(MinUp, Up);
		MaxUp = FMath::Max(MaxUp, Up);
	}
	OutLocation = LocationSum / Locations.Num() + mWorldLocationOffset;

	const float RangeScale = FMath::Max(MaxRight - MinRight, MaxUp - MinUp);
	OutZoomScale = FMath::Clamp(mZoomDefaultScale + RangeScale * mZoomScaleRatio, mMinZoomScale, mMaxZoomScale);
}
