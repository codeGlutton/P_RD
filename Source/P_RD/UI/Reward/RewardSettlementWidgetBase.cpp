#include "UI/Reward/RewardSettlementWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UI/ViewportZOrderType.h"
#include "UI/Reward/RewardUIModel.h"

#define LOCTEXT_NAMESPACE "RewardSettlementWidget"

namespace
{
	const FLinearColor Ink(0.12f, 0.065f, 0.025f, 1.0f);
	const FLinearColor Cream(1.0f, 0.93f, 0.78f, 1.0f);
	const FLinearColor Blue(0.02f, 0.35f, 0.78f, 1.0f);

	FSlateBrush TextureBrush(UTexture2D* Texture, const FBox2f* UV = nullptr)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(Texture);
		if (Texture != nullptr)
		{
			Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
		}
		if (UV != nullptr)
		{
			Brush.SetUVRegion(*UV);
		}
		return Brush;
	}

	void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color,
		ETextJustify::Type Justification = ETextJustify::Left)
	{
		if (Text == nullptr)
		{
			return;
		}
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.OutlineSettings.OutlineSize = Size >= 27 ? 1 : 0;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
		Text->SetJustification(Justification);
		Text->SetAutoWrapText(false);
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget,
		const FVector2D Position, const FVector2D Size, int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
		Slot->SetAnchors(FAnchors(0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetAutoSize(false);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
		return Slot;
	}

	FText ChoiceFallback(const FRewardChoiceUI& Choice)
	{
		if (Choice.mName.IsEmpty() == false)
		{
			return Choice.mName;
		}
		switch (Choice.mKind)
		{
		case ERewardChoiceKind::Skill: return LOCTEXT("SkillReward", "스킬");
		case ERewardChoiceKind::Equipment: return LOCTEXT("EquipmentReward", "아티팩트");
		case ERewardChoiceKind::Gold: return LOCTEXT("GoldReward", "골드");
		default: return FText::GetEmpty();
		}
	}
}

URewardSettlementWidgetBase::URewardSettlementWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);

#define RD_SETTLEMENT_TEX(Member, Path) Member = LoadObject<UTexture2D>(nullptr, TEXT(Path))
	RD_SETTLEMENT_TEX(mMercenaryRowTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/RewardSettlement/T_RS_MercenaryRow.T_RS_MercenaryRow");
	RD_SETTLEMENT_TEX(mPortraitFrameTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/RewardSettlement/T_RS_PortraitFrame.T_RS_PortraitFrame");
	RD_SETTLEMENT_TEX(mXPBadgeTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/RewardSettlement/T_RS_XPBadge.T_RS_XPBadge");
	RD_SETTLEMENT_TEX(mExpTrackTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/RewardSettlement/T_RS_ExpTrack.T_RS_ExpTrack");
	RD_SETTLEMENT_TEX(mExpFillTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/RewardSettlement/T_RS_ExpFill.T_RS_ExpFill");
	RD_SETTLEMENT_TEX(mGoldIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_gold_icon.T_reward_v4_gold_icon");
	RD_SETTLEMENT_TEX(mExpIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_exp_icon.T_reward_v4_exp_icon");
	RD_SETTLEMENT_TEX(mEquipmentIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Equipment/T_equip_weapon_common.T_equip_weapon_common");
	RD_SETTLEMENT_TEX(mSkillIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_CombatHUD_SkillIcon_Basic.T_CombatHUD_SkillIcon_Basic");
#undef RD_SETTLEMENT_TEX
}

void URewardSettlementWidgetBase::OpenUI(FOnEndUIOpenAnimation Callback)
{
	mContinueCommitted = false;
	StopAllAnimations();
	Super::OpenUI(MoveTemp(Callback));
	StopAllAnimations();
	FinishOpenUI();
}

void URewardSettlementWidgetBase::CloseUI(FOnEndUICloseAnimation Callback)
{
	StopAllAnimations();
	Super::CloseUI(MoveTemp(Callback));
	StopAllAnimations();
	FinishCloseUI();
}

void URewardSettlementWidgetBase::PlayOpenUIAnimation_Implementation()
{
	FinishOpenUI();
}

void URewardSettlementWidgetBase::PlayCloseUIAnimation_Implementation()
{
	FinishCloseUI();
}

void URewardSettlementWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	if (mNextButton != nullptr)
	{
		mNextButton->OnClicked.AddUniqueDynamic(this, &URewardSettlementWidgetBase::HandleNextClicked);
		mNextButton->SetVisibility(ESlateVisibility::Visible);
	}
	RefreshView();
}

void URewardSettlementWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}

void URewardSettlementWidgetBase::BindUIModel(URewardUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		RefreshView();
		return;
	}
	UnbindUIModel();
	mUIModel = InUIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &URewardSettlementWidgetBase::HandleUIChanged);
		mUIModel->OnChoicesChanged.AddDynamic(this, &URewardSettlementWidgetBase::HandleChoicesChanged);
	}
	RefreshView();
}

void URewardSettlementWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &URewardSettlementWidgetBase::HandleUIChanged);
		mUIModel->OnChoicesChanged.RemoveDynamic(this, &URewardSettlementWidgetBase::HandleChoicesChanged);
	}
	mUIModel = nullptr;
}

