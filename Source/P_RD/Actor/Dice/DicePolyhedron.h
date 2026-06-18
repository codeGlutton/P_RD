// @file DicePolyhedron.h
// @brief 주사위 다면체(d2/d4/d6/d8/d10/d12/d20)를 코드로 절차 생성하는 지오메트리 라이브러리
// 면에 들어갈 숫자가 런타임에 바뀔 수 있어 구운 텍스처 대신 정점/면/법선/up 데이터만 절차 생성한다.
// 타입별 최초 1회 생성 후 캐시하며, 숫자 TextRender 배치와 굴림 안착 회전이 이 데이터에 의존한다.

#pragma once

#include "RDMinimal.h"

namespace RDDicePolyhedron
{
	/** @brief 다면체 한 면. 메시 삼각화 + 면 숫자 배치/안착에 필요한 정보. */
	struct FDiceFace
	{
		// 바깥쪽 CCW winding이어야 ProceduralMesh 법선/UV 커버와 텍스트 회전이 같은 기준을 쓴다.
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
		// d2 코인의 옆면처럼 숫자를 붙이지 않는 면을 제외하기 위한 indirection이다.
		TArray<int32> mValueFaceIndices;

		// ValueFaceIndex는 표시 눈 값의 0-base index다. 외부 FaceValue(1-base)는 호출부에서 변환한다.
		int32 GetValueFaceCount() const { return mValueFaceIndices.Num(); }
		const FDiceFace& GetValueFace(int32 ValueFaceIndex) const { return mFaces[mValueFaceIndices[ValueFaceIndex]]; }
	};

	/** @brief 면 수에 해당하는 다면체를 돌려준다(최초 1회 생성 후 캐시). */
	// @param FaceCount 2/4/6/8/10/12/20. 그 외는 d6로 대체.
	// @return 캐시된 다면체 참조(정점은 대략 반지름 1로 정규화돼 있음).
	const FDicePolyhedron& Get(int32 FaceCount);
}
