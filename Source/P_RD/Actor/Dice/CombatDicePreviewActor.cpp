#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Actor/Dice/CombatDicePreviewActorPrivate.h"
#include "Actor/Dice/DicePolyhedron.h"

/** @brief CDO-safe 구성만 수행하고 런타임 컴포넌트 재생성은 BeginPlay/SetDiceType으로 미룬다. */
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

/** @brief 스폰 후 아직 타입 지정이 없으면 기본 d6 프리뷰를 구성한다. */
void ACombatDicePreviewActor::BeginPlay()
{
	Super::BeginPlay();

	// 스폰 후 HUD가 SetDiceType을 부르지 않는 경로(패널 등)를 위해 기본 주사위를 만들어 둔다.
	if (mFaceTexts.Num() == 0)
	{
		SetDiceType(mCurrentFaceCount);
	}
}

/** @brief 면 수에 맞는 메시/텍스트/커버를 재빌드하고 기본 1..N 면값을 적용한다. */
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

/** @brief 지원 면 수별 블랭크 메시를 적용하고 DiceRadius 기준으로 월드 크기를 통일한다. */
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
		// d2는 동전형 메시라 본체를 얇게 눌러야 카메라 정면에서 주사위 패널 안 밀도가 맞는다.
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

/** @brief 현재 면 수의 Polyhedron 데이터에서 지정 눈 값이 카메라 정면에 오도록 하는 회전을 계산한다. */
FRotator ACombatDicePreviewActor::GetSettledFaceRotation(int32 FaceValue) const
{
	const RDDicePolyhedron::FDicePolyhedron& Poly = RDDicePolyhedron::Get(mCurrentFaceCount);
	// 외부 눈 값은 1-base, Poly의 ValueFaceIndex는 0-base다.
	const int32 FaceIndex = FMath::Clamp(FaceValue - 1, 0, Poly.GetValueFaceCount() - 1);
	const RDDicePolyhedron::FDiceFace& Face = Poly.GetValueFace(FaceIndex);
	return RDCombatDicePreview::MakeFaceToCameraRotation(Face.mNormal, Face.mUp);
}
