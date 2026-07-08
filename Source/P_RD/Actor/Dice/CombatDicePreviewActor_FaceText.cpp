#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "Actor/Dice/CombatDicePreviewActorPrivate.h"
#include "Actor/Dice/DicePolyhedron.h"

namespace
{
	constexpr float FaceCoverInset = 1.08f;
	constexpr float FaceCoverLift = 0.030f;
	constexpr float FaceTextLift = 0.060f;
	constexpr float FaceTextStrokeLift = 0.058f;
	constexpr float FaceTextStrokeOffset = 0.0065f;
	constexpr int32 FaceTextStrokeCount = 4;
	constexpr float FaceUvPadding = 0.0f;

	FVector MakeFaceRightVector(const RDDicePolyhedron::FDiceFace& Face)
	{
		FVector Right = FVector::CrossProduct(Face.mUp, Face.mNormal).GetSafeNormal();
		if (Right.IsNearlyZero())
		{
			Right = FVector::CrossProduct(FVector::YAxisVector, Face.mNormal).GetSafeNormal();
		}
		return Right;
	}

	float GetPolygonSignedArea(const TArray<FVector2D>& Points)
	{
		float Area = 0.0f;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const FVector2D& Current = Points[Index];
			const FVector2D& Next = Points[(Index + 1) % Points.Num()];
			Area += Current.X * Next.Y - Next.X * Current.Y;
		}
		return Area * 0.5f;
	}

	int32 FindFixedUvAnchorIndex(const TArray<FVector2D>& PlaneCoords)
	{
		int32 AnchorIndex = 0;
		float BestUp = -TNumericLimits<float>::Max();
		float BestLeft = TNumericLimits<float>::Max();
		float BestCenter = TNumericLimits<float>::Max();

		for (int32 Index = 0; Index < PlaneCoords.Num(); ++Index)
		{
			const FVector2D& Coord = PlaneCoords[Index];
			const bool bBetterUp = Coord.Y > BestUp + KINDA_SMALL_NUMBER;
			const bool bSameUp = FMath::IsNearlyEqual(Coord.Y, BestUp, 0.01f);
			const float AbsRight = FMath::Abs(Coord.X);
			const bool bBetterTie =
				PlaneCoords.Num() == 4
					? Coord.X < BestLeft
					: AbsRight < BestCenter;

			if (bBetterUp || (bSameUp && bBetterTie))
			{
				AnchorIndex = Index;
				BestUp = Coord.Y;
				BestLeft = Coord.X;
				BestCenter = AbsRight;
			}
		}

		return AnchorIndex;
	}

	FVector2D MakeFixedFaceUV(int32 VertexCount, int32 CanonicalIndex)
	{
		const float StartAngle = VertexCount == 4 ? 3.0f * PI * 0.25f : PI * 0.5f;
		const auto MakeUnitPoint = [StartAngle, VertexCount](int32 Index)
		{
			const float Angle = StartAngle + (2.0f * PI * static_cast<float>(Index) / static_cast<float>(VertexCount));
			return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		};

		FVector2D Min(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
		FVector2D Max(-TNumericLimits<float>::Max(), -TNumericLimits<float>::Max());
		for (int32 Index = 0; Index < VertexCount; ++Index)
		{
			const FVector2D Point = MakeUnitPoint(Index);
			Min.X = FMath::Min(Min.X, Point.X);
			Min.Y = FMath::Min(Min.Y, Point.Y);
			Max.X = FMath::Max(Max.X, Point.X);
			Max.Y = FMath::Max(Max.Y, Point.Y);
		}

		const FVector2D Point = MakeUnitPoint(CanonicalIndex);
		const float U = (Point.X - Min.X) / FMath::Max(Max.X - Min.X, KINDA_SMALL_NUMBER);
		const float V = 1.0f - ((Point.Y - Min.Y) / FMath::Max(Max.Y - Min.Y, KINDA_SMALL_NUMBER));
		const float PaddedRange = 1.0f - FaceUvPadding * 2.0f;
		return FVector2D(
			FaceUvPadding + U * PaddedRange,
			FaceUvPadding + V * PaddedRange);
	}

	void BuildFaceCoverSection(
		const RDDicePolyhedron::FDicePolyhedron& Poly,
		const RDDicePolyhedron::FDiceFace& Face,
		float DiceRadius,
		TArray<FVector>& OutVertices,
		TArray<int32>& OutTriangles,
		TArray<FVector>& OutNormals,
		TArray<FVector2D>& OutUVs,
		TArray<FLinearColor>& OutVertexColors,
		TArray<FProcMeshTangent>& OutTangents)
	{
		const int32 VertexCount = Face.mVertexIndices.Num();
		OutVertices.Reset(VertexCount);
		OutTriangles.Reset(FMath::Max(0, VertexCount - 2) * 3);
		OutNormals.Reset(VertexCount);
		OutUVs.Reset(VertexCount);
		OutVertexColors.Reset(VertexCount);
		OutTangents.Reset(VertexCount);

		if (VertexCount < 3)
		{
			return;
		}

		const FVector Right = MakeFaceRightVector(Face);
		TArray<FVector2D> PlaneCoords;
		PlaneCoords.Reserve(VertexCount);

		for (const int32 VertexIndex : Face.mVertexIndices)
		{
			const FVector Delta = Poly.mVertices[VertexIndex] - Face.mCenter;
			const float RightCoord = static_cast<float>(FVector::DotProduct(Delta, Right));
			const float UpCoord = static_cast<float>(FVector::DotProduct(Delta, Face.mUp));
			PlaneCoords.Add(FVector2D(RightCoord, UpCoord));
		}

		const int32 AnchorIndex = FindFixedUvAnchorIndex(PlaneCoords);
		const bool bLocalCounterClockwise = GetPolygonSignedArea(PlaneCoords) >= 0.0f;
		for (int32 LocalIndex = 0; LocalIndex < VertexCount; ++LocalIndex)
		{
			const int32 VertexIndex = Face.mVertexIndices[LocalIndex];
			const FVector CoveredVertex = Face.mCenter + (Poly.mVertices[VertexIndex] - Face.mCenter) * FaceCoverInset;
			const int32 CanonicalIndex = bLocalCounterClockwise
				? (LocalIndex - AnchorIndex + VertexCount) % VertexCount
				: (AnchorIndex - LocalIndex + VertexCount) % VertexCount;
			OutVertices.Add(CoveredVertex * DiceRadius + Face.mNormal * (DiceRadius * FaceCoverLift));
			OutNormals.Add(Face.mNormal);
			OutUVs.Add(MakeFixedFaceUV(VertexCount, CanonicalIndex));
			OutVertexColors.Add(FLinearColor::White);
			OutTangents.Add(FProcMeshTangent(Right, false));
		}

		for (int32 TriangleIndex = 1; TriangleIndex < VertexCount - 1; ++TriangleIndex)
		{
			OutTriangles.Add(0);
			OutTriangles.Add(TriangleIndex + 1);
			OutTriangles.Add(TriangleIndex);
		}
	}
}

