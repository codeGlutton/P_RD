#include "Actor/Dice/CombatDiceCaptureActor.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

/** @brief SceneCapture2D를 주사위 전용 오프스크린 캡처 설정으로 만든다. */
ACombatDiceCaptureActor::ACombatDiceCaptureActor()
{
	mSceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("DiceSceneCapture"));
	mSceneCaptureComponent->SetupAttachment(GetRootComponent());
	mSceneCaptureComponent->SetRelativeLocation(FVector(-320.0f, 0.0f, 26.0f));
	mSceneCaptureComponent->SetRelativeRotation(FRotator::ZeroRotator);
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

/** @brief 투명 RenderTarget과 UI 합성 머티리얼을 만들고 현재 주사위 컴포넌트를 캡처 대상으로 등록한다. */
void ACombatDiceCaptureActor::InitializeCapture(UObject* RenderTargetOuter, int32 RenderTargetSize)
{
	if (mSceneCaptureComponent == nullptr)
	{
		return;
	}

	// 128 미만은 숫자 가독성이 무너지고, 1024 초과는 HUD 다중 주사위에서 RT 비용이 급격히 커진다.
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

/** @brief 자동 캡처를 끈 대신 주사위 상태가 바뀐 직후 수동으로 한 프레임 캡처한다. */
void ACombatDiceCaptureActor::CaptureDice() const
{
	if (mSceneCaptureComponent != nullptr && mRenderTarget != nullptr)
	{
		mSceneCaptureComponent->CaptureScene();
	}
}

/** @brief UMG brush가 직접 쓰거나 디버그할 수 있는 RenderTarget 포인터를 반환한다. */
UTextureRenderTarget2D* ACombatDiceCaptureActor::GetRenderTarget() const
{
	return mRenderTarget;
}

/** @brief 알파 합성 머티리얼 인스턴스를 반환한다. UImage에는 보통 이 머티리얼을 물린다. */
UMaterialInstanceDynamic* ACombatDiceCaptureActor::GetCaptureMaterial() const
{
	return mCaptureMaterial;
}

/** @brief 주사위 타입 변경으로 컴포넌트가 재생성되면 SceneCapture 목록을 다시 구성하고 재촬영한다. */
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

/** @brief 주사위 Primitive를 게임 카메라에서는 숨기고 SceneCapture의 ShowOnlyList에만 노출한다. */
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
