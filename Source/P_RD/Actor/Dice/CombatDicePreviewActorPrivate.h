#pragma once

#include "RDMinimal.h"

namespace RDCombatDicePreview
{
	struct FDiceFacePose
	{
		FVector mNormal;
		FVector mUp;
	};

	inline FDiceFacePose GetDiceFacePose(int32 FaceValue)
	{
		switch (FMath::Clamp(FaceValue, 1, 6))
		{
		case 1:
			return { FVector(0.0f, 0.0f, -1.0f), FVector(0.0f, -1.0f, 0.0f) };
		case 2:
			return { FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f) };
		case 3:
			return { FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f) };
		case 4:
			return { FVector(0.0f, -1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f) };
		case 5:
			return { FVector(-1.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f) };
		case 6:
			return { FVector(0.0f, 0.0f, 1.0f), FVector(0.0f, 1.0f, 0.0f) };
		default:
			return { FVector(-1.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f) };
		}
	}

	inline FRotator MakeFaceTextRotation(int32 FaceValue)
	{
		const FDiceFacePose FacePose = GetDiceFacePose(FaceValue);
		return FRotationMatrix::MakeFromXZ(FacePose.mNormal, FacePose.mUp).Rotator();
	}

	inline FRotator MakeFaceToCameraRotation(int32 FaceValue)
	{
		const FDiceFacePose FacePose = GetDiceFacePose(FaceValue);
		const FVector TargetNormal = FVector(-1.0f, 0.18f, 0.12f).GetSafeNormal();

		FVector TargetUp = FVector::UpVector - (FVector::UpVector | TargetNormal) * TargetNormal;
		if (TargetUp.Normalize() == false)
		{
			TargetUp = FVector::YAxisVector;
		}

		const FQuat SourceQuat = FRotationMatrix::MakeFromXZ(FacePose.mNormal, FacePose.mUp).ToQuat();
		const FQuat TargetQuat = FRotationMatrix::MakeFromXZ(TargetNormal, TargetUp).ToQuat();
		return (TargetQuat * SourceQuat.Inverse()).Rotator();
	}
}
