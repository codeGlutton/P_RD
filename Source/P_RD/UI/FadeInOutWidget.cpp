#include "UI/FadeInOutWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/ViewportZOrderType.h"

/**
 * Fade widget owns the transition overlay layer.
 *
 * Callers should only request OpenUI/CloseUI. They should not need to know
 * where a full-screen fade effect belongs in the viewport stack.
 */
UFadeInOutWidget::UFadeInOutWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::FadeInOut);
}

/**
 * Builds the native minimum visual when no WBP root exists.
 *
 * The native path keeps transition timing testable before designer-authored WBP
 * content is ready. If a WBP provides its own root, that root remains in charge.
 */
bool UFadeInOutWidget::Initialize()
{
	const bool bIsInitialized = Super::Initialize();
	EnsureDefaultVisual();
	return bIsInitialized;
}

/**
 * Keeps the default visual available in editor preview as well as runtime.
 */
void UFadeInOutWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureDefaultVisual();
}

/**
 * Advances the native fade and resolves the OpenUI/CloseUI callback.
 *
 * GameMode and transition subsystems wait for the callback only. The concrete
 * presentation can be this native opacity tick or a future WBP animation.
 */
void UFadeInOutWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!mFadeAnimationPlaying)
	{
		return;
	}

	mFadeElapsedSeconds += InDeltaTime;
	const float Alpha = mFadeDurationSeconds <= 0.0f
		? 1.0f
		: FMath::Clamp(mFadeElapsedSeconds / mFadeDurationSeconds, 0.0f, 1.0f);
	ApplyFadeAlpha(FMath::Lerp(mFadeStartAlpha, mFadeEndAlpha, Alpha));

	if (Alpha < 1.0f)
	{
		return;
	}

	mFadeAnimationPlaying = false;
	if (mFinishOpenWhenFadeEnds)
	{
		FinishOpenUI();
	}
	else
	{
		FinishCloseUI();
	}
}

/**
 * Fades to black before the room transition continues.
 */
void UFadeInOutWidget::PlayOpenUIAnimation_Implementation()
{
	StartFade(0.0f, 1.0f, mFadeOutSeconds, true);
}

/**
 * Fades from black back to gameplay before the close callback is released.
 */
void UFadeInOutWidget::PlayCloseUIAnimation_Implementation()
{
	StartFade(GetRenderOpacity(), 0.0f, mFadeInSeconds, false);
}

/**
 * Creates a plain full-screen black overlay only when no WBP root is present.
 */
void UFadeInOutWidget::EnsureDefaultVisual()
{
	if (WidgetTree == nullptr)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (WidgetTree->RootWidget != nullptr)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* FadeOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FadeOverlay"));
	FadeOverlay->SetBrushColor(FLinearColor::Black);

	UCanvasPanelSlot* OverlaySlot = RootCanvas->AddChildToCanvas(FadeOverlay);
	OverlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	OverlaySlot->SetOffsets(FMargin(0.0f));
	OverlaySlot->SetAlignment(FVector2D(0.0f, 0.0f));
}

/**
 * Starts a fade request through one completion path.
 *
 * Zero-duration fades and tick-driven fades both end through FinishOpenUI or
 * FinishCloseUI so callers observe the same lifecycle contract.
 */
void UFadeInOutWidget::StartFade(float StartAlpha, float EndAlpha, float Duration, bool bFinishOpen)
{
	mFadeStartAlpha = StartAlpha;
	mFadeEndAlpha = EndAlpha;
	mFadeElapsedSeconds = 0.0f;
	mFadeDurationSeconds = FMath::Max(0.0f, Duration);
	mFinishOpenWhenFadeEnds = bFinishOpen;
	mFadeAnimationPlaying = true;

	ApplyFadeAlpha(StartAlpha);
	if (mFadeDurationSeconds <= 0.0f)
	{
		ApplyFadeAlpha(EndAlpha);
		mFadeAnimationPlaying = false;
		if (mFinishOpenWhenFadeEnds)
		{
			FinishOpenUI();
		}
		else
		{
			FinishCloseUI();
		}
	}
}

/**
 * Applies the current fade opacity to the whole widget.
 */
void UFadeInOutWidget::ApplyFadeAlpha(float Alpha)
{
	SetRenderOpacity(FMath::Clamp(Alpha, 0.0f, 1.0f));
}
