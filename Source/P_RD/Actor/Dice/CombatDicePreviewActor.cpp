#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Actor/Dice/CombatDicePreviewActorPrivate.h"
#include "Actor/Dice/DicePolyhedron.h"

ACombatDicePreviewActor::ACombatDicePreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	InitializeSceneComponents();
	InitializeLighting();
	LoadDiceNumberFont();
	LoadDiceMeshes();
	InitializeMaterials();
	LoadDefaultFaceTextures();
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
	mCurrentFaceCount = FaceCount;
	ApplyDiceMesh(FaceCount);
	RebuildFaceTexts(FaceCount);

	// 기본 숫자 1..N(런타임에 SetFaceValues로 갱신).
	TArray<int32> DefaultValues;
	DefaultValues.Reserve(mFaceTexts.Num());
	for (int32 Index = 0; Index < mFaceTexts.Num(); ++Index)
	{
		DefaultValues.Add(Index + 1);
	}
	SetFaceValues(DefaultValues);
	SetFaceTextures(mFaceTextureOverrides);

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
		const FVector MeshScale = FaceCount == 2
			? FVector(Scale * 1.06f, Scale * 1.06f, Scale * 0.24f)
			: FVector(Scale);
		mDiceMesh->SetRelativeScale3D(MeshScale);

		if (mCoinRimMesh != nullptr)
		{
			const bool bShowCoinRim = FaceCount == 2;
			mCoinRimMesh->SetStaticMesh(bShowCoinRim ? Mesh : nullptr);
			mCoinRimMesh->SetVisibility(bShowCoinRim);
			if (bShowCoinRim)
			{
				mCoinRimMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -Scale * 0.9f));
				mCoinRimMesh->SetRelativeScale3D(FVector(Scale * 1.16f, Scale * 1.16f, Scale * 0.22f));
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
		mDiceMesh->SetMaterial(0, mDiceMaterial);
	}
	if (mCoinRimMesh != nullptr && mCoinRimMaterial != nullptr)
	{
		mCoinRimMesh->SetMaterial(0, mCoinRimMaterial);
	}
}

FRotator ACombatDicePreviewActor::GetSettledFaceRotation(int32 FaceValue) const
{
	const RDDicePolyhedron::FDicePolyhedron& Poly = RDDicePolyhedron::Get(mCurrentFaceCount);
	const int32 FaceIndex = FMath::Clamp(FaceValue - 1, 0, Poly.GetValueFaceCount() - 1);
	const RDDicePolyhedron::FDiceFace& Face = Poly.GetValueFace(FaceIndex);
	return RDCombatDicePreview::MakeFaceToCameraRotation(Face.mNormal, Face.mUp);
}
