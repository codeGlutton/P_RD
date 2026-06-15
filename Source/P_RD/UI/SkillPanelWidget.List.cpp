#include "UI/SkillPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "UI/SkillPanelWidgetPrivate.h"
#include "UI/UIRuntimeLayout.h"

namespace
{
	const FVector2D SkillSelectButtonBaseSize(180.0f, 244.0f);
	const FVector2D SkillSelectButtonLinearOrigin(0.0f, 36.0f);
	constexpr float SkillSelectButtonLinearMaxWidth = 920.0f;
	constexpr float SkillSelectButtonLinearPreferredSpacing = 168.0f;
}

void USkillPanelWidget::EnsureSkillSelectButtons()
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

	if (mSkillSelectButtons.Num() != RDSkillPanel::ItemCount)
	{
		mSkillSelectButtons.SetNum(RDSkillPanel::ItemCount);
	}

	for (int32 Index = 0; Index < RDSkillPanel::ItemCount; ++Index)
	{
		if (mSkillSelectButtons[Index] != nullptr)
		{
			continue;
		}

		UButton* SelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*FString::Printf(TEXT("SkillSelectButton_%d"), Index)));
		if (SelectButton == nullptr)
		{
			continue;
		}

		/*
		 * WBP_SkillPanel은 CarouselItem_N 카드만 있고 CarouselButton_N은 없다.
		 * 모바일에서는 카드 안쪽 자식 위젯이 터치를 먼저 가져가면 부모의 카드 전체 터치 판정까지 도달하지 않을 수 있다.
		 * 그래서 카드와 같은 위치에 투명 버튼을 얹어, 목록 상태의 "스킬 카드 선택" 입력을 확실하게 받는다.
		 */
		SelectButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
		SelectButton->SetRenderOpacity(0.01f);
		BindSkillSelectButton(SelectButton, Index);
		mRootCanvas->AddChildToCanvas(SelectButton);
		mSkillSelectButtons[Index] = SelectButton;
	}

	ApplySkillSelectButtonLayout();
	RefreshSkillSelectButtons();
}

void USkillPanelWidget::ApplySkillSelectButtonLayout() const
{
	const float Spacing = RDSkillPanel::ItemCount > 1
		? FMath::Min(SkillSelectButtonLinearPreferredSpacing, SkillSelectButtonLinearMaxWidth / StaticCast<float>(RDSkillPanel::ItemCount - 1))
		: 0.0f;
	const float StartX = -Spacing * StaticCast<float>(RDSkillPanel::ItemCount - 1) * 0.5f;

	for (int32 Index = 0; Index < RDSkillPanel::ItemCount; ++Index)
	{
		if (mSkillSelectButtons.IsValidIndex(Index) == false)
		{
			continue;
		}

		RDUILayout::ApplyCenteredSlot(
			mSkillSelectButtons[Index],
			SkillSelectButtonLinearOrigin + FVector2D(StartX + Spacing * StaticCast<float>(Index), 0.0f),
			SkillSelectButtonBaseSize,
			206
		);
	}
}

void USkillPanelWidget::RefreshSkillSelectButtons()
{
	const ESlateVisibility SelectButtonVisibility = mShowingSkillDetail ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
	for (TObjectPtr<UButton>& SelectButton : mSkillSelectButtons)
	{
		if (SelectButton != nullptr)
		{
			SelectButton->SetVisibility(SelectButtonVisibility);
		}
	}
}

void USkillPanelWidget::SelectSkillFromList(int32 SkillIndex)
{
	if (SkillIndex < 0 || SkillIndex >= RDSkillPanel::ItemCount)
	{
		return;
	}

	SelectCarouselItem(SkillIndex);
}

void USkillPanelWidget::BindSkillSelectButton(UButton* SelectButton, int32 SkillIndex)
{
	if (SelectButton == nullptr)
	{
		return;
	}

	switch (SkillIndex)
	{
	case 0:
		SelectButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillSelectButton0Clicked);
		break;
	case 1:
		SelectButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillSelectButton1Clicked);
		break;
	case 2:
		SelectButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillSelectButton2Clicked);
		break;
	case 3:
		SelectButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillSelectButton3Clicked);
		break;
	case 4:
		SelectButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillSelectButton4Clicked);
		break;
	case 5:
		SelectButton->OnClicked.AddUniqueDynamic(this, &USkillPanelWidget::HandleSkillSelectButton5Clicked);
		break;
	default:
		break;
	}
}

void USkillPanelWidget::UnbindSkillSelectButton(UButton* SelectButton, int32 SkillIndex)
{
	if (SelectButton == nullptr)
	{
		return;
	}

	switch (SkillIndex)
	{
	case 0:
		SelectButton->OnClicked.RemoveDynamic(this, &USkillPanelWidget::HandleSkillSelectButton0Clicked);
		break;
	case 1:
		SelectButton->OnClicked.RemoveDynamic(this, &USkillPanelWidget::HandleSkillSelectButton1Clicked);
		break;
	case 2:
		SelectButton->OnClicked.RemoveDynamic(this, &USkillPanelWidget::HandleSkillSelectButton2Clicked);
		break;
	case 3:
		SelectButton->OnClicked.RemoveDynamic(this, &USkillPanelWidget::HandleSkillSelectButton3Clicked);
		break;
	case 4:
		SelectButton->OnClicked.RemoveDynamic(this, &USkillPanelWidget::HandleSkillSelectButton4Clicked);
		break;
	case 5:
		SelectButton->OnClicked.RemoveDynamic(this, &USkillPanelWidget::HandleSkillSelectButton5Clicked);
		break;
	default:
		break;
	}
}

void USkillPanelWidget::UnbindSkillSelectButtons()
{
	for (int32 SkillIndex = 0; SkillIndex < mSkillSelectButtons.Num(); ++SkillIndex)
	{
		UnbindSkillSelectButton(mSkillSelectButtons[SkillIndex], SkillIndex);
	}
}

void USkillPanelWidget::HandleSkillSelectButtonClicked(int32 SkillIndex)
{
	SelectSkillFromList(SkillIndex);
}

void USkillPanelWidget::HandleSkillSelectButton0Clicked()
{
	HandleSkillSelectButtonClicked(0);
}

void USkillPanelWidget::HandleSkillSelectButton1Clicked()
{
	HandleSkillSelectButtonClicked(1);
}

void USkillPanelWidget::HandleSkillSelectButton2Clicked()
{
	HandleSkillSelectButtonClicked(2);
}

void USkillPanelWidget::HandleSkillSelectButton3Clicked()
{
	HandleSkillSelectButtonClicked(3);
}

void USkillPanelWidget::HandleSkillSelectButton4Clicked()
{
	HandleSkillSelectButtonClicked(4);
}

void USkillPanelWidget::HandleSkillSelectButton5Clicked()
{
	HandleSkillSelectButtonClicked(5);
}
