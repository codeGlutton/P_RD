#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "UI/Combat/CombatUIModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

void UCombatLayoutHUDWidget::EnsureCombatAnnouncementWidgets()
{
	if (WidgetTree == nullptr || mRootCanvas == nullptr || mCombatAnnouncementRoot != nullptr)
	{
		return;
	}

	// 연출이 떠 있는 동안 월드 입력만 삼킨다. 그림은 없는 투명 버튼이다.
	mTurnChangeInputBlocker = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(), TEXT("CombatAnnouncementInputBlocker"));
	if (mTurnChangeInputBlocker != nullptr)
	{
		FButtonStyle Clear = mTurnChangeInputBlocker->GetStyle();
		Clear.Normal.TintColor = FSlateColor(FLinearColor::Transparent);
		Clear.Hovered.TintColor = FSlateColor(FLinearColor::Transparent);
		Clear.Pressed.TintColor = FSlateColor(FLinearColor::Transparent);
		Clear.Disabled.TintColor = FSlateColor(FLinearColor::Transparent);
		mTurnChangeInputBlocker->SetStyle(Clear);
		mTurnChangeInputBlocker->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* BlockerSlot =
			mRootCanvas->AddChildToCanvas(mTurnChangeInputBlocker))
		{
			BlockerSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			BlockerSlot->SetOffsets(FMargin(0.f));
			BlockerSlot->SetZOrder(899);
		}
	}

	mCombatAnnouncementRoot = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("CombatAnnouncementRoot"));
	mCombatAnnouncementText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("CombatAnnouncementText"));
	if (mCombatAnnouncementRoot == nullptr || mCombatAnnouncementText == nullptr)
	{
		return;
	}

	mCombatAnnouncementRoot->SetBrushColor(FLinearColor(0.015f, 0.01f, 0.025f, 0.72f));
	mCombatAnnouncementRoot->SetHorizontalAlignment(HAlign_Center);
	mCombatAnnouncementRoot->SetVerticalAlignment(VAlign_Center);
	mCombatAnnouncementRoot->SetVisibility(ESlateVisibility::Collapsed);
	mCombatAnnouncementRoot->AddChild(mCombatAnnouncementText);

	mCombatAnnouncementText->SetJustification(ETextJustify::Center);
	mCombatAnnouncementText->SetColorAndOpacity(
		FSlateColor(FLinearColor(1.0f, 0.84f, 0.32f, 1.0f)));
	mCombatAnnouncementText->SetShadowOffset(FVector2D(4.0f, 5.0f));
	mCombatAnnouncementText->SetShadowColorAndOpacity(
		FLinearColor(0.0f, 0.0f, 0.0f, 0.92f));
	FSlateFontInfo Font = mCombatAnnouncementText->GetFont();
	Font.Size = 82;
	mCombatAnnouncementText->SetFont(Font);

	if (UCanvasPanelSlot* AnnouncementSlot = mRootCanvas->AddChildToCanvas(mCombatAnnouncementRoot))
	{
		AnnouncementSlot->SetAnchors(FAnchors(0.0f, 0.36f, 1.0f, 0.64f));
		AnnouncementSlot->SetOffsets(FMargin(0.0f));
		AnnouncementSlot->SetZOrder(900);
	}
}

