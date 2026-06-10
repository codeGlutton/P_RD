#include "UI/LoadingNotifyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "TimerManager.h"
#include "UI/ViewportZOrderType.h"

/**
 * Loading notice owns its transition notification layer.
 *
 * Callers should open and close the widget through the common UI lifecycle
 * without passing presentation-specific z-order values.
 */
ULoadingNotifyWidget::ULoadingNotifyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::LoadingNotify);
}

/**
 * Builds the native minimum visual when no WBP root exists.
 *
 * This keeps preload and transition flow testable before final WBP art is
 * available. A WBP-provided root remains the owner of the final layout.
 */
bool ULoadingNotifyWidget::Initialize()
{
	const bool bIsInitialized = Super::Initialize();
	EnsureDefaultVisual();
	return bIsInitialized;
}

/**
 * Keeps the default visual available in editor preview as well as runtime.
 */
void ULoadingNotifyWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureDefaultVisual();
}

/**
 * Drives the lightweight loading indicator animation while loading is active.
 *
 * The widget owns the animation rhythm. GameMode and transition subsystems only
 * wait for OpenUI/CloseUI completion and do not inspect this presentation detail.
 */
void ULoadingNotifyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (mLoadingState != ELoadingNotifyState::Loading || mLoadingIndicator == nullptr)
	{
		return;
	}

	mIndicatorElapsedSeconds += InDeltaTime;
	const float BlinkRate = FMath::Max(0.01f, mIndicatorBlinkSpeed);
	const float WaveAlpha = 0.5f + 0.5f * FMath::Sin(mIndicatorElapsedSeconds * BlinkRate * 2.0f * UE_PI);
	ApplyIndicatorAlpha(0.35f + 0.65f * WaveAlpha);
}

/**
 * Opens the loading notice and clears stale close timers.
 *
 * A rapid transition can reopen the widget while a previous completed-state
 * timer is still pending, so every open starts from a clean timer state.
 */
void ULoadingNotifyWidget::PlayOpenUIAnimation_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mMinimumVisibleTimerHandle);
		World->GetTimerManager().ClearTimer(mCompletedVisibleTimerHandle);
		World->GetTimerManager().ClearTimer(mCloseAnimationTimerHandle);
		mOpenedTimeSeconds = World->GetTimeSeconds();
	}
	else
	{
		mOpenedTimeSeconds = 0.0;
	}

	SetLoadingState(ELoadingNotifyState::Loading);
	ApplyIndicatorAlpha(1.0f);
	FinishOpenUI();
}

/**
 * Requests the close sequence after the minimum visible time is satisfied.
 *
 * Even when preload completes quickly, the player should still see that a
 * transition occurred. The close callback is therefore delayed as needed.
 */
void ULoadingNotifyWidget::PlayCloseUIAnimation_Implementation()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		ShowCompletedState();
		return;
	}

	const double ElapsedSeconds = FMath::Max(0.0, World->GetTimeSeconds() - mOpenedTimeSeconds);
	const float RemainingSeconds = FMath::Max(0.0f, mMinimumVisibleSeconds - static_cast<float>(ElapsedSeconds));
	if (RemainingSeconds > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			mMinimumVisibleTimerHandle,
			this,
			&ULoadingNotifyWidget::ShowCompletedState,
			RemainingSeconds,
			false);
		return;
	}

	ShowCompletedState();
}

/**
 * Creates a plain lower-right loading indicator only when no WBP root exists.
 */
void ULoadingNotifyWidget::EnsureDefaultVisual()
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

	UTextBlock* IndicatorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadingIndicator"));
	IndicatorText->SetText(NSLOCTEXT("LoadingNotifyWidget", "DefaultIndicatorText", "●"));
	IndicatorText->SetJustification(ETextJustify::Center);
	IndicatorText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	IndicatorText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	FSlateFontInfo IndicatorFontInfo = IndicatorText->GetFont();
	IndicatorFontInfo.Size = 32;
	IndicatorText->SetFont(IndicatorFontInfo);

	UCanvasPanelSlot* IndicatorSlot = RootCanvas->AddChildToCanvas(IndicatorText);
	IndicatorSlot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
	IndicatorSlot->SetOffsets(FMargin(-284.0f, -96.0f, 48.0f, 48.0f));
	IndicatorSlot->SetAlignment(FVector2D(0.0f, 0.5f));
	mLoadingIndicator = IndicatorText;

	UTextBlock* LoadingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadingStatusText"));
	LoadingText->SetJustification(ETextJustify::Center);
	LoadingText->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	FSlateFontInfo FontInfo = LoadingText->GetFont();
	FontInfo.Size = 36;
	LoadingText->SetFont(FontInfo);

	UCanvasPanelSlot* TextSlot = RootCanvas->AddChildToCanvas(LoadingText);
	TextSlot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
	TextSlot->SetOffsets(FMargin(-236.0f, -92.0f, 196.0f, 56.0f));
	TextSlot->SetAlignment(FVector2D(0.0f, 0.5f));
	mLoadingStatusText = LoadingText;
}

/**
 * Updates the local visual phase without exposing text or indicator details.
 */
void ULoadingNotifyWidget::SetLoadingState(ELoadingNotifyState NewState)
{
	mLoadingState = NewState;
	if (mLoadingIndicator != nullptr)
	{
		mLoadingIndicator->SetVisibility(NewState == ELoadingNotifyState::Loading ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (mLoadingStatusText == nullptr)
	{
		return;
	}

	switch (NewState)
	{
	case ELoadingNotifyState::Loading:
		mLoadingStatusText->SetText(NSLOCTEXT("LoadingNotifyWidget", "LoadingText", "로딩중"));
		break;
	case ELoadingNotifyState::Completed:
		mLoadingStatusText->SetText(NSLOCTEXT("LoadingNotifyWidget", "LoadingCompletedText", "로딩완료"));
		break;
	default:
		mLoadingStatusText->SetText(FText::GetEmpty());
		break;
	}
}

/**
 * Applies opacity to the native loading indicator when present.
 */
void ULoadingNotifyWidget::ApplyIndicatorAlpha(float Alpha) const
{
	if (mLoadingIndicator != nullptr)
	{
		mLoadingIndicator->SetRenderOpacity(FMath::Clamp(Alpha, 0.0f, 1.0f));
	}
}

/**
 * Shows the completed state before the close lifecycle finishes.
 *
 * This lets transition code wait for CloseUI while the widget decides how long
 * to display the "completed" phase.
 */
void ULoadingNotifyWidget::ShowCompletedState()
{
	SetLoadingState(ELoadingNotifyState::Completed);

	UWorld* World = GetWorld();
	const float CloseDelaySeconds = FMath::Max(0.0f, mCompletedVisibleSeconds + mCloseAnimationSeconds);
	if (World != nullptr && CloseDelaySeconds > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			mCompletedVisibleTimerHandle,
			this,
			&ULoadingNotifyWidget::FinishCompletedState,
			CloseDelaySeconds,
			false);
		return;
	}

	FinishCompletedState();
}

/**
 * Completes the close lifecycle after the loading notice has finished its local
 * presentation sequence.
 */
void ULoadingNotifyWidget::FinishCompletedState()
{
	SetLoadingState(ELoadingNotifyState::None);
	FinishCloseUI();
}
