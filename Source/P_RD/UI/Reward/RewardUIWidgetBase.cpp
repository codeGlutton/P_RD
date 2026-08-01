#include "UI/Reward/RewardUIWidgetBase.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/Reward/RewardRowWidgetBase.h"
#include "UI/ViewportZOrderType.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "RewardUIWidgetBase"

namespace
{
	const FLinearColor RewardCreamColor(0.98f, 0.92f, 0.79f, 1.0f);
	const FLinearColor RewardMutedColor(0.72f, 0.80f, 0.84f, 1.0f);
	const FLinearColor RewardTitleColor(0.96f, 0.77f, 0.30f, 1.0f);

	void SetRewardTextStyle(
		UTextBlock* Text,
		int32 FontSize,
		const FLinearColor& Color)
	{
		if (Text == nullptr)
		{
			return;
		}

		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
	}

	void PlaceOnCanvas(
		UCanvasPanel* Canvas,
		UWidget* Widget,
		const FVector2D& Position,
		const FVector2D& Size,
		int32 ZOrder)
	{
		if (Canvas == nullptr || Widget == nullptr)
		{
			return;
		}

		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget))
		{
			Slot->SetAnchors(FAnchors(0.0f, 0.0f));
			Slot->SetAlignment(FVector2D::ZeroVector);
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetAutoSize(false);
			Slot->SetZOrder(ZOrder);
		}
	}

	FText MakeRewardChoiceText(const FRewardChoiceUI& Item)
	{
		if (Item.mName.IsEmpty() == false)
		{
			return Item.mName;
		}

		switch (Item.mKind)
		{
		case ERewardChoiceKind::Equipment:
			return LOCTEXT("RewardChoiceEquipment", "장비");
		case ERewardChoiceKind::Skill:
			return LOCTEXT("RewardChoiceSkill", "스킬");
		case ERewardChoiceKind::Gold:
			return LOCTEXT("RewardChoiceGold", "골드");
		default:
			return FText::GetEmpty();
		}
	}
}

URewardUIWidgetBase::URewardUIWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);

	// 선택 보상은 데이터 에셋 아이콘을 우선하고, 비어 있을 때만 종류별 공용
	// 아이콘으로 폴백한다. 서로 다른 종류를 모두 골드로 보여 주면 보상 의미가
	// 바뀌므로 스킬/장비 아이콘도 하드 레퍼런스로 함께 잡는다.
	mRewardGoldIconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_gold_icon.T_reward_v4_gold_icon"));
	mRewardExpIconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_exp_icon.T_reward_v4_exp_icon"));

	static ConstructorHelpers::FObjectFinder<UTexture2D> EquipmentIconFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Equipment/T_equip_weapon_common.T_equip_weapon_common"));
	mEquipmentIcon = EquipmentIconFinder.Succeeded()
		? EquipmentIconFinder.Object
		: mRewardGoldIconTexture;

	static ConstructorHelpers::FObjectFinder<UTexture2D> SkillIconFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_CombatHUD_SkillIcon_Basic.T_CombatHUD_SkillIcon_Basic"));
	mSkillIcon = SkillIconFinder.Succeeded()
		? SkillIconFinder.Object
		: mRewardGoldIconTexture;

	mGoldIcon = mRewardGoldIconTexture;
	mRewardRowWidgetClass = LoadClass<URewardRowWidgetBase>(nullptr, TEXT("/Game/UI/WBP_RewardRow.WBP_RewardRow_C"));

	static ConstructorHelpers::FObjectFinder<UTexture2D> RewardBackgroundFinder(
		TEXT("/Game/UI/Art/RunFlow/T_Reward_Background_Current.T_Reward_Background_Current"));
	if (RewardBackgroundFinder.Succeeded())
	{
		mRewardBackgroundTexture = RewardBackgroundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> RewardRowFrameFinder(
		TEXT("/Game/UI/Art/RunFlow/T_Reward_RowFrame_Current.T_Reward_RowFrame_Current"));
	if (RewardRowFrameFinder.Succeeded())
	{
		mRewardRowFrameTexture = RewardRowFrameFinder.Object;
	}
}

