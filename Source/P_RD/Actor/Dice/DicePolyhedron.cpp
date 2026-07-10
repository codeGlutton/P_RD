#include "Actor/Dice/DicePolyhedron.h"

#include "Algo/Reverse.h"

namespace RDDicePolyhedron
{
	namespace
	{
		int32 AddFace(FDicePolyhedron& Poly, std::initializer_list<int32> Indices, bool bValueFace = true)
		{
			FDiceFace Face;
			Face.mVertexIndices = TArray<int32>(Indices);
			const int32 FaceIndex = Poly.mFaces.Add(MoveTemp(Face));
			if (bValueFace)
			{
				Poly.mValueFaceIndices.Add(FaceIndex);
			}
			return FaceIndex;
		}

		// 정점 정규화(최대 반지름 1) + 면별 중심/법선/up 계산 + 바깥 향하게 윈딩 보정.
		void Finalize(FDicePolyhedron& Poly)
		{
			float MaxRadius = 0.0f;
			for (const FVector& V : Poly.mVertices)
			{
				MaxRadius = FMath::Max(MaxRadius, static_cast<float>(V.Size()));
			}
			if (MaxRadius > KINDA_SMALL_NUMBER)
			{
				for (FVector& V : Poly.mVertices)
				{
					V /= MaxRadius;
				}
			}

			for (FDiceFace& Face : Poly.mFaces)
			{
				FVector Center = FVector::ZeroVector;
				for (int32 Index : Face.mVertexIndices)
				{
					Center += Poly.mVertices[Index];
				}
				Center /= static_cast<float>(Face.mVertexIndices.Num());
				Face.mCenter = Center;

				const FVector& A = Poly.mVertices[Face.mVertexIndices[0]];
				const FVector& B = Poly.mVertices[Face.mVertexIndices[1]];
				const FVector& C = Poly.mVertices[Face.mVertexIndices[2]];
				FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();

				// 다면체가 원점 중심이므로, 면 중심 방향과 법선이 반대면 안쪽을 향한 것 → 윈딩 뒤집기.
				if (FVector::DotProduct(Normal, Center) < 0.0f)
				{
					Algo::Reverse(Face.mVertexIndices);
					Normal = -Normal;
				}
				Face.mNormal = Normal;

				// 숫자 정렬용 up: 월드 Z를 면 평면에 투영(수평 면이면 월드 Y로 대체).
				FVector Up = FVector::UpVector - FVector::DotProduct(FVector::UpVector, Normal) * Normal;
				if (Up.Normalize() == false)
				{
					Up = FVector::YAxisVector - FVector::DotProduct(FVector::YAxisVector, Normal) * Normal;
					Up.Normalize();
				}
				Face.mUp = Up;
			}

			if (Poly.mValueFaceIndices.IsEmpty())
			{
				for (int32 FaceIndex = 0; FaceIndex < Poly.mFaces.Num(); ++FaceIndex)
				{
					Poly.mValueFaceIndices.Add(FaceIndex);
				}
			}
		}

