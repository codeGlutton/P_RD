#include "UI/DicePanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "UI/CombatDiceCaptureActor.h"
#include "UI/DiceCapturePreviewUtils.h"
#include "UI/DicePanelLayoutPolicy.h"

void UDicePanelWidget::ApplyDiceRotation()
{
	const int32 SelectedIndex = mSelectedDicePanelIndex;
	if (mDicePanelPreviewRotations.IsValidIndex(SelectedIndex) == false)
	{
		return;
	}

	if (mDicePanelPreviewActors.IsValidIndex(SelectedIndex) == false)
	{
		return;
	}

	if (ACombatDiceCaptureActor* SelectedPreviewActor = mDicePanelPreviewActors[SelectedIndex])
	{
		SelectedPreviewActor->SetDiceRotation(mDicePanelPreviewRotations[SelectedIndex].Rotator());
		RequestDicePanelPreviewCapture(SelectedIndex);
	}
}

void UDicePanelWidget::StartDicePanelFaceRotation(int32 FaceValue)
{
	const int32 SelectedIndex = mSelectedDicePanelIndex;
	if (mDicePanelViews.IsValidIndex(SelectedIndex) == false)
	{
		return;
	}

	if (mDicePanelPreviewRotations.IsValidIndex(SelectedIndex) == false
		|| mDicePanelTargetPreviewRotations.IsValidIndex(SelectedIndex) == false
		|| mDicePanelPreviewRotationAnimations.IsValidIndex(SelectedIndex) == false
		|| mDicePanelSelectedFaceValues.IsValidIndex(SelectedIndex) == false)
	{
		return;
	}

	const int32 ClampedFaceValue = FMath::Clamp(FaceValue, 1, 6);
	mDicePanelTargetPreviewRotations[SelectedIndex] = ACombatDiceCaptureActor::GetSettledFaceRotation(ClampedFaceValue).Quaternion();
	mDicePanelPreviewRotationAnimations[SelectedIndex] = true;
	mDicePanelSelectedFaceValues[SelectedIndex] = ClampedFaceValue;

	RequestDicePanelPreviewCapture(SelectedIndex);
	RefreshDiceCarouselFaceButtonStyles();
}

void UDicePanelWidget::TickDicePanelFaceRotation(float InDeltaTime)
{
	if (InDeltaTime <= 0.0f)
	{
		return;
	}

	const float StepAlpha = FMath::Clamp(InDeltaTime * DicePanelFaceRotationSpeed, 0.0f, 1.0f);
	for (int32 DiceIndex = 0; DiceIndex < mDicePanelPreviewRotationAnimations.Num(); ++DiceIndex)
	{
		if (mDicePanelPreviewRotationAnimations[DiceIndex] == false
			|| mDicePanelPreviewRotations.IsValidIndex(DiceIndex) == false
			|| mDicePanelTargetPreviewRotations.IsValidIndex(DiceIndex) == false)
		{
			continue;
		}

		FQuat CurrentRotation = mDicePanelPreviewRotations[DiceIndex].GetNormalized();
		const FQuat TargetRotation = mDicePanelTargetPreviewRotations[DiceIndex].GetNormalized();
		CurrentRotation = FQuat::Slerp(CurrentRotation, TargetRotation, StepAlpha).GetNormalized();

		const float RotationDot = FMath::Abs(
			CurrentRotation.X * TargetRotation.X
			+ CurrentRotation.Y * TargetRotation.Y
			+ CurrentRotation.Z * TargetRotation.Z
			+ CurrentRotation.W * TargetRotation.W
		);
		if (RotationDot >= DicePanelFaceRotationSnapDot)
		{
			CurrentRotation = TargetRotation;
			mDicePanelPreviewRotationAnimations[DiceIndex] = false;
		}

		mDicePanelPreviewRotations[DiceIndex] = CurrentRotation;
		CaptureDicePanelPreviewActor(DiceIndex);
	}
}