void URewardUIWidgetBase::OpenUI(FOnEndUIOpenAnimation Callback)
{
	// 보상 화면은 결과를 읽는 화면이다. 시간 기반 등장 연출 없이 같은 프레임에 완성된 값을 보여 준다.
	mCloseCommitted = false;
	StopAllAnimations();
	Super::OpenUI(MoveTemp(Callback));

	// 파생 WBP가 과거의 Blueprint 애니메이션 이벤트를 남겨 두었더라도 계약상 즉시 완료한다.
	StopAllAnimations();
	FinishOpenUI();
}

void URewardUIWidgetBase::CloseUI(FOnEndUICloseAnimation Callback)
{
	StopAllAnimations();
	Super::CloseUI(MoveTemp(Callback));
	StopAllAnimations();
	FinishCloseUI();
}

void URewardUIWidgetBase::PlayOpenUIAnimation_Implementation()
{
	FinishOpenUI();
}

void URewardUIWidgetBase::PlayCloseUIAnimation_Implementation()
{
	FinishCloseUI();
}

/** @brief 받기 버튼 클릭을 연결하고, BindUIModel이 먼저 됐다면 들어온 값을 즉시 그린다. */
void URewardUIWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	StopAllAnimations();
	EnsureBackgroundArt();

	if (mCloseButton != nullptr)
	{
		mCloseButton->OnClicked.AddUniqueDynamic(this, &URewardUIWidgetBase::HandleCloseClicked);
		ApplyTransparentCloseButtonStyle();
	}

	RefreshRows();
	UpdateCloseButtonVisibility();
}

void URewardUIWidgetBase::EnsureBackgroundArt()
{
	if (WidgetTree == nullptr || mRewardBackgroundTexture == nullptr)
	{
		return;
	}

	UCanvasPanel* DesignCanvas = Cast<UCanvasPanel>(
		WidgetTree->FindWidget(TEXT("RewardDesignCanvas")));
	if (DesignCanvas == nullptr)
	{
		return;
	}

	// 기존 보상 시안의 불투명 금색/남색 판은 새 양피지 배경을 가린다.
	// 버튼·스크롤·텍스트는 그대로 두고 판 이미지만 접는다.
	if (UWidget* LegacyPanel = WidgetTree->FindWidget(
		TEXT("RewardElem_02_widget_00_widget")))
	{
		LegacyPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (mRewardBackgroundImage == nullptr)
	{
		mRewardBackgroundImage = Cast<UImage>(
			WidgetTree->FindWidget(TEXT("RewardBackgroundImage")));
	}
	if (mRewardBackgroundImage == nullptr)
	{
		mRewardBackgroundImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("RewardBackgroundImage"));
		DesignCanvas->AddChildToCanvas(mRewardBackgroundImage);
	}

	mRewardBackgroundImage->SetBrushFromTexture(mRewardBackgroundTexture, true);
	mRewardBackgroundImage->SetColorAndOpacity(FLinearColor::White);
	mRewardBackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UCanvasPanelSlot* BackgroundSlot = Cast<UCanvasPanelSlot>(mRewardBackgroundImage->Slot))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BackgroundSlot->SetAlignment(FVector2D::ZeroVector);
		BackgroundSlot->SetOffsets(FMargin(0.f));
		BackgroundSlot->SetAutoSize(false);
		BackgroundSlot->SetZOrder(-100);
	}

	// 기존 보상 ScrollBox는 470px 폭이라 16:9 시안에서만 간신히 맞고,
	// Fold 계열 화면에서는 새 900px 행이 470px로 압축돼 문구가 겹쳤다.
	// 새 중앙 보드의 실제 내부 영역에 맞춰 스크롤을 넓히고 기본 흰 막대는 숨긴다.
	if (UScrollBox* RowsScroll = Cast<UScrollBox>(
		WidgetTree->FindWidget(TEXT("RewardRowsScrollBox"))))
	{
		RowsScroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* RowsSlot =
			Cast<UCanvasPanelSlot>(RowsScroll->Slot))
		{
			RowsSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			RowsSlot->SetAlignment(FVector2D::ZeroVector);
			RowsSlot->SetPosition(FVector2D(386.0f, 220.0f));
			RowsSlot->SetSize(FVector2D(900.0f, 480.0f));
			RowsSlot->SetAutoSize(false);
			RowsSlot->SetZOrder(4);
		}
	}

	// WBP_Reward에는 제목 슬롯이 없어서 구형 패널을 접으면 화면 성격도
	// 같이 사라진다. 새 배경의 남색 보드 상단에 런타임 제목을 한 번 만든다.
	if (mTitleText == nullptr)
	{
		mTitleText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("RewardRuntimeTitle"));
		mTitleText->SetJustification(ETextJustify::Center);
		mTitleText->SetAutoWrapText(false);
		SetRewardTextStyle(mTitleText, 34, RewardTitleColor);
		PlaceOnCanvas(
			DesignCanvas,
			mTitleText,
			FVector2D(430.0f, 166.0f),
			FVector2D(812.0f, 48.0f),
			5);
	}
}