		FDicePolyhedron BuildCoin()   // d2
		{
			FDicePolyhedron P;
			constexpr int32 SegmentCount = 32;
			constexpr float HalfThickness = 0.045f;
			constexpr float FaceRadius = 0.94f;
			constexpr float RimRadius = 1.0f;

			for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
			{
				const float Angle = FMath::DegreesToRadians(360.0f * static_cast<float>(Segment) / static_cast<float>(SegmentCount));
				P.mVertices.Add(FVector(FMath::Cos(Angle) * FaceRadius, FMath::Sin(Angle) * FaceRadius, HalfThickness));
			}
			for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
			{
				const float Angle = FMath::DegreesToRadians(360.0f * static_cast<float>(Segment) / static_cast<float>(SegmentCount));
				P.mVertices.Add(FVector(FMath::Cos(Angle) * FaceRadius, FMath::Sin(Angle) * FaceRadius, -HalfThickness));
			}
			for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
			{
				const float Angle = FMath::DegreesToRadians(360.0f * static_cast<float>(Segment) / static_cast<float>(SegmentCount));
				P.mVertices.Add(FVector(FMath::Cos(Angle) * RimRadius, FMath::Sin(Angle) * RimRadius, 0.0f));
			}

			TArray<int32> Top;
			TArray<int32> Bottom;
			Top.Reserve(SegmentCount);
			Bottom.Reserve(SegmentCount);
			for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
			{
				Top.Add(Segment);
				Bottom.Insert(SegmentCount + Segment, 0);
			}

			FDiceFace TopFace;
			TopFace.mVertexIndices = MoveTemp(Top);
			P.mValueFaceIndices.Add(P.mFaces.Add(MoveTemp(TopFace)));

			FDiceFace BottomFace;
			BottomFace.mVertexIndices = MoveTemp(Bottom);
			P.mValueFaceIndices.Add(P.mFaces.Add(MoveTemp(BottomFace)));

			for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
			{
				const int32 Next = (Segment + 1) % SegmentCount;
				const int32 Rim = SegmentCount * 2 + Segment;
				const int32 RimNext = SegmentCount * 2 + Next;
				AddFace(P, { Segment, Next, RimNext, Rim }, false);
				AddFace(P, { Rim, RimNext, SegmentCount + Next, SegmentCount + Segment }, false);
			}

			Finalize(P);
			return P;
		}

		FDicePolyhedron BuildTetrahedron()   // d4
		{
			FDicePolyhedron P;
			// FBX import axis conversion flips the tetrahedron to its complementary orientation.
			// The other regular dice are center-symmetric, but d4 is not, so mirror only this layout
			// to keep TextRender placement and settle rotations on the actual visible faces.
			P.mVertices = { FVector(-1,-1,-1), FVector(-1,1,1), FVector(1,-1,1), FVector(1,1,-1) };
			AddFace(P, {0,1,2}); AddFace(P, {0,3,1}); AddFace(P, {0,2,3}); AddFace(P, {1,3,2});
			Finalize(P);
			return P;
		}

		FDicePolyhedron BuildCube()          // d6
		{
			FDicePolyhedron P;
			P.mVertices = {
				FVector(-1,-1,-1), FVector(1,-1,-1), FVector(1,1,-1), FVector(-1,1,-1),
				FVector(-1,-1,1),  FVector(1,-1,1),  FVector(1,1,1),  FVector(-1,1,1)
			};
			AddFace(P, {0,1,2,3}); AddFace(P, {4,5,6,7});
			AddFace(P, {0,1,5,4}); AddFace(P, {2,3,7,6});
			AddFace(P, {1,2,6,5}); AddFace(P, {0,3,7,4});
			Finalize(P);
			return P;
		}

		FDicePolyhedron BuildOctahedron()    // d8
		{
			FDicePolyhedron P;
			P.mVertices = {
				FVector(1,0,0), FVector(-1,0,0), FVector(0,1,0),
				FVector(0,-1,0), FVector(0,0,1), FVector(0,0,-1)
			};
			AddFace(P, {0,2,4}); AddFace(P, {2,1,4}); AddFace(P, {1,3,4}); AddFace(P, {3,0,4});
			AddFace(P, {2,0,5}); AddFace(P, {1,2,5}); AddFace(P, {3,1,5}); AddFace(P, {0,3,5});
			Finalize(P);
			return P;
		}