void UDicePanelWidget::RequestDicePanelPreviewCapture(int32 DiceIndex)
{
	if (mDicePanelViews.IsValidIndex(DiceIndex) == false)
	{
		return;
	}

	mPendingDicePanelCaptureIndex = DiceIndex;
}

void UDicePanelWidget::CaptureDicePanelPreviewActor(int32 DiceIndex)
{
	if (mDicePanelPreviewActors.IsValidIndex(DiceIndex) == false || mDicePanelPreviewRotations.IsValidIndex(DiceIndex) == false)
	{
		return;
	}

	ACombatDiceCaptureActor* DicePreviewActor = mDicePanelPreviewActors[DiceIndex].Get();
	if (DicePreviewActor == nullptr)
	{
		return;
	}

	DicePreviewActor->SetDiceRotation(mDicePanelPreviewRotations[DiceIndex].Rotator());
	DicePreviewActor->CaptureDice();

	if (mDicePanelImages.IsValidIndex(DiceIndex))
	{
		ApplyDicePanelCaptureBrush(mDicePanelImages[DiceIndex], DicePreviewActor, RDDicePanelLayout::GetPreviewBrushSize());
	}
}

void UDicePanelWidget::EnsureDicePanelPreviewWidgets()
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

	if (mDicePanelImages.Num() != DicePanelSlotCount)
	{
		mDicePanelImages.SetNum(DicePanelSlotCount);
	}
	if (mDicePanelCardBorders.Num() != DicePanelSlotCount)
	{
		mDicePanelCardBorders.SetNum(DicePanelSlotCount);
	}
	if (mDicePanelPreviewActors.Num() != DicePanelSlotCount)
	{
		mDicePanelPreviewActors.SetNum(DicePanelSlotCount);
	}
	if (mDicePanelPreviewRotations.Num() != DicePanelSlotCount)
	{
		mDicePanelPreviewRotations.SetNum(DicePanelSlotCount);
		for (int32 DiceIndex = 0; DiceIndex < DicePanelSlotCount; ++DiceIndex)
		{
			mDicePanelPreviewRotations[DiceIndex] = GetDicePanelIdleRotation(DiceIndex).Quaternion();
		}
	}
	if (mDicePanelTargetPreviewRotations.Num() != DicePanelSlotCount)
	{
		mDicePanelTargetPreviewRotations.SetNum(DicePanelSlotCount);
		for (int32 DiceIndex = 0; DiceIndex < DicePanelSlotCount; ++DiceIndex)
		{
			mDicePanelTargetPreviewRotations[DiceIndex] = mDicePanelPreviewRotations.IsValidIndex(DiceIndex)
				? mDicePanelPreviewRotations[DiceIndex]
				: GetDicePanelIdleRotation(DiceIndex).Quaternion();
		}
	}
	if (mDicePanelPreviewRotationAnimations.Num() != DicePanelSlotCount)
	{
		mDicePanelPreviewRotationAnimations.SetNum(DicePanelSlotCount);
		for (int32 DiceIndex = 0; DiceIndex < DicePanelSlotCount; ++DiceIndex)
		{
			mDicePanelPreviewRotationAnimations[DiceIndex] = false;
		}
	}
	if (mDicePanelSelectedFaceValues.Num() != DicePanelSlotCount)
	{
		mDicePanelSelectedFaceValues.SetNum(DicePanelSlotCount);
		for (int32 DiceIndex = 0; DiceIndex < DicePanelSlotCount; ++DiceIndex)
		{
			mDicePanelSelectedFaceValues[DiceIndex] = INDEX_NONE;
		}
	}

	for (int32 DiceIndex = 0; DiceIndex < DicePanelSlotCount; ++DiceIndex)
	{
		if (mDicePanelCardBorders[DiceIndex] == nullptr)
		{
			UBorder* DiceCardBorder = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*FString::Printf(TEXT("RuntimeDicePanelCard_%d"), DiceIndex))
			);
			if (DiceCardBorder != nullptr)
			{
				DiceCardBorder->SetVisibility(ESlateVisibility::Collapsed);
				mRootCanvas->AddChildToCanvas(DiceCardBorder);
				mDicePanelCardBorders[DiceIndex] = DiceCardBorder;
			}
		}

		if (mDicePanelImages[DiceIndex] != nullptr)
		{
			continue;
		}

		UImage* DiceImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			FName(*FString::Printf(TEXT("DicePanelImage_%d"), DiceIndex))
		);
		if (DiceImage == nullptr)
		{
			continue;
		}

		DiceImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		mRootCanvas->AddChildToCanvas(DiceImage);
		mDicePanelImages[DiceIndex] = DiceImage;
	}
}