void URewardUIWidgetBase::HandleCloseClicked()
{
	if (mCloseCommitted || AreAllRewardRowsClaimed() == false)
	{
		return;
	}

	mCloseCommitted = true;
	if (mUIModel != nullptr)
	{
		mUIModel->RequestClaim();
	}
	OnClosed.Broadcast();
	CloseUI();
}

/** @brief 새 UIModel을 구독하고 이미 들어온 보상 스냅샷도 즉시 한 번 그린다. */
void URewardUIWidgetBase::BindUIModel(URewardUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}

	UnbindUIModel();
	mUIModel = InUIModel;

	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &URewardUIWidgetBase::HandleUIChanged);
		mUIModel->OnChoicesChanged.AddDynamic(this, &URewardUIWidgetBase::HandleChoicesChanged);
		mUIModel->OnRewardClaimConfirmed.AddDynamic(this, &URewardUIWidgetBase::HandleRewardClaimConfirmed);
		RefreshRows();
	}
}

/** @brief WBP의 받기 버튼 입력을 UIModel의 Claim 의도 이벤트로 전달한다. */
void URewardUIWidgetBase::Claim()
{
	HandleCloseClicked();
}

/** @brief 현재 UIModel 구독을 해제해 화면 파괴 후 알림이 들어오지 않게 한다. */
void URewardUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &URewardUIWidgetBase::HandleUIChanged);
		mUIModel->OnChoicesChanged.RemoveDynamic(this, &URewardUIWidgetBase::HandleChoicesChanged);
		mUIModel->OnRewardClaimConfirmed.RemoveDynamic(this, &URewardUIWidgetBase::HandleRewardClaimConfirmed);
	}
	mUIModel = nullptr;
}

void URewardUIWidgetBase::HandleUIChanged()
{
	RefreshRows();
}

void URewardUIWidgetBase::HandleChoicesChanged()
{
	RefreshRows();
}

void URewardUIWidgetBase::ApplyTransparentCloseButtonStyle() const
{
	if (mCloseButton == nullptr)
	{
		return;
	}

	FSlateBrush TransparentBrush;
	TransparentBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	TransparentBrush.TintColor = FSlateColor(FLinearColor::Transparent);

	FButtonStyle TransparentStyle;
	TransparentStyle.SetNormal(TransparentBrush);
	TransparentStyle.SetHovered(TransparentBrush);
	TransparentStyle.SetPressed(TransparentBrush);
	TransparentStyle.SetDisabled(TransparentBrush);
	TransparentStyle.SetNormalPadding(FMargin(0.0f));
	TransparentStyle.SetPressedPadding(FMargin(0.0f));
	mCloseButton->SetStyle(TransparentStyle);
}

