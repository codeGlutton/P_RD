#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Actor/Dice/CombatDicePreviewActorPrivate.h"
#include "Actor/Dice/DicePolyhedron.h"

namespace
{
	void ApplyMaterialToEverySlot(UStaticMeshComponent* MeshComponent, UMaterialInterface* Material)
	{
		if (MeshComponent == nullptr || Material == nullptr)
		{
			return;
		}

		const int32 MaterialSlotCount = FMath::Max(1, MeshComponent->GetNumMaterials());
		for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
		{
			MeshComponent->SetMaterial(SlotIndex, Material);
		}
	}
}

ACombatDicePreviewActor::ACombatDicePreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	InitializeSceneComponents();
	InitializeLighting();
	LoadDiceNumberFont();
	LoadDiceMeshes();
	InitializeMaterials();
	// 메시/숫자 생성(RegisterComponent 등 런타임 작업)은 BeginPlay/SetDiceType에서 — 생성자(CDO)에서 하면 크래시.
}

void ACombatDicePreviewActor::BeginPlay()
{
	Super::BeginPlay();

	// 스폰 후 HUD가 SetDiceType을 부르지 않는 경로(패널 등)를 위해 기본 주사위를 만들어 둔다.
	if (mFaceTexts.Num() == 0)
	{
		SetDiceType(mCurrentFaceCount);
	}
}

void ACombatDicePreviewActor::SetDiceType(int32 FaceCount)
{
	if (mCurrentFaceCount == FaceCount && mFaceTexts.IsEmpty() == false)
	{
		return;
	}

	mCurrentFaceCount = FaceCount;
	ApplyDiceMesh(FaceCount);
	ApplyDiceBodyMaterialPreset();
	RebuildFaceTexts(FaceCount);

	// 기본 숫자 1..N(런타임에 SetFaceValues로 갱신).
	TArray<int32> DefaultValues;
	DefaultValues.Reserve(mFaceTexts.Num());
	for (int32 Index = 0; Index < mFaceTexts.Num(); ++Index)
	{
		DefaultValues.Add(Index + 1);
	}
	SetFaceValues(DefaultValues);

	OnDiceRebuilt();   // 캡처 액터가 새 컴포넌트를 캡처 목록에 다시 등록/촬영
}

void ACombatDicePreviewActor::ApplyDiceMesh(int32 FaceCount)
{
	if (mDiceMesh == nullptr)
	{
		return;
	}

	TObjectPtr<UStaticMesh>* Found = mDiceMeshAssets.Find(FaceCount);
	if (Found == nullptr)
	{
		Found = mDiceMeshAssets.Find(6);   // 폴백: 큐브
	}
	UStaticMesh* Mesh = (Found != nullptr) ? Found->Get() : nullptr;
	mDiceMesh->SetStaticMesh(Mesh);

	if (Mesh != nullptr)
	{
		// 바운딩 반지름을 DiceRadius로 맞춰 주사위 종류별 크기 통일(FBX 임포트 스케일과 무관하게).
		const float MeshRadius = static_cast<float>(Mesh->GetBounds().SphereRadius);
		const float Scale = (MeshRadius > KINDA_SMALL_NUMBER) ? (DiceRadius / MeshRadius) : 1.0f;

		// 외접구(코너까지) 기준 정규화는 뾰족한 주사위(d4/d6)의 몸통을 작게 만든다.
		// 종류별 보정 배율로 체감 크기를 맞춘다(눈으로 튜닝하는 값).
		float SizeCorrection = 1.0f;
		switch (FaceCount)
		{
		case 4:  SizeCorrection = 1.45f; break;
		case 6:  SizeCorrection = 1.15f; break;
		case 8:  SizeCorrection = 1.18f; break;
		case 10: SizeCorrection = 1.10f; break;
		case 12: SizeCorrection = 0.92f; break;
		case 20: SizeCorrection = 0.85f; break;
		default: break;
		}

		// 보정은 부모(mDiceRoot)에 걸어 몸통·면 텍스처판·숫자가 함께 스케일되게 한다.
		// (몸통 메시에만 걸면 면판/숫자가 원래 반경에 남아 커진 몸통 속에 파묻힌다.)
		if (mDiceRoot != nullptr)
		{
			mDiceRoot->SetRelativeScale3D(FaceCount == 2 ? FVector(0.96f) : FVector(SizeCorrection));
		}

		const FVector MeshScale = FaceCount == 2
			? FVector(Scale * 1.08f, Scale * 1.08f, Scale * 0.105f)
			: FVector(Scale);
		mDiceMesh->SetRelativeScale3D(MeshScale);

		if (mCoinRimMesh != nullptr)
		{
			const bool bShowCoinRim = false;
			mCoinRimMesh->SetStaticMesh(bShowCoinRim ? Mesh : nullptr);
			mCoinRimMesh->SetVisibility(bShowCoinRim);
			if (bShowCoinRim)
			{
				mCoinRimMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -Scale * 3.2f));
				mCoinRimMesh->SetRelativeScale3D(FVector(Scale * 1.12f, Scale * 1.12f, Scale * 0.08f));
			}
		}
	}
	else if (mCoinRimMesh != nullptr)
	{
		mCoinRimMesh->SetStaticMesh(nullptr);
		mCoinRimMesh->SetVisibility(false);
	}

	if (mDiceMaterial != nullptr)
	{
		ApplyMaterialToEverySlot(mDiceMesh, mDiceMaterial);
	}
	if (mCoinRimMesh != nullptr && mCoinRimMaterial != nullptr)
	{
		ApplyMaterialToEverySlot(mCoinRimMesh, mCoinRimMaterial);
	}
}

FRotator ACombatDicePreviewActor::GetSettledFaceRotation(int32 FaceValue) const
{
	const RDDicePolyhedron::FDicePolyhedron& Poly = RDDicePolyhedron::Get(mCurrentFaceCount);
	const int32 FaceIndex = FMath::Clamp(FaceValue - 1, 0, Poly.GetValueFaceCount() - 1);
	const RDDicePolyhedron::FDiceFace& Face = Poly.GetValueFace(FaceIndex);
	return RDCombatDicePreview::MakeFaceToCameraRotation(Face.mNormal, Face.mUp);
}