void ACombatDicePreviewActor::RebuildFaceTexts(int32 FaceCount)
{
	// 기존 TextRender 제거.
	for (UTextRenderComponent* FaceText : mFaceTexts)
	{
		if (FaceText != nullptr)
		{
			FaceText->DestroyComponent();
		}
	}
	mFaceTexts.Reset();
	for (UTextRenderComponent* FaceTextStroke : mFaceTextStrokes)
	{
		if (FaceTextStroke != nullptr)
		{
			FaceTextStroke->DestroyComponent();
		}
	}
	mFaceTextStrokes.Reset();
	for (UProceduralMeshComponent* FaceMesh : mFaceMeshes)
	{
		if (FaceMesh != nullptr)
		{
			FaceMesh->DestroyComponent();
		}
	}
	mFaceMeshes.Reset();
	mFaceMaterials.Reset();

	const RDDicePolyhedron::FDicePolyhedron& Poly = RDDicePolyhedron::Get(FaceCount);

	// 주사위 종류당 숫자 크기를 하나로 통일(면마다 들쭉날쭉하지 않게). 가장 작은 면에도 들어가게 최소 내접반지름 기준.
	float MinInradius = TNumericLimits<float>::Max();
	for (int32 ValueFaceIndex = 0; ValueFaceIndex < Poly.GetValueFaceCount(); ++ValueFaceIndex)
	{
		const RDDicePolyhedron::FDiceFace& Face = Poly.GetValueFace(ValueFaceIndex);
		const int32 EdgeCount = Face.mVertexIndices.Num();
		for (int32 Edge = 0; Edge < EdgeCount; ++Edge)
		{
			const FVector& V0 = Poly.mVertices[Face.mVertexIndices[Edge]];
			const FVector& V1 = Poly.mVertices[Face.mVertexIndices[(Edge + 1) % EdgeCount]];
			MinInradius = FMath::Min(MinInradius, static_cast<float>(((V0 + V1) * 0.5f - Face.mCenter).Size()));
		}
	}
	// 주사위 간에도 너무 차이나지 않게 클램프.
	const float UniformTextSize = FMath::Clamp(MinInradius * DiceRadius * 1.48f, 24.0f, 48.0f);

	for (int32 FaceIndex = 0; FaceIndex < Poly.GetValueFaceCount(); ++FaceIndex)
	{
		const RDDicePolyhedron::FDiceFace& Face = Poly.GetValueFace(FaceIndex);
		const float TextWorldSize = UniformTextSize;
		if (mFaceMaterialTemplate != nullptr)
		{
			UProceduralMeshComponent* FaceMesh = NewObject<UProceduralMeshComponent>(this);
			if (FaceMesh != nullptr)
			{
				FaceMesh->SetupAttachment(mDiceRoot);
				FaceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				FaceMesh->SetCastShadow(false);
				FaceMesh->SetCanEverAffectNavigation(false);
				FaceMesh->RegisterComponent();

				TArray<FVector> Vertices;
				TArray<int32> Triangles;
				TArray<FVector> Normals;
				TArray<FVector2D> UVs;
				TArray<FLinearColor> VertexColors;
				TArray<FProcMeshTangent> Tangents;
				BuildFaceCoverSection(Poly, Face, DiceRadius, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
				FaceMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);

				UMaterialInstanceDynamic* FaceMaterial = UMaterialInstanceDynamic::Create(mFaceMaterialTemplate, this);
				if (FaceMaterial != nullptr)
				{
					FaceMesh->SetMaterial(0, FaceMaterial);
				}
				mFaceMaterials.Add(FaceMaterial);
				mFaceMeshes.Add(FaceMesh);
			}
			else
			{
				mFaceMaterials.Add(nullptr);
				mFaceMeshes.Add(nullptr);
			}
		}
		else
		{
			mFaceMaterials.Add(nullptr);
			mFaceMeshes.Add(nullptr);
		}

		const FVector BaseTextLocation = Face.mCenter * DiceRadius + Face.mNormal * (DiceRadius * FaceTextLift);
		const FVector StrokeBaseLocation = Face.mCenter * DiceRadius + Face.mNormal * (DiceRadius * FaceTextStrokeLift);
		const FVector Right = MakeFaceRightVector(Face);
		const float StrokeOffset = DiceRadius * FaceTextStrokeOffset;
		const FRotator TextRotation = RDCombatDicePreview::MakeFaceTextRotation(Face.mNormal, Face.mUp);
		const FText DefaultText = FText::AsNumber(FaceIndex + 1);
		const FVector StrokeDirections[FaceTextStrokeCount] =
		{
			Right,
			-Right,
			Face.mUp,
			-Face.mUp
		};

		for (int32 StrokeIndex = 0; StrokeIndex < FaceTextStrokeCount; ++StrokeIndex)
		{
			UTextRenderComponent* FaceTextStroke = NewObject<UTextRenderComponent>(this);
			if (FaceTextStroke == nullptr)
			{
				mFaceTextStrokes.Add(nullptr);
				continue;
			}

			FaceTextStroke->SetupAttachment(mDiceRoot);
			FaceTextStroke->RegisterComponent();
			FaceTextStroke->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
			FaceTextStroke->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
			FaceTextStroke->SetTextRenderColor(FColor(12, 12, 18));
			FaceTextStroke->SetWorldSize(TextWorldSize);
			FaceTextStroke->SetCastShadow(false);
			if (mDiceNumberFont != nullptr)
			{
				FaceTextStroke->SetFont(mDiceNumberFont);
			}
			FaceTextStroke->SetRelativeLocation(StrokeBaseLocation + StrokeDirections[StrokeIndex] * StrokeOffset);
			FaceTextStroke->SetRelativeRotation(TextRotation);
			FaceTextStroke->SetText(DefaultText);
			mFaceTextStrokes.Add(FaceTextStroke);
		}

		UTextRenderComponent* FaceText = NewObject<UTextRenderComponent>(this);
		if (FaceText == nullptr)
		{
			mFaceTexts.Add(nullptr);
			continue;
		}
		FaceText->SetupAttachment(mDiceRoot);
		FaceText->RegisterComponent();
		FaceText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		FaceText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
		FaceText->SetTextRenderColor(FColor(24, 24, 30));   // 밝은 주사위에 대비되는 진한 숫자
		FaceText->SetWorldSize(TextWorldSize);
		FaceText->SetCastShadow(false);
		if (mDiceNumberFont != nullptr)
		{
			FaceText->SetFont(mDiceNumberFont);
		}

		// 면 중심에서 바깥으로 띄워, 숫자가 주사위 몸체에 가리지 않게.
		FaceText->SetRelativeLocation(BaseTextLocation);
		FaceText->SetRelativeRotation(TextRotation);
		FaceText->SetText(DefaultText);

		mFaceTexts.Add(FaceText);
	}
}

void ACombatDicePreviewActor::SetFaceText(int32 FaceIndex, const FText& FaceText)
{
	const int32 ArrayIndex = FaceIndex - 1;
	if (mFaceTexts.IsValidIndex(ArrayIndex) && mFaceTexts[ArrayIndex] != nullptr)
	{
		mFaceTexts[ArrayIndex]->SetText(FaceText);
	}

	const int32 StrokeStartIndex = ArrayIndex * FaceTextStrokeCount;
	for (int32 StrokeOffsetIndex = 0; StrokeOffsetIndex < FaceTextStrokeCount; ++StrokeOffsetIndex)
	{
		const int32 StrokeIndex = StrokeStartIndex + StrokeOffsetIndex;
		if (mFaceTextStrokes.IsValidIndex(StrokeIndex) && mFaceTextStrokes[StrokeIndex] != nullptr)
		{
			mFaceTextStrokes[StrokeIndex]->SetText(FaceText);
		}
	}
}