void URewardSettlementWidgetBase::HandleUIChanged()
{
	RefreshView();
}

void URewardSettlementWidgetBase::HandleChoicesChanged()
{
	RefreshView();
}

void URewardSettlementWidgetBase::HandleNextClicked()
{
	ContinueToNext();
}

void URewardSettlementWidgetBase::ContinueToNext()
{
	if (mContinueCommitted)
	{
		return;
	}
	mContinueCommitted = true;

	if (mUIModel != nullptr)
	{
		const FRewardUI Reward = mUIModel->GetReward();
		const TArray<FRewardChoiceUI> Choices = mUIModel->GetRewardChoices();
		if (Reward.mGoldGained > 0)
		{
			mUIModel->RequestClaimReward(ERewardClaimKind::Gold);
		}
		if (Reward.mExpGained > 0)
		{
			mUIModel->RequestClaimReward(ERewardClaimKind::Exp);
		}
		for (const FRewardChoiceUI& Choice : Choices)
		{
			mUIModel->RequestClaimReward(ERewardClaimKind::Choice, Choice.mChoiceIndex);
		}
		mUIModel->RequestClaim();
	}

	OnClosed.Broadcast();
	CloseUI();
}

void URewardSettlementWidgetBase::RefreshView()
{
	if (mTitleText != nullptr)
	{
		mTitleText->SetText(mUIModel != nullptr && mUIModel->GetReward().mTitle.IsEmpty() == false
			? mUIModel->GetReward().mTitle
			: LOCTEXT("DefaultTitle", "전투 보상"));
	}
	if (mNextButtonText != nullptr)
	{
		mNextButtonText->SetText(LOCTEXT("Next", "다음"));
	}
	if (mGoldBalanceText != nullptr)
	{
		const int32 Gold = mUIModel != nullptr ? mUIModel->GetReward().mGoldBalance : 0;
		mGoldBalanceText->SetText(FText::Format(LOCTEXT("GoldBalance", "보유 골드 {0}"), FText::AsNumber(Gold)));
	}
	RebuildSummaryRows();
	RebuildMercenaryRows();
}

void URewardSettlementWidgetBase::RebuildSummaryRows()
{
	if (mSummaryRowsBox == nullptr)
	{
		return;
	}
	mSummaryRowsBox->ClearChildren();
	if (mUIModel == nullptr)
	{
		return;
	}

	const FRewardUI& Reward = mUIModel->GetReward();
	if (Reward.mGoldGained > 0)
	{
		AddSummaryRow(
			FText::Format(LOCTEXT("GoldGain", "골드 +{0}"), FText::AsNumber(Reward.mGoldGained)),
			FText::Format(LOCTEXT("GoldAfter", "보유 골드 {0}"), FText::AsNumber(Reward.mGoldBalance)),
			mGoldIconTexture);
	}
	if (Reward.mExpGained > 0)
	{
		AddSummaryRow(
			FText::Format(LOCTEXT("ExpGain", "경험치 +{0}"), FText::AsNumber(Reward.mExpGained)),
			FText::Format(LOCTEXT("ExpTargets", "용병 {0}명 정산"), FText::AsNumber(Reward.mMercenaryExp.Num())),
			mExpIconTexture);
	}

	for (const FRewardChoiceUI& Choice : mUIModel->GetRewardChoices())
	{
		UTexture2D* Fallback = Choice.mKind == ERewardChoiceKind::Skill
			? mSkillIconTexture.Get() : mEquipmentIconTexture.Get();
		AddSummaryRow(ChoiceFallback(Choice), Choice.mDescription,
			Choice.mIcon != nullptr ? Choice.mIcon.Get() : Fallback);
	}
}