bool UCombatLayoutHUDWidget::PlayCombatAnnouncement(const FText& Text,
	const ECombatAnnouncementKind Kind, TSharedPtr<FPresentationBarrier> Barrier)
{
	// 프리뷰/단위 시험은 프레젠테이션 배리어 없이 상태만 흘린다. 실제 전투
	// 흐름에서만 시간을 점유해 에디터 프리뷰가 1초 동안 멈추지 않게 한다.
	if (Barrier.IsValid() == false)
	{
		return false;
	}
	EnsureCombatAnnouncementWidgets();
	if (mCombatAnnouncementRoot == nullptr || mCombatAnnouncementText == nullptr)
	{
		return false;
	}
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	if (mCombatAnnouncementPlaying)
	{
		FinishCombatAnnouncement();
	}

	mCombatAnnouncementKind = Kind;
	mCombatAnnouncementBarrier = MoveTemp(Barrier);
	mCombatAnnouncementElapsed = 0.0f;
	switch (Kind)
	{
	case ECombatAnnouncementKind::CombatStart:
		mCombatAnnouncementDuration = 1.15f;
		break;
	case ECombatAnnouncementKind::RoundStart:
		mCombatAnnouncementDuration = .90f;
		break;
	case ECombatAnnouncementKind::TurnStart:
	default:
		mCombatAnnouncementDuration = .75f;
		break;
	}
	mCombatAnnouncementPlaying = true;
	mCombatAnnouncementText->SetText(Text);
	mCombatAnnouncementRoot->SetRenderOpacity(0.0f);
	mCombatAnnouncementRoot->SetRenderScale(FVector2D(0.92f, 1.0f));
	mCombatAnnouncementRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (mTurnChangeInputBlocker != nullptr)
	{
		mTurnChangeInputBlocker->SetVisibility(ESlateVisibility::Visible);
	}

	World->GetTimerManager().SetTimer(mCombatAnnouncementTimerHandle,
		this, &UCombatLayoutHUDWidget::FinishCombatAnnouncement,
		mCombatAnnouncementDuration, false);
	return true;
}

void UCombatLayoutHUDWidget::FinishCombatAnnouncement()
{
	if (mCombatAnnouncementPlaying == false)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mCombatAnnouncementTimerHandle);
	}

	const ECombatAnnouncementKind FinishedKind = mCombatAnnouncementKind;
	mCombatAnnouncementPlaying = false;
	mCombatAnnouncementKind = ECombatAnnouncementKind::None;
	mCombatAnnouncementElapsed = 0.0f;
	if (mCombatAnnouncementRoot != nullptr)
	{
		mCombatAnnouncementRoot->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (mTurnChangeInputBlocker != nullptr)
	{
		mTurnChangeInputBlocker->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (FinishedKind == ECombatAnnouncementKind::TurnStart)
	{
		CompleteTurnPresentationBegin();
	}
	// 마지막에 놓아 프레임워크 재진입 중 상태가 다시 덮이지 않게 한다.
	mCombatAnnouncementBarrier.Reset();
}

void UCombatLayoutHUDWidget::UpdateCombatAnnouncement(const float DeltaTime)
{
	if (mCombatAnnouncementPlaying == false || mCombatAnnouncementRoot == nullptr)
	{
		return;
	}
	mCombatAnnouncementElapsed += FMath::Max(0.0f, DeltaTime);
	const float Progress = FMath::Clamp(
		mCombatAnnouncementElapsed / FMath::Max(mCombatAnnouncementDuration, 0.01f),
		0.0f, 1.0f);
	const float FadeIn = FMath::Clamp(Progress / 0.16f, 0.0f, 1.0f);
	const float FadeOut = FMath::Clamp((1.0f - Progress) / 0.20f, 0.0f, 1.0f);
	const float Opacity = FMath::Min(FadeIn, FadeOut);
	mCombatAnnouncementRoot->SetRenderOpacity(Opacity);
	mCombatAnnouncementRoot->SetRenderScale(FVector2D(
		FMath::Lerp(0.92f, 1.0f, FMath::InterpEaseOut(0.0f, 1.0f, FadeIn, 3.0f)),
		1.0f));
}

FText UCombatLayoutHUDWidget::GetCurrentTurnAnnouncementText() const
{
	const FUnitUI* TurnUnit = FindTurnUnit();
	const FText UnitName = TurnUnit != nullptr && TurnUnit->mName.IsEmpty() == false
		? TurnUnit->mName
		: NSLOCTEXT("CombatAnnouncement", "UnknownUnit", "UNIT");
	return FText::Format(
		NSLOCTEXT("CombatAnnouncement", "UnitTurn", "{0}'S TURN"), UnitName);
}
