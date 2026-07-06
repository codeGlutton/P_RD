#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/Combat/CombatUIModel.h"

// 이 파일: 전투 플로팅 로그(유닛 머리 위 HP 증감 텍스트) + 턴 라운드 배너("N번째 턴").
// 데이터는 CombatUIModel 경계(OnCombatFloatingLog / FTurnUI.mRound)로만 받고,
// 표시는 머리 위 HP바(UnitBars)와 같은 월드→스크린 투영 패턴을 쓴다.

namespace
{
	constexpr float FloatingLogLifetime = 1.2f;       // 플로팅 로그 표시 수명(초)
	constexpr float FloatingLogQueueInterval = 0.28f; // 순차 로그 간 표시 간격(초)
	constexpr float FloatingLogRiseSpeed = 46.0f;     // 초당 상승 픽셀
	constexpr float FloatingLogBaseOffsetY = -96.0f;  // 머리 위 HP바(-70)보다 위에서 시작
	constexpr float FloatingLogFadePortion = 0.6f;    // 수명의 이 비율 이후부터 페이드아웃
	constexpr float TurnBannerLifetime = 1.6f;        // 배너 표시 수명(초)
	constexpr float TurnBannerFadePortion = 0.55f;    // 배너 페이드 시작 비율

	FLinearColor ResolveFloatingLogColor(EFloatingLogColorType ColorType)
	{
		switch (ColorType)
		{
		case EFloatingLogColorType::Damage:
			return FLinearColor(1.0f, 0.25f, 0.2f, 1.0f);
		case EFloatingLogColorType::Heal:
			return FLinearColor(0.35f, 1.0f, 0.4f, 1.0f);
		case EFloatingLogColorType::Buff:
			return FLinearColor(0.45f, 0.75f, 1.0f, 1.0f);
		case EFloatingLogColorType::Debuff:
			return FLinearColor(0.75f, 0.45f, 1.0f, 1.0f);
		case EFloatingLogColorType::Warning:
			return FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);
		case EFloatingLogColorType::Move:
			return FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);
		case EFloatingLogColorType::Neutral:
		default:
			return FLinearColor::White;
		}
	}

	UTexture2D* ResolveFloatingLogIcon(EFloatingLogIconType IconType)
	{
		// None은 아이콘 없음(텍스트만). 나머지 아이콘 종류는 임시로 전부 HP(하트) 텍스처로 연결한다.
		// Poison/Fire/Shield/Move 전용 아이콘 에셋이 준비되면 case를 분리해 경로만 갈아끼우면 됨.
		if (IconType == EFloatingLogIconType::None)
		{
			return nullptr;
		}

		return LoadObject<UTexture2D>(
			nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ClassSelect/T_icon_hp.T_icon_hp"));
	}
}

void UCombatTileMapHUDWidget::HandleCombatFloatingLog(FCombatFloatingLogRequest Request)
{
	FQueuedFloatingCombatLogEntry Entry;
	Entry.mRequest = Request;
	Entry.mArrivalOrder = mNextFloatingCombatLogArrivalOrder++;
	mPendingFloatingCombatLogs.Add(Entry);
}

void UCombatTileMapHUDWidget::UpdateFloatingCombatLogQueue(float InDeltaTime)
{
	if (mPendingFloatingCombatLogs.Num() == 0)
	{
		mFloatingCombatLogQueueCooldown = 0.0f;
		return;
	}

	mFloatingCombatLogQueueCooldown -= InDeltaTime;
	if (mFloatingCombatLogQueueCooldown > 0.0f)
	{
		return;
	}

	const FCombatFloatingLogRequest Request = mPendingFloatingCombatLogs[0].mRequest;
	mPendingFloatingCombatLogs.RemoveAt(0);
	SpawnFloatingCombatLogAtWorld(Request);
	mFloatingCombatLogQueueCooldown = FloatingLogQueueInterval;
}

