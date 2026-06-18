#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "Actor/Dice/CombatDicePreviewActorPrivate.h"
#include "Actor/Dice/DicePolyhedron.h"

namespace
{
	// 면 커버를 본체보다 살짝 안쪽/바깥쪽에 배치해 z-fighting을 피하고 가장자리 림을 남긴다.
	constexpr float FaceCoverInset = 0.94f;
	constexpr float FaceCoverLift = 0.085f;
	constexpr float FaceUvPadding = 0.06f;

	/** @brief Face.mUp과 Face.mNormal에서 면 평면의 오른쪽 축을 만든다. */
	FVector MakeFaceRightVector(const RDDicePolyhedron::FDiceFace& Face)
	{
		FVector Right = FVector::CrossProduct(Face.mUp, Face.mNormal).GetSafeNormal();
		if (Right.IsNearlyZero())
		{
			Right = FVector::CrossProduct(FVector::YAxisVector, Face.mNormal).GetSafeNormal();
		}
		return Right;
	}

	/** @brief 한 면 위에 텍스처를 얹을 절차 메시 섹션을 삼각 팬과 정규화 UV로 만든다. */
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

		float MaxAbsRight = KINDA_SMALL_NUMBER;
		float MaxAbsUp = KINDA_SMALL_NUMBER;
		for (const int32 VertexIndex : Face.mVertexIndices)
		{
			const FVector CoveredDelta = (Poly.mVertices[VertexIndex] - Face.mCenter) * FaceCoverInset;
			const float RightCoord = static_cast<float>(FVector::DotProduct(CoveredDelta, Right));
			const float UpCoord = static_cast<float>(FVector::DotProduct(CoveredDelta, Face.mUp));
			PlaneCoords.Add(FVector2D(RightCoord, UpCoord));
			MaxAbsRight = FMath::Max(MaxAbsRight, FMath::Abs(RightCoord));
			MaxAbsUp = FMath::Max(MaxAbsUp, FMath::Abs(UpCoord));
		}

		const float UvHalfRange = 0.5f - FaceUvPadding;
		for (int32 LocalIndex = 0; LocalIndex < VertexCount; ++LocalIndex)
		{
			const int32 VertexIndex = Face.mVertexIndices[LocalIndex];
			const FVector CoveredVertex = Face.mCenter + (Poly.mVertices[VertexIndex] - Face.mCenter) * FaceCoverInset;
			OutVertices.Add(CoveredVertex * DiceRadius + Face.mNormal * (DiceRadius * FaceCoverLift));
			OutNormals.Add(Face.mNormal);
			OutUVs.Add(FVector2D(
				0.5f + (PlaneCoords[LocalIndex].X / MaxAbsRight) * UvHalfRange,
				0.5f - (PlaneCoords[LocalIndex].Y / MaxAbsUp) * UvHalfRange));
			OutVertexColors.Add(FLinearColor::White);
			OutTangents.Add(FProcMeshTangent(Right, false));
		}

		// Face.mVertexIndices는 바깥쪽 CCW라, 여기서는 뒤집힌 삼각 팬 순서로 전면 법선과 맞춘다.
		for (int32 TriangleIndex = 1; TriangleIndex < VertexCount - 1; ++TriangleIndex)
		{
			OutTriangles.Add(0);
			OutTriangles.Add(TriangleIndex + 1);
			OutTriangles.Add(TriangleIndex);
		}
	}
}

/** @brief 기존 면 숫자/커버 컴포넌트를 제거하고 현재 Polyhedron value face 수만큼 다시 만든다. */
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
	// [합의필요] 19~38 월드 크기는 현재 HUD 캡처용 가독성 튜닝값이다. 패널 크기 확정 후 재조정할 수 있다.
	const float UniformTextSize = FMath::Clamp(MinInradius * DiceRadius * 1.30f, 19.0f, 38.0f);

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
		FaceText->SetTextRenderColor(FColor(22, 24, 30));   // 밝은 주사위에 대비되는 진한 숫자
		FaceText->SetWorldSize(TextWorldSize);
		FaceText->SetCastShadow(false);
		if (mDiceNumberFont != nullptr)
		{
			FaceText->SetFont(mDiceNumberFont);
		}

		// 면 중심에서 바깥으로 띄워, 숫자가 주사위 몸체/커버 메시에 가리지 않게.
		FaceText->SetRelativeLocation(Face.mCenter * DiceRadius + Face.mNormal * (DiceRadius * 0.12f));
		FaceText->SetRelativeRotation(RDCombatDicePreview::MakeFaceTextRotation(Face.mNormal, Face.mUp));
		FaceText->SetText(FText::AsNumber(FaceIndex + 1));

		mFaceTexts.Add(FaceText);
	}
}

/** @brief 1-base FaceIndex를 mFaceTexts 0-base 배열로 변환해 숫자를 교체한다. */
void ACombatDicePreviewActor::SetFaceText(int32 FaceIndex, const FText& FaceText)
{
	const int32 ArrayIndex = FaceIndex - 1;
	if (mFaceTexts.IsValidIndex(ArrayIndex) && mFaceTexts[ArrayIndex] != nullptr)
	{
		mFaceTexts[ArrayIndex]->SetText(FaceText);
	}
}
