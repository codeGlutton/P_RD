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
 * 로딩 알림 위젯은 전환 알림 레이어를 소유한다.
 *
 * 호출자는 표현 전용 ZOrder 값을 넘기지 않고 공통 UI 생명주기를 통해 이 위젯을 열고 닫는다.
 */
ULoadingNotifyWidget::ULoadingNotifyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::LoadingNotify);
}

/**
 * WBP 루트가 없을 때 네이티브 최소 화면을 만든다.
 *
 * 최종 WBP 아트가 준비되기 전에도 프리로드와 전환 흐름을 검증할 수 있게 한다.
 * WBP가 자체 루트를 제공하면 최종 레이아웃의 책임은 WBP가 가진다.
 */
bool ULoadingNotifyWidget::Initialize()
{
	const bool bIsInitialized = Super::Initialize();
	EnsureDefaultVisual();
	return bIsInitialized;
}

/**
 * 에디터 미리보기와 런타임 모두에서 기본 화면을 사용할 수 있게 한다.
 */
void ULoadingNotifyWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureDefaultVisual();
}

/**
 * 로딩 중 가벼운 로딩 인디케이터 애니메이션을 진행한다.
 *
 * 애니메이션 리듬은 이 위젯이 소유한다.
 * GameMode와 전환 서브시스템은 OpenUI/CloseUI 완료만 기다리고 표현 세부 사항은 들여다보지 않는다.
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
 * 로딩 알림을 열고 이전 닫힘 타이머를 정리한다.
 *
 * 빠른 전환에서는 이전 완료 상태 타이머가 남아 있는 동안 위젯을 다시 열 수 있다.
 * 따라서 매번 열 때마다 깨끗한 타이머 상태에서 시작한다.
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
 * 최소 표시 시간이 지난 뒤 닫힘 흐름을 요청한다.
 *
 * 프리로드가 빠르게 끝나더라도 플레이어는 전환이 있었다는 것을 볼 수 있어야 한다.
 * 그래서 필요한 경우 닫힘 콜백을 지연한다.
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
	const float RemainingSeconds = FMath::Max(0.0f, mMinimumVisibleSeconds - StaticCast<float>(ElapsedSeconds));
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
 * WBP 루트가 없을 때만 우하단의 단순 로딩 인디케이터를 만든다.
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
 * 텍스트나 인디케이터 세부 사항을 밖으로 드러내지 않고 로컬 표시 단계를 갱신한다.
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
 * 네이티브 로딩 인디케이터가 있을 때 투명도를 적용한다.
 */
void ULoadingNotifyWidget::ApplyIndicatorAlpha(float Alpha) const
{
	if (mLoadingIndicator != nullptr)
	{
		mLoadingIndicator->SetRenderOpacity(FMath::Clamp(Alpha, 0.0f, 1.0f));
	}
}

/**
 * 닫힘 생명주기가 끝나기 전에 완료 상태를 보여준다.
 *
 * 전환 코드는 CloseUI를 기다리고, 완료 단계를 얼마나 보여줄지는 위젯이 결정한다.
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
 * 로딩 알림의 로컬 표시 흐름이 끝난 뒤 닫힘 생명주기를 완료한다.
 */
void ULoadingNotifyWidget::FinishCompletedState()
{
	SetLoadingState(ELoadingNotifyState::None);
	FinishCloseUI();
}