void UCombatTileMapHUDWidget::SpawnFloatingCombatLogAtWorld(const FCombatFloatingLogRequest& Request)
{
	if (RootCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	// 아이콘+텍스트를 한 덩어리로 움직이게 가로 박스에 담는다(아이콘 없으면 텍스트만).
	UHorizontalBox* LogBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* LogText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (LogBox == nullptr || LogText == nullptr)
	{
		return;
	}
	LogBox->SetVisibility(ESlateVisibility::HitTestInvisible);

	UTexture2D* Icon = ResolveFloatingLogIcon(Request.mIconType);
	if (Icon != nullptr)
	{
		if (UImage* LogIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
		{
			LogIcon->SetBrushFromTexture(Icon, false);
			LogIcon->SetDesiredSizeOverride(FVector2D(26.0f, 26.0f));
			if (UHorizontalBoxSlot* IconSlot = LogBox->AddChildToHorizontalBox(LogIcon))
			{
				IconSlot->SetVerticalAlignment(VAlign_Center);
				IconSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
			}
		}
	}

	LogText->SetJustification(ETextJustify::Center);
	LogText->SetText(Request.mText);
	LogText->SetColorAndOpacity(FSlateColor(ResolveFloatingLogColor(Request.mColorType)));
	// 기본 폰트에서 크기만 키운다(전투 화면 위에서 읽히는 최소 크기).
	FSlateFontInfo LogFont = LogText->GetFont();
	LogFont.Size = 22;
	LogText->SetFont(LogFont);
	if (UHorizontalBoxSlot* TextSlot = LogBox->AddChildToHorizontalBox(LogText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UCanvasPanelSlot* LogSlot = RootCanvas->AddChildToCanvas(LogBox))
	{
		LogSlot->SetAutoSize(true);
		LogSlot->SetAlignment(FVector2D(0.5f, 1.0f)); // 바닥-중앙 기준으로 머리 위에 선다.
		LogSlot->SetZOrder(30);                        // HP바보다 위에 그린다.
	}

	FFloatingCombatLogEntry Entry;
	Entry.mRoot = LogBox;
	Entry.mWorldLocation = Request.mWorldLocation;
	Entry.mElapsed = 0.0f;
	mFloatingCombatLogs.Add(Entry);

	// 같은 위치에 로그가 연달아 뜰 때(예: 데미지+쓰러짐) 겹치지 않게, 기존 로그를 한 칸씩 위로 민다.
	for (int32 LogIndex = 0; LogIndex < mFloatingCombatLogs.Num() - 1; ++LogIndex)
	{
		FFloatingCombatLogEntry& Existing = mFloatingCombatLogs[LogIndex];
		if (Existing.mWorldLocation.Equals(Entry.mWorldLocation, 1.0f))
		{
			Existing.mElapsed = FMath::Max(Existing.mElapsed, 0.35f);
		}
	}
}

void UCombatTileMapHUDWidget::UpdateFloatingCombatLogs(float InDeltaTime)
{
	if (mFloatingCombatLogs.Num() == 0)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();

	for (int32 LogIndex = mFloatingCombatLogs.Num() - 1; LogIndex >= 0; --LogIndex)
	{
		FFloatingCombatLogEntry& Entry = mFloatingCombatLogs[LogIndex];
		Entry.mElapsed += InDeltaTime;

		UWidget* LogRoot = Entry.mRoot;
		if (LogRoot == nullptr || Entry.mElapsed >= FloatingLogLifetime)
		{
			if (LogRoot != nullptr)
			{
				LogRoot->RemoveFromParent();
			}
			mFloatingCombatLogs.RemoveAt(LogIndex);
			continue;
		}

		// HP바와 같은 투영. 화면 밖이면 숨기되 수명은 계속 흘려보낸다.
		FVector2D ScreenPosition;
		const bool bOnScreen = PlayerController != nullptr
			&& UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
				PlayerController, Entry.mWorldLocation, ScreenPosition, false);
		if (bOnScreen == false)
		{
			LogRoot->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		LogRoot->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (UCanvasPanelSlot* LogSlot = Cast<UCanvasPanelSlot>(LogRoot->Slot))
		{
			LogSlot->SetPosition(ScreenPosition
				+ FVector2D(0.0f, FloatingLogBaseOffsetY - FloatingLogRiseSpeed * Entry.mElapsed));
		}

		// 수명 후반부에 서서히 사라진다.
		const float FadeStart = FloatingLogLifetime * FloatingLogFadePortion;
		const float Opacity = Entry.mElapsed <= FadeStart
			? 1.0f
			: 1.0f - (Entry.mElapsed - FadeStart) / (FloatingLogLifetime - FadeStart);
		LogRoot->SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
	}
}

void UCombatTileMapHUDWidget::RefreshTurnRoundBanner()
{
	if (mCombatUIModel == nullptr || RootCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	// 라운드가 실제로 바뀔 때만 배너를 띄운다. 같은 라운드 안의 턴 전환/스냅샷 push에는 반응하지 않는다.
	const int32 Round = mCombatUIModel->GetTurnUI().mRound;
	if (Round <= 0 || Round == mLastShownTurnRound)
	{
		return;
	}
	mLastShownTurnRound = Round;

	if (mTurnRoundBannerText == nullptr)
	{
		mTurnRoundBannerText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("TurnRoundBannerText"));
		if (mTurnRoundBannerText == nullptr)
		{
			return;
		}
		mTurnRoundBannerText->SetVisibility(ESlateVisibility::HitTestInvisible);
		mTurnRoundBannerText->SetJustification(ETextJustify::Center);
		mTurnRoundBannerText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.92f, 0.62f, 1.0f)));
		FSlateFontInfo BannerFont = mTurnRoundBannerText->GetFont();
		BannerFont.Size = 34;
		mTurnRoundBannerText->SetFont(BannerFont);

		if (UCanvasPanelSlot* BannerSlot = RootCanvas->AddChildToCanvas(mTurnRoundBannerText))
		{
			BannerSlot->SetAutoSize(true);
			BannerSlot->SetAnchors(FAnchors(0.5f, 0.2f)); // 중앙 상단(탑바 아래)
			BannerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			BannerSlot->SetPosition(FVector2D::ZeroVector);
			BannerSlot->SetZOrder(31);
		}
	}

	mTurnRoundBannerText->SetText(FText::Format(
		NSLOCTEXT("CombatTileMapHUDWidget", "TurnRoundBanner", "{0}번째 턴"), FText::AsNumber(Round)));
	mTurnRoundBannerText->SetRenderOpacity(1.0f);
	mTurnRoundBannerText->SetVisibility(ESlateVisibility::HitTestInvisible);
	mTurnRoundBannerElapsed = 0.0f;
}

void UCombatTileMapHUDWidget::UpdateTurnRoundBanner(float InDeltaTime)
{
	if (mTurnRoundBannerText == nullptr
		|| mTurnRoundBannerText->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return;
	}

	mTurnRoundBannerElapsed += InDeltaTime;
	if (mTurnRoundBannerElapsed >= TurnBannerLifetime)
	{
		mTurnRoundBannerText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const float FadeStart = TurnBannerLifetime * TurnBannerFadePortion;
	const float Opacity = mTurnRoundBannerElapsed <= FadeStart
		? 1.0f
		: 1.0f - (mTurnRoundBannerElapsed - FadeStart) / (TurnBannerLifetime - FadeStart);
	mTurnRoundBannerText->SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
}
