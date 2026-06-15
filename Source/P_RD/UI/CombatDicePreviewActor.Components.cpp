#include "UI/CombatDicePreviewActor.h"

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

	mDiceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DiceMesh"));
	mDiceMesh->SetupAttachment(mDiceRoot);
	mDiceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mDiceMesh->SetCastShadow(false);
	mDiceMesh->SetRelativeScale3D(FVector(1.05f));
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

void ACombatDicePreviewActor::ApplyCubeMesh(UStaticMesh* CubeMesh)
{
	if (CubeMesh == nullptr)
	{
		return;
	}

	mDiceMesh->SetStaticMesh(CubeMesh);
	mBackdropMesh->SetStaticMesh(CubeMesh);
}

void ACombatDicePreviewActor::LoadDiceNumberFont()
{
	static ConstructorHelpers::FObjectFinder<UFont> DiceNumberFontFinder(TEXT("/Engine/EngineFonts/RobotoDistanceField.RobotoDistanceField"));
	if (DiceNumberFontFinder.Succeeded())
	{
		mDiceNumberFont = DiceNumberFontFinder.Object;
	}
}
