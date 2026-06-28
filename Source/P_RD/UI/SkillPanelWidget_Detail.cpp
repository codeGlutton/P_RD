#include "UI/SkillPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/PanelNavigationStyle.h"
#include "UI/SkillPanelWidgetPrivate.h"
#include "UI/UIRuntimeLayout.h"
#include "UI/PanelRootWidgetUtils.h"

namespace
{
	using RDPanelWidgetUtils::FindOrCreateRootWidget;

	/** @brief 임시 스킬 index를 표시 라벨로 변환한다. 실제 스킬 데이터 연결 전까지만 쓰는 fixture다. */
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

	/** @brief 버튼 안의 텍스트를 WBP에서 찾거나 fallback으로 만든다. */
	UTextBlock* FindOrCreateSkillButtonText(UWidgetTree* WidgetTree, UButton* Button, FName Name, const FText& Text)
	{
		if (WidgetTree == nullptr || Button == nullptr)
		{
			return nullptr;
		}

		UTextBlock* ButtonText = Cast<UTextBlock>(WidgetTree->FindWidget(Name));
		if (ButtonText == nullptr)
		{
			ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		}
		if (ButtonText != nullptr)
		{
			ButtonText->SetJustification(ETextJustify::Center);
			ButtonText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetButtonTextColor()));
			ButtonText->SetText(Text);
			if (ButtonText->GetParent() == nullptr)
			{
				Button->AddChild(ButtonText);
			}
		}
		return ButtonText;
	}
}

/** @brief 상세 보기 텍스트와 좌우/뒤로가기 버튼을 런타임으로 만든다. */
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
		mSkillDetailTitleText = FindOrCreateRootWidget<UTextBlock>(WidgetTree, mRootCanvas, TEXT("SkillDetailTitleText"));
		if (mSkillDetailTitleText != nullptr)
		{
			mSkillDetailTitleText->SetJustification(ETextJustify::Center);
			mSkillDetailTitleText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetPanelTextColor()));
		}
	}

	if (mSkillDetailBodyText == nullptr)
	{
		mSkillDetailBodyText = FindOrCreateRootWidget<UTextBlock>(WidgetTree, mRootCanvas, TEXT("SkillDetailBodyText"));
		if (mSkillDetailBodyText != nullptr)
		{
			mSkillDetailBodyText->SetJustification(ETextJustify::Center);
			mSkillDetailBodyText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetPanelTextColor()));
		}
	}

	if (mSkillDetailPreviousButton == nullptr)
	{
		mSkillDetailPreviousButton = FindOrCreateRootWidget<UButton>(WidgetTree, mRootCanvas, TEXT("SkillDetailPreviousButton"));
		if (mSkillDetailPreviousButton != nullptr)
		{
			FindOrCreateSkillButtonText(WidgetTree, mSkillDetailPreviousButton, TEXT("SkillDetailPreviousButtonText"), NSLOCTEXT("SkillPanelWidget", "SkillDetailPreviousText", "<"));
			mSkillDetailPreviousButton->SetBackgroundColor(RDPanelNavigationStyle::GetNavigationButtonColor());
			mSkillDetailPreviousButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillDetailPreviousButtonClicked);
		}
	}

	if (mSkillDetailNextButton == nullptr)
	{
		mSkillDetailNextButton = FindOrCreateRootWidget<UButton>(WidgetTree, mRootCanvas, TEXT("SkillDetailNextButton"));
		if (mSkillDetailNextButton != nullptr)
		{
			FindOrCreateSkillButtonText(WidgetTree, mSkillDetailNextButton, TEXT("SkillDetailNextButtonText"), NSLOCTEXT("SkillPanelWidget", "SkillDetailNextText", ">"));
			mSkillDetailNextButton->SetBackgroundColor(RDPanelNavigationStyle::GetNavigationButtonColor());
			mSkillDetailNextButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillDetailNextButtonClicked);
		}
	}

	if (mSkillDetailBackButton == nullptr)
	{
		mSkillDetailBackButton = FindOrCreateRootWidget<UButton>(WidgetTree, mRootCanvas, TEXT("SkillDetailBackButton"));
		if (mSkillDetailBackButton != nullptr)
		{
			FindOrCreateSkillButtonText(WidgetTree, mSkillDetailBackButton, TEXT("SkillDetailBackButtonText"), NSLOCTEXT("SkillPanelWidget", "SkillDetailBackText", "BACK"));
			mSkillDetailBackButton->SetBackgroundColor(RDPanelNavigationStyle::GetBackButtonColor());
			mSkillDetailBackButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillDetailBackButtonClicked);
		}
	}

	ApplySkillDetailLayout();
	RefreshSkillDetailWidgets();
}

/** @brief 상세 보기 런타임 위젯을 현재 WBP 목업 위치에 맞는 anchor로 배치한다. */
void USkillPanelWidget::ApplySkillDetailLayout() const
{
	// [합의필요] Anchor 값은 현재 WBP_SkillPanel 목업 비율에 맞춘 임시 정책이다.
	RDUILayout::ApplyAnchoredSlot(mSkillDetailTitleText, FAnchors(0.340f, 0.215f, 0.660f, 0.285f), 210);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailBodyText, FAnchors(0.315f, 0.320f, 0.685f, 0.620f), 210);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailPreviousButton, FAnchors(0.065f, 0.405f, 0.125f, 0.520f), 211);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailNextButton, FAnchors(0.875f, 0.405f, 0.935f, 0.520f), 211);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailBackButton, FAnchors(0.405f, 0.705f, 0.595f, 0.785f), 211);
}

/** @brief 지정 스킬 index를 상세 모드로 열고 목록 투명 버튼을 비활성 표시로 돌린다. */
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

/** @brief 상세 모드를 닫고 부모 carousel 목록 배치를 다시 연다. */
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

/** @brief 상세 모드 표시 상태와 mock 텍스트 내용을 현재 detail index로 갱신한다. */
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

/** @brief 상세 모드에서 이전/다음 스킬로 순환 이동하고 부모 carousel 선택도 동기화한다. */
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