void URewardUIWidgetBase::UpdateCloseButtonVisibility() const
{
	if (mCloseButton == nullptr)
	{
		return;
	}

	// 보상이 0개인 방도 진행을 막지 않는다. 보상이 있으면 게임플레이가 전부 지급 성공을 확정한 뒤에만 닫는다.
	const bool bCanClose = mUIModel != nullptr && AreAllRewardRowsClaimed();
	mCloseButton->SetVisibility(bCanClose ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

bool URewardUIWidgetBase::AreAllRewardRowsClaimed() const
{
	for (const FRewardClaimRow& ClaimRow : mRewardClaimRows)
	{
		if (ClaimRow.mClaimed == false)
		{
			return false;
		}
	}

	return true;
}

int32 URewardUIWidgetBase::GetClaimedRewardRowCount() const
{
	int32 ClaimedCount = 0;
	for (const FRewardClaimRow& ClaimRow : mRewardClaimRows)
	{
		ClaimedCount += ClaimRow.mClaimed ? 1 : 0;
	}
	return ClaimedCount;
}

void URewardUIWidgetBase::RefreshSummary()
{
	RefreshRows();
}

UTexture2D* URewardUIWidgetBase::GetRewardIcon(ERewardChoiceKind Kind) const
{
	switch (Kind)
	{
	case ERewardChoiceKind::Equipment:
		return mEquipmentIcon != nullptr ? mEquipmentIcon.Get() : mRewardGoldIconTexture.Get();
	case ERewardChoiceKind::Skill:
		return mSkillIcon != nullptr ? mSkillIcon.Get() : mRewardGoldIconTexture.Get();
	case ERewardChoiceKind::Gold:
		return mGoldIcon != nullptr ? mGoldIcon.Get() : mRewardGoldIconTexture.Get();
	default:
		return mRewardGoldIconTexture.Get();
	}
}

void URewardUIWidgetBase::RefreshRows()
{
	mRewardClaimRows.Reset();
	mRewardRowWidgets.Reset();
	mRewardRowFrameImages.Reset();
	mRewardRowVisuals.Reset();

	if (mUIModel == nullptr || mRewardRowsBox == nullptr)
	{
		UpdateCloseButtonVisibility();
		return;
	}

	mRewardRowsBox->ClearChildren();

	auto AddRow = [this](ERewardClaimKind ClaimKind, int32 ChoiceIndex, const FText& MainText, const FText& SubText, UTexture2D* IconTexture)
	{
		if (mRewardRowWidgetClass == nullptr)
		{
			mRewardRowWidgetClass = LoadClass<URewardRowWidgetBase>(nullptr, TEXT("/Game/UI/WBP_RewardRow.WBP_RewardRow_C"));
		}
		if (mRewardRowWidgetClass == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("WBP_RewardRow 클래스를 찾지 못해 보상 행을 생성하지 못했습니다."));
			return;
		}

		URewardRowWidgetBase* RowWidget = CreateWidget<URewardRowWidgetBase>(GetWorld(), mRewardRowWidgetClass);
		if (RowWidget == nullptr)
		{
			return;
		}

		const int32 RewardRowIndex = mRewardClaimRows.Add(FRewardClaimRow{ ClaimKind, ChoiceIndex, false });

		RowWidget->SetRewardRow(MainText, SubText, IconTexture);
		RowWidget->SetRewardIndex(RewardRowIndex);
		RowWidget->OnRewardRowClicked.AddUniqueDynamic(this, &URewardUIWidgetBase::HandleRewardRowClicked);
		mRewardRowWidgets.Add(RowWidget);

		UWidget* RowHost = RowWidget;
		if (WidgetTree != nullptr && mRewardRowFrameTexture != nullptr)
		{
			const FString RowSuffix = FString::FromInt(RewardRowIndex);

			USizeBox* RowSize = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				*FString::Printf(TEXT("RewardRowFrameSize_%s"), *RowSuffix));
			RowSize->SetWidthOverride(mRewardRowSize.X);
			RowSize->SetHeightOverride(mRewardRowSize.Y);
			RowSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			UOverlay* RowOverlay = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				*FString::Printf(TEXT("RewardRowOverlay_%s"), *RowSuffix));
			RowOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			RowSize->AddChild(RowOverlay);

			UImage* RowFrame = WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(),
				*FString::Printf(TEXT("RewardRowFrame_%s"), *RowSuffix));
			FSlateBrush RowFrameBrush;
			RowFrameBrush.SetResourceObject(mRewardRowFrameTexture);
			RowFrameBrush.DrawAs = ESlateBrushDrawType::Box;
			// 왼쪽 아이콘 소켓과 네 귀퉁이는 보존하고, 가운데 남색 면만
			// 가로로 늘린다. 1536x384 원본의 실제 장식 비율에 맞춘 값이다.
			RowFrameBrush.Margin = FMargin(0.22f, 0.28f, 0.11f, 0.28f);
			RowFrame->SetBrush(RowFrameBrush);
			RowFrame->SetColorAndOpacity(FLinearColor::White);
			RowFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UOverlaySlot* FrameSlot = RowOverlay->AddChildToOverlay(RowFrame))
			{
				FrameSlot->SetHorizontalAlignment(HAlign_Fill);
				FrameSlot->SetVerticalAlignment(VAlign_Fill);
			}

			UCanvasPanel* RowVisual = WidgetTree->ConstructWidget<UCanvasPanel>(
				UCanvasPanel::StaticClass(),
				*FString::Printf(TEXT("RewardRowVisual_%s"), *RowSuffix));
			RowVisual->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UOverlaySlot* VisualSlot = RowOverlay->AddChildToOverlay(RowVisual))
			{
				VisualSlot->SetHorizontalAlignment(HAlign_Fill);
				VisualSlot->SetVerticalAlignment(VAlign_Fill);
			}

			UImage* RewardIcon = WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(),
				*FString::Printf(TEXT("RewardRowIcon_%s"), *RowSuffix));
			if (IconTexture != nullptr)
			{
				RewardIcon->SetBrushFromTexture(IconTexture, false);
			}
			RewardIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
			PlaceOnCanvas(
				RowVisual,
				RewardIcon,
				FVector2D(61.0f, 17.0f),
				FVector2D(72.0f, 72.0f),
				1);

			const bool bHasSubText = SubText.IsEmpty() == false;
			UTextBlock* MainLabel = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				*FString::Printf(TEXT("RewardRowMainText_%s"), *RowSuffix));
			MainLabel->SetText(MainText);
			MainLabel->SetAutoWrapText(false);
			SetRewardTextStyle(MainLabel, 24, RewardCreamColor);
			PlaceOnCanvas(
				RowVisual,
				MainLabel,
				FVector2D(174.0f, bHasSubText ? 30.0f : 36.0f),
				FVector2D(676.0f, 36.0f),
				1);

			if (bHasSubText)
			{
				UTextBlock* SubLabel = WidgetTree->ConstructWidget<UTextBlock>(
					UTextBlock::StaticClass(),
					*FString::Printf(TEXT("RewardRowSubText_%s"), *RowSuffix));
				SubLabel->SetText(SubText);
				SubLabel->SetAutoWrapText(false);
				SetRewardTextStyle(SubLabel, 15, RewardMutedColor);
				PlaceOnCanvas(
					RowVisual,
					SubLabel,
					FVector2D(174.0f, 60.0f),
					FVector2D(676.0f, 28.0f),
					1);
			}

			// 새 행은 프레임·아이콘·문구를 모두 직접 그린다. 구형 WBP는
			// 보이지 않는 전체 행 입력면으로만 남겨 기존 click/claim 계약을 보존한다.
			const FName LegacyVisualNames[] = {
				TEXT("mRowIconFrame"),
				TEXT("mRewardIcon"),
				TEXT("mRewardSingleText"),
				TEXT("mRewardMainText"),
				TEXT("mRewardSubText"),
			};
			for (const FName LegacyVisualName : LegacyVisualNames)
			{
				if (UWidget* LegacyVisual =
					RowWidget->GetWidgetFromName(LegacyVisualName))
				{
					LegacyVisual->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			if (UOverlaySlot* InputSlot = RowOverlay->AddChildToOverlay(RowWidget))
			{
				InputSlot->SetHorizontalAlignment(HAlign_Fill);
				InputSlot->SetVerticalAlignment(VAlign_Fill);
			}

			mRewardRowFrameImages.Add(RowFrame);
			mRewardRowVisuals.Add(RowSize);
			RowHost = RowSize;
		}

		if (UVerticalBoxSlot* RowSlot = mRewardRowsBox->AddChildToVerticalBox(RowHost))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Center);
			RowSlot->SetVerticalAlignment(VAlign_Center);
			RowSlot->SetPadding(mRewardRowPadding);
		}
	};

	const FRewardUI& Reward = mUIModel->GetReward();
	if (mTitleText != nullptr)
	{
		mTitleText->SetText(Reward.mTitle);
	}

	UTexture2D* SummaryIcon = mGoldIcon != nullptr ? mGoldIcon.Get() : mRewardGoldIconTexture.Get();

	if (Reward.mGoldGained != 0)
	{
		AddRow(
			ERewardClaimKind::Gold,
			INDEX_NONE,
			FText::Format(LOCTEXT("GoldRewardValue", "골드 +{0}"), FText::AsNumber(Reward.mGoldGained)),
			FText::Format(LOCTEXT("GoldRewardBalance", "보유 골드 {0}"), FText::AsNumber(Reward.mGoldBalance)),
			SummaryIcon);
	}
	if (Reward.mExpGained != 0)
	{
		// 경험치 행은 전용 아이콘(없으면 골드 아이콘 폴백).
		UTexture2D* ExpIcon = mRewardExpIconTexture != nullptr ? mRewardExpIconTexture.Get() : SummaryIcon;

		FText ExpProgressText = FText::GetEmpty();
		if (Reward.mMercenaryExp.IsEmpty() == false)
		{
			TArray<FString> MercenaryProgress;
			MercenaryProgress.Reserve(Reward.mMercenaryExp.Num());
			for (const FRewardMercenaryExpUI& Mercenary : Reward.mMercenaryExp)
			{
				const FString Progress = Mercenary.mMaxExp > 0.f
					? FString::Printf(TEXT("Lv.%d  %d/%d"),
						Mercenary.mLevel,
						FMath::RoundToInt(Mercenary.mExpAfter),
						FMath::RoundToInt(Mercenary.mMaxExp))
					: FString::Printf(TEXT("Lv.%d  %d"),
						Mercenary.mLevel,
						FMath::RoundToInt(Mercenary.mExpAfter));
				MercenaryProgress.Add(FText::Format(
					LOCTEXT("MercenaryExpProgress", "{0}  {1}"),
					Mercenary.mName,
					FText::FromString(Progress)).ToString());
			}
			ExpProgressText = FText::FromString(
				FString::Join(MercenaryProgress, TEXT("   |   ")));
		}
		else if (Reward.mMaxExp > 0.0f)
		{
			const int32 CurrentExp = FMath::RoundToInt(Reward.mExpAfter);
			const int32 MaxExp = FMath::RoundToInt(Reward.mMaxExp);
			if (Reward.mLevelAfter > 0 && Reward.mLevelAfter != Reward.mLevelBefore)
			{
				ExpProgressText = FText::Format(
					LOCTEXT("ExpRewardLevelUpProgress", "Lv.{0} → Lv.{1} · {2}/{3}"),
					FText::AsNumber(Reward.mLevelBefore),
					FText::AsNumber(Reward.mLevelAfter),
					FText::AsNumber(CurrentExp),
					FText::AsNumber(MaxExp));
			}
			else if (Reward.mLevelAfter > 0)
			{
				ExpProgressText = FText::Format(
					LOCTEXT("ExpRewardProgressWithLevel", "Lv.{0} · {1}/{2}"),
					FText::AsNumber(Reward.mLevelAfter),
					FText::AsNumber(CurrentExp),
					FText::AsNumber(MaxExp));
			}
			else
			{
				ExpProgressText = FText::Format(
					LOCTEXT("ExpRewardProgress", "{0}/{1}"),
					FText::AsNumber(CurrentExp),
					FText::AsNumber(MaxExp));
			}
		}

		AddRow(
			ERewardClaimKind::Exp,
			INDEX_NONE,
			FText::Format(
				Reward.mMercenaryExp.Num() > 1
					? LOCTEXT("PartyExpRewardValue", "모든 용병 경험치 +{0}")
					: LOCTEXT("ExpRewardValue", "경험치 +{0}"),
				FText::AsNumber(Reward.mExpGained)),
			ExpProgressText,
			ExpIcon);
	}

	const TArray<FRewardChoiceUI>& Items = mUIModel->GetRewardChoices();
	for (const FRewardChoiceUI& Item : Items)
	{
		UTexture2D* IconTexture = Item.mIcon != nullptr ? Item.mIcon.Get() : GetRewardIcon(Item.mKind);
		AddRow(ERewardClaimKind::Choice, Item.mChoiceIndex, MakeRewardChoiceText(Item), Item.mDescription, IconTexture);
	}

	UpdateCloseButtonVisibility();
}