void URewardSettlementWidgetBase::AddSummaryRow(
	const FText& MainText, const FText& SubText, UTexture2D* Icon)
{
	const FString Suffix = FString::Printf(TEXT("%d_%d"), mDynamicBuildGeneration,
		mSummaryRowsBox->GetChildrenCount());
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
		*FString::Printf(TEXT("SettlementSummarySize_%s"), *Suffix));
	Size->SetWidthOverride(320.0f);
	Size->SetHeightOverride(88.0f);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),
		*FString::Printf(TEXT("SettlementSummaryOverlay_%s"), *Suffix));
	Size->SetContent(Overlay);

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),
		*FString::Printf(TEXT("SettlementSummaryBackground_%s"), *Suffix));
	Background->SetBrushColor(FLinearColor(0.05f, 0.025f, 0.012f, 0.72f));
	Background->SetPadding(FMargin(0.0f));
	Overlay->AddChildToOverlay(Background);

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),
		*FString::Printf(TEXT("SettlementSummaryCanvas_%s"), *Suffix));
	Overlay->AddChildToOverlay(Canvas);

	UImage* IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
		*FString::Printf(TEXT("SettlementSummaryIcon_%s"), *Suffix));
	IconImage->SetBrush(TextureBrush(Icon));
	IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Place(Canvas, IconImage, FVector2D(13.0f, 12.0f), FVector2D(64.0f, 64.0f), 1);

	UTextBlock* Main = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
		*FString::Printf(TEXT("SettlementSummaryMain_%s"), *Suffix));
	Main->SetText(MainText);
	StyleText(Main, 25, Cream);
	Place(Canvas, Main, FVector2D(88.0f, 10.0f), FVector2D(218.0f, 38.0f), 2);

	UTextBlock* Sub = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
		*FString::Printf(TEXT("SettlementSummarySub_%s"), *Suffix));
	Sub->SetText(SubText);
	StyleText(Sub, 16, FLinearColor(0.78f, 0.72f, 0.62f, 1.0f));
	Place(Canvas, Sub, FVector2D(88.0f, 48.0f), FVector2D(218.0f, 28.0f), 2);

	if (UVerticalBoxSlot* RowSlot = mSummaryRowsBox->AddChildToVerticalBox(Size))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 9.0f));
	}
}

void URewardSettlementWidgetBase::RebuildMercenaryRows()
{
	if (mMercenaryRowsBox == nullptr)
	{
		return;
	}
	mMercenaryRowsBox->ClearChildren();
	++mDynamicBuildGeneration;
	if (mUIModel == nullptr)
	{
		return;
	}

	const FRewardUI& Reward = mUIModel->GetReward();
	for (int32 Index = 0; Index < Reward.mMercenaryExp.Num(); ++Index)
	{
		AddMercenaryRow(Index, Reward.mMercenaryExp[Index], Reward.mExpGained);
	}
}