void UDicePanelWidget::ApplyDicePanelCaptureBrush(UImage* DiceImage, ACombatDiceCaptureActor* DiceActor, FVector2D BrushSize) const
{
	RDDiceCapturePreview::ApplyCaptureBrush(DiceImage, DiceActor, BrushSize);
}

ACombatDiceCaptureActor* UDicePanelWidget::SpawnDicePanelCaptureActor(int32 DiceIndex, int32 RenderTargetSize)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	return RDDiceCapturePreview::SpawnCaptureActor(World, this, RDDicePanelLayout::GetPreviewActorLocation(DiceIndex), RenderTargetSize);
}

void UDicePanelWidget::DestroyDicePanelCaptureActors()
{
	RDDiceCapturePreview::DestroyCaptureActors(mDicePanelPreviewActors);
}

void UDicePanelWidget::RefreshDicePanelPreviewActors()
{
	EnsureDicePanelPreviewWidgets();

	for (int32 DiceIndex = 0; DiceIndex < DicePanelSlotCount; ++DiceIndex)
	{
		UImage* DiceImage = mDicePanelImages.IsValidIndex(DiceIndex) ? mDicePanelImages[DiceIndex].Get() : nullptr;
		if (DiceImage == nullptr)
		{
			continue;
		}

		const bool bShouldShowDice = mDicePanelViews.IsValidIndex(DiceIndex);
		DiceImage->SetVisibility(bShouldShowDice ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShouldShowDice == false)
		{
			continue;
		}

		if (IsValid(mDicePanelPreviewActors[DiceIndex]) == false)
		{
			mDicePanelPreviewActors[DiceIndex] = SpawnDicePanelCaptureActor(DiceIndex, RDDicePanelLayout::GetPreviewRenderTargetSize());
		}

		ACombatDiceCaptureActor* DicePreviewActor = mDicePanelPreviewActors[DiceIndex].Get();
		if (DicePreviewActor == nullptr)
		{
			continue;
		}

		const float DiceScale = RDDicePanelLayout::GetPreviewDiceScale(mDicePanelCarouselActivated && DiceIndex == mSelectedDicePanelIndex);
		DicePreviewActor->SetActorScale3D(FVector(DiceScale));
		DicePreviewActor->SetBackdropVisible(false);
		DicePreviewActor->SetDiceColor(RDUIDice::GetDiceRarityColor(mDicePanelViews[DiceIndex].mRarityType, RDUIDice::EDiceRarityColorTone::DicePanel));
		if (mDicePanelPreviewRotations.IsValidIndex(DiceIndex))
		{
			DicePreviewActor->SetDiceRotation(mDicePanelPreviewRotations[DiceIndex].Rotator());
		}
		DicePreviewActor->CaptureDice();
		ApplyDicePanelCaptureBrush(DiceImage, DicePreviewActor, RDDicePanelLayout::GetPreviewBrushSize());
	}
}

FRotator UDicePanelWidget::GetDicePanelIdleRotation(int32 DiceIndex) const
{
	return RDDicePanelLayout::GetIdleRotation(DiceIndex);
}
