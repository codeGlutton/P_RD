#include "UI/SkillPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "UI/PanelNavigationStyle.h"
#include "UI/SkillPanelWidgetPrivate.h"
#include "UI/UIRuntimeLayout.h"

namespace
{
	FText GetSkillPanelLabel(int32 SkillIndex)
	{
		switch (SkillIndex)
		{
		case 0:
			return NSLOCTEXT("SkillPanelWidget", "SkillPanelBasicAttack", "BASIC");
		case 1:
			return NSLOCTEXT("SkillPanelWidget", "SkillPanelSkill1", "SKILL 1");
		case 2:
			return NSLOCTEXT("SkillPanelWidget", "SkillPanelSkill2", "SKILL 2");
		case 3:
			return NSLOCTEXT("SkillPanelWidget", "SkillPanelSkill3", "SKILL 3");
		case 4:
			return NSLOCTEXT("SkillPanelWidget", "SkillPanelSkill4", "SKILL 4");
		case 5:
			return NSLOCTEXT("SkillPanelWidget", "SkillPanelStep", "STEP");
		default:
			return NSLOCTEXT("SkillPanelWidget", "SkillPanelUnknown", "SKILL");
		}
	}

	UTextBlock* BuildButtonText(UWidgetTree* WidgetTree, const TCHAR* Name, const FText& Text)
	{
		if (WidgetTree == nullptr)
		{
			return nullptr;
		}

		UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		if (ButtonText != nullptr)
		{
			ButtonText->SetJustification(ETextJustify::Center);
			ButtonText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetButtonTextColor()));
			ButtonText->SetText(Text);
		}
		return ButtonText;
	}
}

void USkillPanelWidget::EnsureSkillDetailWidgets()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	if (mRootCanvas == nullptr)
	{
		mRootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}
	if (mRootCanvas == nullptr)
	{
		return;
	}

	if (mSkillDetailTitleText == nullptr)
	{
		mSkillDetailTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillDetailTitleText"));
		if (mSkillDetailTitleText != nullptr)
		{
			mSkillDetailTitleText->SetJustification(ETextJustify::Center);
			mSkillDetailTitleText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetPanelTextColor()));
			mRootCanvas->AddChildToCanvas(mSkillDetailTitleText);
		}
	}

	if (mSkillDetailBodyText == nullptr)
	{
		mSkillDetailBodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillDetailBodyText"));
		if (mSkillDetailBodyText != nullptr)
		{
			mSkillDetailBodyText->SetJustification(ETextJustify::Center);
			mSkillDetailBodyText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetPanelTextColor()));
			mRootCanvas->AddChildToCanvas(mSkillDetailBodyText);
		}
	}

	if (mSkillDetailPreviousButton == nullptr)
	{
		mSkillDetailPreviousButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SkillDetailPreviousButton"));
		if (mSkillDetailPreviousButton != nullptr)
		{
			mSkillDetailPreviousButton->AddChild(BuildButtonText(WidgetTree, TEXT("SkillDetailPreviousButtonText"), NSLOCTEXT("SkillPanelWidget", "SkillDetailPreviousText", "<")));
			mSkillDetailPreviousButton->SetBackgroundColor(RDPanelNavigationStyle::GetNavigationButtonColor());
			mSkillDetailPreviousButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillDetailPreviousButtonClicked);
			mRootCanvas->AddChildToCanvas(mSkillDetailPreviousButton);
		}
	}

	if (mSkillDetailNextButton == nullptr)
	{
		mSkillDetailNextButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SkillDetailNextButton"));
		if (mSkillDetailNextButton != nullptr)
		{
			mSkillDetailNextButton->AddChild(BuildButtonText(WidgetTree, TEXT("SkillDetailNextButtonText"), NSLOCTEXT("SkillPanelWidget", "SkillDetailNextText", ">")));
			mSkillDetailNextButton->SetBackgroundColor(RDPanelNavigationStyle::GetNavigationButtonColor());
			mSkillDetailNextButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillDetailNextButtonClicked);
			mRootCanvas->AddChildToCanvas(mSkillDetailNextButton);
		}
	}

	if (mSkillDetailBackButton == nullptr)
	{
		mSkillDetailBackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SkillDetailBackButton"));
		if (mSkillDetailBackButton != nullptr)
		{
			mSkillDetailBackButton->AddChild(BuildButtonText(WidgetTree, TEXT("SkillDetailBackButtonText"), NSLOCTEXT("SkillPanelWidget", "SkillDetailBackText", "BACK")));
			mSkillDetailBackButton->SetBackgroundColor(RDPanelNavigationStyle::GetBackButtonColor());
			mSkillDetailBackButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillDetailBackButtonClicked);
			mRootCanvas->AddChildToCanvas(mSkillDetailBackButton);
		}
	}

	ApplySkillDetailLayout();
	RefreshSkillDetailWidgets();
}

