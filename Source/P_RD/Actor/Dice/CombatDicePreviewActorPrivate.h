#pragma once

#include "RDMinimal.h"

namespace RDCombatDicePreview
{
	/** @brief 면 법선/up이 캡처 카메라 정면과 화면 위쪽에 맞도록 주사위를 돌리는 회전. */
	inline FRotator MakeFaceToCameraRotation(const FVector& FaceNormal, const FVector& FaceUp)
	{
		// 살짝 기울인 -X 방향은 정면 숫자만 보이는 납작한 프리뷰를 피하기 위한 시각 튜닝값이다.
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

	/** @brief 면 위 숫자가 바깥을 향하고 글자 위쪽이 FaceUp과 맞도록 하는 TextRender 회전. */
	inline FRotator MakeFaceTextRotation(const FVector& FaceNormal, const FVector& FaceUp)
	{
		return FRotationMatrix::MakeFromXZ(FaceNormal, FaceUp).Rotator();
	}
}
