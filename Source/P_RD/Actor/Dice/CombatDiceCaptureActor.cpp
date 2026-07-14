#include "Actor/Dice/CombatDiceCaptureActor.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACombatDiceCaptureActor::ACombatDiceCaptureActor()
{
	mSceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("DiceSceneCapture"));
	mSceneCaptureComponent->SetupAttachment(GetRootComponent());
	mSceneCaptureComponent->SetRelativeLocation(FVector(-360.0f, 0.0f, 0.0f));
	mSceneCaptureComponent->SetRelativeRotation(FRotator::ZeroRotator);
	mSceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	mSceneCaptureComponent->OrthoWidth = 285.0f;
	mSceneCaptureComponent->FOVAngle = 50.0f;
	mSceneCaptureComponent->CaptureSource = SCS_SceneColorHDR;
	mSceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	mSceneCaptureComponent->bCaptureEveryFrame = false;
	mSceneCaptureComponent->bCaptureOnMovement = false;
	mSceneCaptureComponent->ShowFlags.SetAtmosphere(false);
	mSceneCaptureComponent->ShowFlags.SetFog(false);
	mSceneCaptureComponent->ShowFlags.SetBSP(false);
	mSceneCaptureComponent->ShowFlags.SetLandscape(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CaptureMaterialFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/M_DiceCaptureUI.M_DiceCaptureUI"));
	if (CaptureMaterialFinder.Succeeded())
	{
		mCaptureMaterialTemplate = CaptureMaterialFinder.Object;
	}
}

void ACombatDiceCaptureActor::InitializeCapture(UObject* RenderTargetOuter, int32 RenderTargetSize)
{
	if (mSceneCaptureComponent == nullptr)
	{
		return;
	}

	const int32 ClampedRenderTargetSize = FMath::Clamp(RenderTargetSize, 128, 1024);
	UObject* TargetOuter = RenderTargetOuter != nullptr ? RenderTargetOuter : this;
	mRenderTarget = NewObject<UTextureRenderTarget2D>(TargetOuter);
	if (mRenderTarget != nullptr)
	{
		mRenderTarget->RenderTargetFormat = RTF_RGBA8;
		mRenderTarget->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		mRenderTarget->bAutoGenerateMips = false;
		mRenderTarget->InitAutoFormat(ClampedRenderTargetSize, ClampedRenderTargetSize);
		mRenderTarget->UpdateResourceImmediate(true);
	}

	mSceneCaptureComponent->TextureTarget = mRenderTarget;
	if (mCaptureMaterialTemplate != nullptr && mRenderTarget != nullptr)
	{
		mCaptureMaterial = UMaterialInstanceDynamic::Create(mCaptureMaterialTemplate, this);
		if (mCaptureMaterial != nullptr)
		{
			mCaptureMaterial->SetTextureParameterValue(TEXT("DiceCaptureTexture"), mRenderTarget);
		}
	}

	mSceneCaptureComponent->ClearShowOnlyComponents();
	mSceneCaptureComponent->ShowOnlyActors.Reset();
	mSceneCaptureComponent->ShowOnlyActors.Add(this);
	mSceneCaptureComponent->ShowOnlyActorComponents(this, true);
	SetDicePrimitivesSceneCaptureOnly();
	CaptureDice();
}

void ACombatDiceCaptureActor::CaptureDice() const
{
	if (mSceneCaptureComponent != nullptr && mRenderTarget != nullptr)
	{
		mSceneCaptureComponent->CaptureScene();
	}
}

void ACombatDiceCaptureActor::CaptureDiceInto(UTextureRenderTarget2D* RenderTarget) const
{
	if (mSceneCaptureComponent == nullptr || RenderTarget == nullptr)
	{
		return;
	}

	mSceneCaptureComponent->TextureTarget = RenderTarget;
	mSceneCaptureComponent->CaptureScene();
	mSceneCaptureComponent->TextureTarget = mRenderTarget;
}

UMaterialInstanceDynamic* ACombatDiceCaptureActor::CreateCaptureMaterialFor(UObject* Outer, UTextureRenderTarget2D* RenderTarget)
{
	// 템플릿은 생성자(ConstructorHelpers)에서 로드돼 CDO에 들어 있다.
	const ACombatDiceCaptureActor* DefaultActor = GetDefault<ACombatDiceCaptureActor>();
	if (DefaultActor == nullptr || DefaultActor->mCaptureMaterialTemplate == nullptr || RenderTarget == nullptr)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* CaptureMaterial = UMaterialInstanceDynamic::Create(DefaultActor->mCaptureMaterialTemplate, Outer);
	if (CaptureMaterial != nullptr)
	{
		CaptureMaterial->SetTextureParameterValue(TEXT("DiceCaptureTexture"), RenderTarget);
	}
	return CaptureMaterial;
}

UTextureRenderTarget2D* ACombatDiceCaptureActor::GetRenderTarget() const
{
	return mRenderTarget;
}

UMaterialInstanceDynamic* ACombatDiceCaptureActor::GetCaptureMaterial() const
{
	return mCaptureMaterial;
}

void ACombatDiceCaptureActor::OnDiceRebuilt()
{
	if (mSceneCaptureComponent != nullptr && mRenderTarget != nullptr)
	{
		// 새로 만든 다면체 메시/숫자 TextRender를 캡처 목록/캡처전용으로 다시 반영하고 즉시 재촬영.
		mSceneCaptureComponent->ShowOnlyActors.Reset();
		mSceneCaptureComponent->ShowOnlyActors.Add(this);
		mSceneCaptureComponent->ShowOnlyActorComponents(this, true);
		SetDicePrimitivesSceneCaptureOnly();
		CaptureDice();
	}
}

void ACombatDiceCaptureActor::SetDicePrimitivesSceneCaptureOnly()
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr)
		{
			PrimitiveComponent->SetVisibleInSceneCaptureOnly(true);
		}
	}
}
