#pragma once

#include "RDMinimal.h"

namespace RDCombatDicePreview
{
	// 면(법선/up)이 카메라 정면을 향하도록(약간 기울인 -X 방향) 주사위를 돌리는 회전.
	inline FRotator MakeFaceToCameraRotation(const FVector& FaceNormal, const FVector& FaceUp)
	{
		const FVector TargetNormal = FVector(-1.0f, 0.10f, 0.08f).GetSafeNormal();

		FVector TargetUp = FVector::UpVector - (FVector::UpVector | TargetNormal) * TargetNormal;
		if (TargetUp.Normalize() == false)
		{
			TargetUp = FVector::YAxisVector;
		}

		const FQuat SourceQuat = FRotationMatrix::MakeFromXZ(FaceNormal, FaceUp).ToQuat();
		const FQuat TargetQuat = FRotationMatrix::MakeFromXZ(TargetNormal, TargetUp).ToQuat();
		return (TargetQuat * SourceQuat.Inverse()).Rotator();
	}

	// 면 위 숫자가 바깥(법선 방향)을 보도록 하는 TextRender 회전(법선=X, up=Z).
	inline FRotator MakeFaceTextRotation(const FVector& FaceNormal, const FVector& FaceUp)
	{
		return FRotationMatrix::MakeFromXZ(FaceNormal, FaceUp).Rotator();
	}
}
