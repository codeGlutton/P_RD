#include "UI/DicePanelWidget.h"

#include "Components/Button.h"

void UDicePanelWidget::BindDiceCarouselFaceButton(UButton* FaceButton, int32 FaceValue)
{
	if (FaceButton == nullptr)
	{
		return;
	}

	switch (FaceValue)
	{
	case 1:
		FaceButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceOneButtonClicked);
		break;
	case 2:
		FaceButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceTwoButtonClicked);
		break;
	case 3:
		FaceButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceThreeButtonClicked);
		break;
	case 4:
		FaceButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceFourButtonClicked);
		break;
	case 5:
		FaceButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceFiveButtonClicked);
		break;
	case 6:
		FaceButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceSixButtonClicked);
		break;
	default:
		break;
	}
}

void UDicePanelWidget::UnbindDiceCarouselFaceButton(UButton* FaceButton, int32 FaceValue)
{
	if (FaceButton == nullptr)
	{
		return;
	}

	switch (FaceValue)
	{
	case 1:
		FaceButton->OnClicked.RemoveDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceOneButtonClicked);
		break;
	case 2:
		FaceButton->OnClicked.RemoveDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceTwoButtonClicked);
		break;
	case 3:
		FaceButton->OnClicked.RemoveDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceThreeButtonClicked);
		break;
	case 4:
		FaceButton->OnClicked.RemoveDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceFourButtonClicked);
		break;
	case 5:
		FaceButton->OnClicked.RemoveDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceFiveButtonClicked);
		break;
	case 6:
		FaceButton->OnClicked.RemoveDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceSixButtonClicked);
		break;
	default:
		break;
	}
}

void UDicePanelWidget::UnbindDiceCarouselFaceButtons()
{
	for (int32 FaceIndex = 0; FaceIndex < mDiceCarouselFaceButtons.Num(); ++FaceIndex)
	{
		UnbindDiceCarouselFaceButton(mDiceCarouselFaceButtons[FaceIndex], FaceIndex + 1);
	}
}

void UDicePanelWidget::HandleDiceCarouselFaceButtonClicked(int32 FaceValue)
{
	StartDicePanelFaceRotation(FaceValue);
}

void UDicePanelWidget::HandleDiceCarouselPreviousButtonClicked()
{
	if (mDicePanelViews.IsEmpty())
	{
		return;
	}

	SelectDicePanelIndex((mSelectedDicePanelIndex - 1 + mDicePanelViews.Num()) % mDicePanelViews.Num());
}

void UDicePanelWidget::HandleDiceCarouselNextButtonClicked()
{
	if (mDicePanelViews.IsEmpty())
	{
		return;
	}

	SelectDicePanelIndex((mSelectedDicePanelIndex + 1) % mDicePanelViews.Num());
}

void UDicePanelWidget::HandleDiceCarouselBackButtonClicked()
{
	mDicePanelCarouselActivated = false;
	mPressedDicePanelIndex = INDEX_NONE;
	mDicePanelPressPosition = FVector2D::ZeroVector;
	mSelectedDicePanelIndex = mDicePanelViews.IsEmpty() ? INDEX_NONE : 0;
	ApplyDicePanelLayout();
	RefreshDicePanelPreviewActors();
	RefreshDiceCarouselControlWidgets();
}

void UDicePanelWidget::HandleDiceCarouselFaceOneButtonClicked()
{
	HandleDiceCarouselFaceButtonClicked(1);
}

void UDicePanelWidget::HandleDiceCarouselFaceTwoButtonClicked()
{
	HandleDiceCarouselFaceButtonClicked(2);
}

void UDicePanelWidget::HandleDiceCarouselFaceThreeButtonClicked()
{
	HandleDiceCarouselFaceButtonClicked(3);
}

void UDicePanelWidget::HandleDiceCarouselFaceFourButtonClicked()
{
	HandleDiceCarouselFaceButtonClicked(4);
}

void UDicePanelWidget::HandleDiceCarouselFaceFiveButtonClicked()
{
	HandleDiceCarouselFaceButtonClicked(5);
}

void UDicePanelWidget::HandleDiceCarouselFaceSixButtonClicked()
{
	HandleDiceCarouselFaceButtonClicked(6);
}
