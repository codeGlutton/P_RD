#include "UI/FadeInOutWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/ViewportZOrderType.h"

/**
 * 페이드 위젯은 방 전환용 오버레이 레이어를 소유한다.
 *
 * 호출자는 OpenUI로 레이어를 올린 뒤 필요한 페이드 방향을 명시한다.
 * 전체 화면 페이드 효과가 뷰포트 스택의 어느 위치에 있어야 하는지는 이 위젯이 결정한다.
 */
UFadeInOutWidget::UFadeInOutWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::FadeInOut);
}

/**
 * 검은 덮개를 걷어 새 방 화면을 보여준다.
 *
 * 페이드인이 끝난 뒤 위젯을 닫을지는 호출자가 전달한 완료 콜백에서 결정한다.
 */
void UFadeInOutWidget::StartFadeIn(FOnEndFadeInAnimation InOnEndFadeInAnimation)
{
	OnEndFadeInAnimation = MoveTemp(InOnEndFadeInAnimation);
	StartFade(GetRenderOpacity(), 0.0f, mFadeInSeconds, EFadeInOutAnimationType::FadeIn);
}

/**
 * 실제 레벨 전환이 시작되기 전에 화면을 검게 덮는다.
 */
void UFadeInOutWidget::StartFadeOut(FOnEndFadeOutAnimation InOnEndFadeOutAnimation)
{
	OnEndFadeOutAnimation = MoveTemp(InOnEndFadeOutAnimation);
	StartFade(0.0f, 1.0f, mFadeOutSeconds, EFadeInOutAnimationType::FadeOut);
}

/**
 * WBP 루트가 없을 때 네이티브 최소 화면을 만든다.
 *
 * 디자이너가 만든 WBP 콘텐츠가 준비되기 전에도 전환 타이밍을 검증할 수 있게 하기 위한 대체 경로다.
 * WBP가 자체 루트를 제공하면 그 루트가 최종 화면 구성을 담당한다.
 */
bool UFadeInOutWidget::Initialize()
{
	const bool bIsInitialized = Super::Initialize();
	EnsureDefaultVisual();
	return bIsInitialized;
}

/**
 * 런타임뿐 아니라 에디터 미리보기에서도 기본 화면을 사용할 수 있게 한다.
 */
void UFadeInOutWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureDefaultVisual();
}

/**
 * 네이티브 페이드를 진행시키고 페이드 방향에 맞는 완료 콜백을 확정한다.
 *
 * GameMode와 전환 서브시스템은 콜백 완료만 기다린다.
 * 실제 표현은 현재의 네이티브 투명도 틱일 수도 있고, 이후 WBP 애니메이션으로 교체될 수도 있다.
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
	FinishFade();
}

/**
 * 공통 OpenUI는 전환 레이어를 표시 가능한 상태로 만드는 일만 맡는다.
 */
void UFadeInOutWidget::PlayOpenUIAnimation_Implementation()
{
	FinishOpenUI();
}

/**
 * 공통 CloseUI는 페이드가 끝난 전환 레이어를 정리하는 일만 맡는다.
 */
void UFadeInOutWidget::PlayCloseUIAnimation_Implementation()
{
	FinishCloseUI();
}

/**
 * WBP 루트가 없을 때만 단순한 전체 화면 검은 오버레이를 만든다.
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
 * 하나의 완료 경로로 페이드 요청을 시작한다.
 *
 * 지속 시간이 0인 페이드와 틱으로 진행되는 페이드 모두 FinishFade()로 끝난다.
 * 호출자는 페이드 구현 방식과 무관하게 같은 완료 콜백만 관찰하면 된다.
 */
void UFadeInOutWidget::StartFade(float StartAlpha, float EndAlpha, float Duration, EFadeInOutAnimationType FadeAnimationType)
{
	mFadeStartAlpha = StartAlpha;
	mFadeEndAlpha = EndAlpha;
	mFadeElapsedSeconds = 0.0f;
	mFadeDurationSeconds = FMath::Max(0.0f, Duration);
	mFadeAnimationType = FadeAnimationType;
	mFadeAnimationPlaying = true;

	ApplyFadeAlpha(StartAlpha);
	if (mFadeDurationSeconds <= 0.0f)
	{
		ApplyFadeAlpha(EndAlpha);
		mFadeAnimationPlaying = false;
		FinishFade();
	}
}

/**
 * 페이드 완료 후 방향별 후속 처리를 실행한다.
 */
void UFadeInOutWidget::FinishFade()
{
	const EFadeInOutAnimationType FinishedFadeAnimationType = mFadeAnimationType;
	mFadeAnimationType = EFadeInOutAnimationType::None;

	switch (FinishedFadeAnimationType)
	{
	case EFadeInOutAnimationType::FadeIn:
		if (OnEndFadeInAnimation.IsBound())
		{
			OnEndFadeInAnimation.Execute(this);
			OnEndFadeInAnimation.Unbind();
		}
		break;
	case EFadeInOutAnimationType::FadeOut:
		if (OnEndFadeOutAnimation.IsBound())
		{
			OnEndFadeOutAnimation.Execute(this);
			OnEndFadeOutAnimation.Unbind();
		}
		break;
	default:
		break;
	}
}

/**
 * 현재 페이드 투명도를 위젯 전체에 적용한다.
 */
void UFadeInOutWidget::ApplyFadeAlpha(float Alpha)
{
	SetRenderOpacity(FMath::Clamp(Alpha, 0.0f, 1.0f));
}