void USkillPanelWidget::ApplySkillDetailLayout() const
{
	RDUILayout::ApplyAnchoredSlot(mSkillDetailTitleText, FAnchors(0.340f, 0.215f, 0.660f, 0.285f), 210);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailBodyText, FAnchors(0.315f, 0.320f, 0.685f, 0.620f), 210);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailPreviousButton, FAnchors(0.065f, 0.405f, 0.125f, 0.520f), 211);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailNextButton, FAnchors(0.875f, 0.405f, 0.935f, 0.520f), 211);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailBackButton, FAnchors(0.405f, 0.705f, 0.595f, 0.785f), 211);
}

void USkillPanelWidget::ShowSkillDetail(int32 SkillIndex)
{
	if (SkillIndex < 0 || SkillIndex >= RDSkillPanel::ItemCount)
	{
		return;
	}

	mShowingSkillDetail = true;
	mDetailSkillIndex = SkillIndex;
	EnsureSkillDetailWidgets();
	RefreshSkillDetailWidgets();
	RefreshSkillSelectButtons();
}

void USkillPanelWidget::ShowSkillList()
{
	mShowingSkillDetail = false;
	mDetailSkillIndex = INDEX_NONE;
	mTrackingSkillDetailSwipe = false;
	mSkillDetailSwipeStartPosition = FVector2D::ZeroVector;

	Super::ApplyOpenUI();
	EnsureSkillSelectButtons();
	EnsureSkillDetailWidgets();

	RefreshSkillDetailWidgets();
	RefreshSkillSelectButtons();
}

void USkillPanelWidget::RefreshSkillDetailWidgets()
{
	const ESlateVisibility DetailVisibility = mShowingSkillDetail ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (mSkillDetailTitleText != nullptr)
	{
		mSkillDetailTitleText->SetVisibility(DetailVisibility);
	}
	if (mSkillDetailBodyText != nullptr)
	{
		mSkillDetailBodyText->SetVisibility(DetailVisibility);
	}
	if (mSkillDetailPreviousButton != nullptr)
	{
		mSkillDetailPreviousButton->SetVisibility(DetailVisibility);
	}
	if (mSkillDetailNextButton != nullptr)
	{
		mSkillDetailNextButton->SetVisibility(DetailVisibility);
	}
	if (mSkillDetailBackButton != nullptr)
	{
		mSkillDetailBackButton->SetVisibility(DetailVisibility);
	}

	if (mShowingSkillDetail == false || mDetailSkillIndex == INDEX_NONE)
	{
		return;
	}

	if (mSkillDetailTitleText != nullptr)
	{
		mSkillDetailTitleText->SetText(FText::Format(
			NSLOCTEXT("SkillPanelWidget", "SkillDetailTitleFormat", "{0} / {1}"),
			FText::AsNumber(mDetailSkillIndex + 1),
			FText::AsNumber(RDSkillPanel::ItemCount)
		));
	}
	if (mSkillDetailBodyText != nullptr)
	{
		mSkillDetailBodyText->SetText(FText::Format(
			NSLOCTEXT("SkillPanelWidget", "SkillDetailBodyFormat", "{0}\nSkill detail preview\nAPI connection pending"),
			GetSkillPanelLabel(mDetailSkillIndex)
		));
	}
}

void USkillPanelWidget::MoveSkillDetail(int32 Direction)
{
	if (Direction == 0)
	{
		return;
	}

	const int32 CurrentIndex = mDetailSkillIndex != INDEX_NONE ? mDetailSkillIndex : GetSelectedCarouselIndex();
	const int32 NextIndex = (CurrentIndex + Direction + RDSkillPanel::ItemCount) % RDSkillPanel::ItemCount;
	SelectCarouselItem(NextIndex);
}

void USkillPanelWidget::HandleSkillDetailBackButtonClicked()
{
	ShowSkillList();
}

void USkillPanelWidget::HandleSkillDetailPreviousButtonClicked()
{
	MoveSkillDetail(-1);
}

void USkillPanelWidget::HandleSkillDetailNextButtonClicked()
{
	MoveSkillDetail(1);
}
