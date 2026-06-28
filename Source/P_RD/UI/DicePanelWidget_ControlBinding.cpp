#include "UI/DicePanelWidget.h"

#include "Components/Button.h"
#include "UI/IndexedButtonWidget.h"

void UDicePanelWidget::BindDiceCarouselFaceButton(UIndexedButtonWidget* FaceButton, int32 FaceValue)
{
	if (FaceButton == nullptr)
	{
		return;
	}

	FaceButton->SetButtonIndex(FaceValue);
	FaceButton->OnIndexedClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceIndexedButtonClicked);
}

void UDicePanelWidget::UnbindDiceCarouselFaceButton(UIndexedButtonWidget* FaceButton, int32 FaceValue)
{
	if (FaceButton == nullptr)
	{
		return;
	}

	FaceButton->OnIndexedClicked.RemoveDynamic(this, &UDicePanelWidget::HandleDiceCarouselFaceIndexedButtonClicked);
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

void UDicePanelWidget::HandleDiceCarouselFaceIndexedButtonClicked(int32 FaceValue)
{
	HandleDiceCarouselFaceButtonClicked(FaceValue);
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