void URewardSettlementWidgetBase::AddMercenaryRow(
	int32 RowIndex, const FRewardMercenaryExpUI& Mercenary, int32 ExpGained)
{
	const FString Suffix = FString::Printf(TEXT("%d_%d"), mDynamicBuildGeneration, RowIndex);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
		*FString::Printf(TEXT("SettlementMercenarySize_%s"), *Suffix));
	Size->SetWidthOverride(1050.0f);
	Size->SetHeightOverride(128.0f);
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),
		*FString::Printf(TEXT("SettlementMercenaryCanvas_%s"), *Suffix));
	Size->SetContent(Canvas);

	const FBox2f RowUV(FVector2f(42.0f / 1923.0f, 198.0f / 817.0f),
		FVector2f(1880.0f / 1923.0f, 619.0f / 817.0f));
	UImage* RowBackground = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
		*FString::Printf(TEXT("SettlementMercenaryBackground_%s"), *Suffix));
	RowBackground->SetBrush(TextureBrush(mMercenaryRowTexture, &RowUV));
	RowBackground->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Place(Canvas, RowBackground, FVector2D::ZeroVector, FVector2D(1050.0f, 128.0f), 0);

	const FBox2f FrameUV(FVector2f(148.0f / 1254.0f, 133.0f / 1254.0f),
		FVector2f(1106.0f / 1254.0f, 1110.0f / 1254.0f));
	UImage* PortraitFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
		*FString::Printf(TEXT("SettlementPortraitPlate_%s"), *Suffix));
	PortraitFrame->SetBrush(TextureBrush(mPortraitFrameTexture, &FrameUV));
	PortraitFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Place(Canvas, PortraitFrame, FVector2D(13.0f, 9.0f), FVector2D(110.0f, 110.0f), 1);

	UImage* Portrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
		*FString::Printf(TEXT("SettlementPortrait_%s"), *Suffix));
	Portrait->SetBrush(TextureBrush(Mercenary.mPortrait.Get()));
	Portrait->SetVisibility(Mercenary.mPortrait != nullptr
		? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	// 프레임 중앙이 불투명한 원본이므로 초상을 프레임보다 위에, 금색 테두리 안쪽에 둔다.
	Place(Canvas, Portrait, FVector2D(25.0f, 21.0f), FVector2D(86.0f, 86.0f), 2);

	UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
		*FString::Printf(TEXT("SettlementMercenaryName_%s"), *Suffix));
	Name->SetText(Mercenary.mName);
	StyleText(Name, 27, Ink);
	Place(Canvas, Name, FVector2D(140.0f, 14.0f), FVector2D(200.0f, 40.0f), 3);

	UTextBlock* Level = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
		*FString::Printf(TEXT("SettlementMercenaryLevel_%s"), *Suffix));
	Level->SetText(FText::Format(LOCTEXT("Level", "Lv.{0}"), FText::AsNumber(Mercenary.mLevel)));
	StyleText(Level, 22, Blue);
	Place(Canvas, Level, FVector2D(140.0f, 57.0f), FVector2D(200.0f, 36.0f), 3);

	UTextBlock* Transition = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
		*FString::Printf(TEXT("SettlementMercenaryTransition_%s"), *Suffix));
	Transition->SetText(FText::Format(LOCTEXT("ExpTransition", "{0}  →  {1}"),
		FText::AsNumber(FMath::RoundToInt(Mercenary.mExpBefore)),
		FText::AsNumber(FMath::RoundToInt(Mercenary.mExpAfter))));
	StyleText(Transition, 24, Ink, ETextJustify::Center);
	Place(Canvas, Transition, FVector2D(350.0f, 12.0f), FVector2D(390.0f, 40.0f), 3);

	UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),
		*FString::Printf(TEXT("SettlementMercenaryBar_%s"), *Suffix));
	const FBox2f TrackUV(FVector2f(36.0f / 1974.0f, 288.0f / 797.0f),
		FVector2f(1937.0f / 1974.0f, 509.0f / 797.0f));
	const FBox2f FillUV(FVector2f(117.0f / 2172.0f, 254.0f / 724.0f),
		FVector2f(2054.0f / 2172.0f, 470.0f / 724.0f));
	FProgressBarStyle BarStyle;
	BarStyle.SetBackgroundImage(TextureBrush(mExpTrackTexture, &TrackUV));
	BarStyle.SetFillImage(TextureBrush(mExpFillTexture, &FillUV));
	BarStyle.SetMarqueeImage(TextureBrush(nullptr));
	Bar->SetWidgetStyle(BarStyle);
	Bar->SetPercent(Mercenary.mMaxExp > 0.0f
		? FMath::Clamp(Mercenary.mExpAfter / Mercenary.mMaxExp, 0.0f, 1.0f) : 0.0f);
	Bar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Place(Canvas, Bar, FVector2D(350.0f, 61.0f), FVector2D(400.0f, 45.0f), 3);

	UTextBlock* BarText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
		*FString::Printf(TEXT("SettlementMercenaryBarText_%s"), *Suffix));
	BarText->SetText(FText::Format(LOCTEXT("ExpBar", "{0} / {1}"),
		FText::AsNumber(FMath::RoundToInt(Mercenary.mExpAfter)),
		FText::AsNumber(FMath::RoundToInt(Mercenary.mMaxExp))));
	StyleText(BarText, 21, FLinearColor::White, ETextJustify::Center);
	Place(Canvas, BarText, FVector2D(350.0f, 66.0f), FVector2D(400.0f, 34.0f), 4);

	const FBox2f BadgeUV(FVector2f(158.0f / 1649.0f, 197.0f / 954.0f),
		FVector2f(1488.0f / 1649.0f, 733.0f / 954.0f));
	UImage* Badge = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
		*FString::Printf(TEXT("SettlementXPBadge_%s"), *Suffix));
	Badge->SetBrush(TextureBrush(mXPBadgeTexture, &BadgeUV));
	Badge->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Place(Canvas, Badge, FVector2D(820.0f, 28.0f), FVector2D(190.0f, 72.0f), 3);

	UTextBlock* XP = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
		*FString::Printf(TEXT("SettlementXPText_%s"), *Suffix));
	XP->SetText(FText::Format(LOCTEXT("XPBadge", "+{0} XP"), FText::AsNumber(ExpGained)));
	StyleText(XP, 24, FLinearColor::White, ETextJustify::Center);
	Place(Canvas, XP, FVector2D(820.0f, 41.0f), FVector2D(190.0f, 42.0f), 4);

	if (UVerticalBoxSlot* RowSlot = mMercenaryRowsBox->AddChildToVerticalBox(Size))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}
}

#undef LOCTEXT_NAMESPACE