		FDicePolyhedron BuildPentagonalTrapezohedron()   // d10
		{
			FDicePolyhedron P;
			const float Apex = 1.4f;
			const float MidZ = 0.28f;
			const float MidR = 1.0f;
			// 적도 정점 10개(짝수=위, 홀수=아래로 지그재그)
			for (int32 K = 0; K < 10; ++K)
			{
				const float Angle = FMath::DegreesToRadians(36.0f * static_cast<float>(K));
				const float Z = (K % 2 == 0) ? MidZ : -MidZ;
				P.mVertices.Add(FVector(MidR * FMath::Cos(Angle), MidR * FMath::Sin(Angle), Z));
			}
			const int32 Top = P.mVertices.Add(FVector(0, 0, Apex));     // 10
			const int32 Bottom = P.mVertices.Add(FVector(0, 0, -Apex)); // 11

			// 윗꼭지 카이트: 윗꼭지 + high(짝수) + low(홀수,먼점) + high(짝수). 아랫꼭지는 그 반대.
			// (Blender 생성 메시와 동일한 그룹핑이라야 숫자 포즈가 실제 면에 얹힌다.)
			for (int32 I = 0; I < 5; ++I)
			{
				AddFace(P, { Top, 2 * I, (2 * I + 1) % 10, (2 * I + 2) % 10 });
			}
			for (int32 I = 0; I < 5; ++I)
			{
				AddFace(P, { Bottom, (2 * I + 1) % 10, (2 * I + 2) % 10, (2 * I + 3) % 10 });
			}
			Finalize(P);
			return P;
		}

		FDicePolyhedron BuildDodecahedron()  // d12
		{
			FDicePolyhedron P;
			const float B = 0.6180339887f;   // 1/φ
			const float C = 1.6180339887f;   // φ
			P.mVertices = {
				FVector(1,1,1), FVector(1,1,-1), FVector(1,-1,1), FVector(1,-1,-1),
				FVector(-1,1,1), FVector(-1,1,-1), FVector(-1,-1,1), FVector(-1,-1,-1),
				FVector(0,B,C), FVector(0,B,-C), FVector(0,-B,C), FVector(0,-B,-C),
				FVector(B,C,0), FVector(B,-C,0), FVector(-B,C,0), FVector(-B,-C,0),
				FVector(C,0,B), FVector(C,0,-B), FVector(-C,0,B), FVector(-C,0,-B)
			};
			AddFace(P, {0,8,10,2,16}); AddFace(P, {0,16,17,1,12}); AddFace(P, {0,12,14,4,8});
			AddFace(P, {8,4,18,6,10}); AddFace(P, {10,6,15,13,2}); AddFace(P, {2,13,3,17,16});
			AddFace(P, {1,17,3,11,9});  AddFace(P, {1,9,5,14,12}); AddFace(P, {14,5,19,18,4});
			AddFace(P, {18,19,7,15,6}); AddFace(P, {15,7,11,3,13}); AddFace(P, {9,11,7,19,5});
			Finalize(P);
			return P;
		}

		FDicePolyhedron BuildIcosahedron()   // d20
		{
			FDicePolyhedron P;
			const float Phi = 1.6180339887f;
			P.mVertices = {
				FVector(-1,Phi,0), FVector(1,Phi,0), FVector(-1,-Phi,0), FVector(1,-Phi,0),
				FVector(0,-1,Phi), FVector(0,1,Phi), FVector(0,-1,-Phi), FVector(0,1,-Phi),
				FVector(Phi,0,-1), FVector(Phi,0,1), FVector(-Phi,0,-1), FVector(-Phi,0,1)
			};
			const int32 Tris[20][3] = {
				{0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
				{1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
				{3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
				{4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}
			};
			for (int32 I = 0; I < 20; ++I)
			{
				AddFace(P, { Tris[I][0], Tris[I][1], Tris[I][2] });
			}
			Finalize(P);
			return P;
		}

		FDicePolyhedron Build(int32 FaceCount)
		{
			switch (FaceCount)
			{
			case 2:  return BuildCoin();
			case 4:  return BuildTetrahedron();
			case 8:  return BuildOctahedron();
			case 10: return BuildPentagonalTrapezohedron();
			case 12: return BuildDodecahedron();
			case 20: return BuildIcosahedron();
			case 6:
			default: return BuildCube();
			}
		}
	}

	const FDicePolyhedron& Get(int32 FaceCount)
	{
		static TMap<int32, FDicePolyhedron> Cache;
		const int32 Key = (FaceCount == 2 || FaceCount == 4 || FaceCount == 8 || FaceCount == 10 || FaceCount == 12 || FaceCount == 20) ? FaceCount : 6;
		if (FDicePolyhedron* Found = Cache.Find(Key))
		{
			return *Found;
		}
		return Cache.Add(Key, Build(Key));
	}
}
