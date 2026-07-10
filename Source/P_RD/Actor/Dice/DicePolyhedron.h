/*****************************************************************//**
 * @file   DicePolyhedron.h
 * @brief  주사위 다면체(d2/d4/d6/d8/d10/d12/d20)를 코드로 절차 생성하는 지오메트리 라이브러리
 *
 * @details
 * 면에 들어갈 숫자가 런타임에 바뀔 수 있어(구운 텍스처 부적합) 모양만 절차적으로 만든다.
 * 정점/면 데이터와 함께 면별 중심·법선·up(숫자 TextRender 배치 + 굴림 안착 회전용)을 제공한다.
 * 타입(면 수)별로 최초 1회 생성 후 캐시해 재사용한다(주사위마다 재생성하지 않음).
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

namespace RDDicePolyhedron
{
	/** @brief 다면체 한 면. 메시 삼각화 + 면 숫자 배치/안착에 필요한 정보. */
	struct FDiceFace
	{
		TArray<int32> mVertexIndices;            // 면을 이루는 정점 인덱스(바깥쪽 CCW로 정렬됨)
		FVector mCenter = FVector::ZeroVector;   // 면 중심(정점 평균)
		FVector mNormal = FVector::ZeroVector;   // 바깥 법선(단위)
		FVector mUp = FVector::ZeroVector;        // 면 평면 내 '위' 방향(숫자 정렬용, 단위)
	};

	/** @brief 다면체 전체(정점 + 면 목록). */
	struct FDicePolyhedron
	{
		TArray<FVector> mVertices;
		TArray<FDiceFace> mFaces;
		TArray<int32> mValueFaceIndices;

		int32 GetValueFaceCount() const { return mValueFaceIndices.Num(); }
		const FDiceFace& GetValueFace(int32 ValueFaceIndex) const { return mFaces[mValueFaceIndices[ValueFaceIndex]]; }
	};

	/**
	 * @brief 면 수에 해당하는 다면체를 돌려준다(최초 1회 생성 후 캐시).
	 *
	 * @param FaceCount 2/4/6/8/10/12/20. 그 외는 d6로 대체.
	 * @return 캐시된 다면체 참조(정점은 대략 반지름 1로 정규화돼 있음).
	 */
	const FDicePolyhedron& Get(int32 FaceCount);
}