void URewardUIWidgetBase::HandleRewardRowClicked(int32 RewardRowIndex)
{
	if (mRewardClaimRows.IsValidIndex(RewardRowIndex) == false || mRewardClaimRows[RewardRowIndex].mClaimed)
	{
		return;
	}

	// 지급 의도만 게임플레이로 보낸다. 실제 지급 성공 여부와 행 제거는 게임플레이 확정(NotifyRewardClaimed)이 결정한다.
	// RequestClaimReward는 동기적으로 게임플레이를 거쳐 NotifyRewardClaimed까지 재진입할 수 있으므로,
	// 값을 복사해 호출하고 이후 이 행 인덱스를 다시 건드리지 않는다(재진입 중 배열 상태 변경 대비).
	const FRewardClaimRow ClaimRow = mRewardClaimRows[RewardRowIndex];
	if (mUIModel != nullptr)
	{
		mUIModel->RequestClaimReward(ClaimRow.mKind, ClaimRow.mChoiceIndex);
	}
}

void URewardUIWidgetBase::HandleRewardClaimConfirmed(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	NotifyRewardClaimed(ClaimKind, ChoiceIndex);
}

/** @brief 게임플레이가 지급을 확정한 보상 행을 목록에 남긴 채 수령 완료 상태로 바꾼다. */
void URewardUIWidgetBase::NotifyRewardClaimed(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	for (int32 RowIndex = 0; RowIndex < mRewardClaimRows.Num(); ++RowIndex)
	{
		FRewardClaimRow& ClaimRow = mRewardClaimRows[RowIndex];
		if (ClaimRow.mClaimed || ClaimRow.mKind != ClaimKind || ClaimRow.mChoiceIndex != ChoiceIndex)
		{
			continue;
		}

		// 결과를 사라지게 하지 않는다. 모든 보상을 한눈에 확인할 수 있고, 재클릭만 막는다.
		ClaimRow.mClaimed = true;
		if (mRewardRowWidgets.IsValidIndex(RowIndex) && mRewardRowWidgets[RowIndex] != nullptr)
		{
			mRewardRowWidgets[RowIndex]->SetClaimed(true);
		}
		if (mRewardRowVisuals.IsValidIndex(RowIndex)
			&& mRewardRowVisuals[RowIndex] != nullptr)
		{
			mRewardRowVisuals[RowIndex]->SetRenderOpacity(0.62f);
		}
		break;
	}

	UpdateCloseButtonVisibility();
}

/** @brief 위젯 생명주기 종료 시 UIModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void URewardUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
