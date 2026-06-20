#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Font.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

void ACombatDicePreviewActor::InitializeSceneComponents()
{
	mSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(mSceneRoot);

	mDiceRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DiceRoot"));
	mDiceRoot->SetupAttachment(mSceneRoot);

	mBackdropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackdropMesh"));
	mBackdropMesh->SetupAttachment(mSceneRoot);
	mBackdropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mBackdropMesh->SetCastShadow(false);
	mBackdropMesh->SetRelativeLocation(FVector(118.0f, 0.0f, 0.0f));
	mBackdropMesh->SetRelativeScale3D(FVector(0.018f, 10.0f, 8.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		mBackdropMesh->SetStaticMesh(CubeMeshFinder.Object);
	}

	// 주사위 몸체: 면 수에 맞는 블랭크 스태틱메시(SetDiceType이 교체).
	mDiceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DiceMesh"));
	mDiceMesh->SetupAttachment(mDiceRoot);
	mDiceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mDiceMesh->SetCastShadow(false);

	mCoinRimMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinRimMesh"));
	mCoinRimMesh->SetupAttachment(mDiceRoot);
	mCoinRimMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mCoinRimMesh->SetCastShadow(false);
	mCoinRimMesh->SetVisibility(false);
}

void ACombatDicePreviewActor::LoadDiceMeshes()
{
	struct FDiceMeshEntry { int32 FaceCount; const TCHAR* Path; };
	static const FDiceMeshEntry Entries[] = {
		{ 2,  TEXT("/Engine/BasicShapes/Cylinder.Cylinder") },
		{ 4,  TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/Meshes/SM_d4.SM_d4") },
		{ 6,  TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/Meshes/SM_d6.SM_d6") },
		{ 8,  TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/Meshes/SM_d8.SM_d8") },
		{ 10, TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/Meshes/SM_d10.SM_d10") },
		{ 12, TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/Meshes/SM_d12.SM_d12") },
		{ 20, TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/Meshes/SM_d20.SM_d20") },
	};
	static ConstructorHelpers::FObjectFinder<UStaticMesh> D2(Entries[0].Path);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> D4(Entries[1].Path);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> D6(Entries[2].Path);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> D8(Entries[3].Path);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> D10(Entries[4].Path);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> D12(Entries[5].Path);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> D20(Entries[6].Path);
	ConstructorHelpers::FObjectFinder<UStaticMesh>* Finders[] = { &D2, &D4, &D6, &D8, &D10, &D12, &D20 };

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Entries); ++Index)
	{
		if (Finders[Index]->Succeeded())
		{
			mDiceMeshAssets.Add(Entries[Index].FaceCount, Finders[Index]->Object);
		}
	}
}

void ACombatDicePreviewActor::InitializeLighting()
{
	mFrontFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FrontFillLight"));
	mFrontFillLight->SetupAttachment(mSceneRoot);
	mFrontFillLight->SetRelativeLocation(FVector(-180.0f, 0.0f, 90.0f));
	mFrontFillLight->SetIntensity(5200.0f);
	mFrontFillLight->SetAttenuationRadius(520.0f);
	mFrontFillLight->SetCastShadows(false);
	mFrontFillLight->SetLightColor(FLinearColor(0.96f, 1.0f, 0.98f));

	mSideFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SideFillLight"));
	mSideFillLight->SetupAttachment(mSceneRoot);
	mSideFillLight->SetRelativeLocation(FVector(-40.0f, 180.0f, 70.0f));
	mSideFillLight->SetIntensity(3400.0f);
	mSideFillLight->SetAttenuationRadius(460.0f);
	mSideFillLight->SetCastShadows(false);
	mSideFillLight->SetLightColor(FLinearColor(0.86f, 0.96f, 1.0f));

	mTopFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TopFillLight"));
	mTopFillLight->SetupAttachment(mSceneRoot);
	mTopFillLight->SetRelativeLocation(FVector(-30.0f, 0.0f, 220.0f));
	mTopFillLight->SetIntensity(2600.0f);
	mTopFillLight->SetAttenuationRadius(520.0f);
	mTopFillLight->SetCastShadows(false);
	mTopFillLight->SetLightColor(FLinearColor(1.0f, 0.98f, 0.90f));
}

void ACombatDicePreviewActor::LoadDiceNumberFont()
{
	static ConstructorHelpers::FObjectFinder<UFont> DiceNumberFontFinder(TEXT("/Engine/EngineFonts/RobotoDistanceField.RobotoDistanceField"));
	if (DiceNumberFontFinder.Succeeded())
	{
		mDiceNumberFont = DiceNumberFontFinder.Object;
	}
}
