#include "UI/DicePanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"
#include "UI/DiceCapturePreviewUtils.h"
#include "UI/DicePanelLayoutPolicy.h"

namespace
{
	/** @brief 굴림 결과를 PreviewActor::GetSettledFaceRotation에 넘길 1-base 눈 값으로 변환한다. */
	int32 GetDicePanelSettledFaceOrdinal(const FDiceViewData& DiceView)
	{
		if (DiceView.mRolledFaceIndex != INDEX_NONE)
		{
			return DiceView.mRolledFaceIndex + 1;
		}

		const int32 MatchingFaceIndex = DiceView.mFaceValues.Find(DiceView.mResultValue);
		if (MatchingFaceIndex != INDEX_NONE)
		{
			return MatchingFaceIndex + 1;
		}

		return FMath::Clamp(DiceView.mResultValue, 1, FMath::Max(1, DiceView.mFaceCount));
	}
}

/** @brief 선택 카드의 현재 회전값을 3D 캡처 액터에 적용하고 다음 캡처를 예약한다. */
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

/** @brief 선택 주사위가 지정 1-base 면을 정면으로 보도록 목표 회전을 설정한다. */
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

	const int32 FaceCount = mDicePanelViews.IsValidIndex(SelectedIndex)
		? FMath::Max(1, mDicePanelViews[SelectedIndex].mFaceCount)
		: 1;
	const int32 ClampedFaceValue = FMath::Clamp(FaceValue, 1, FaceCount);
	ACombatDiceCaptureActor* PanelPreviewActor = mDicePanelPreviewActors.IsValidIndex(SelectedIndex) ? mDicePanelPreviewActors[SelectedIndex].Get() : nullptr;
	mDicePanelTargetPreviewRotations[SelectedIndex] = PanelPreviewActor != nullptr
		? PanelPreviewActor->GetSettledFaceRotation(ClampedFaceValue).Quaternion()
		: FQuat::Identity;
	mDicePanelPreviewRotationAnimations[SelectedIndex] = true;
	mDicePanelSelectedFaceValues[SelectedIndex] = ClampedFaceValue;

	RequestDicePanelPreviewCapture(SelectedIndex);
	RefreshDiceCarouselFaceButtonStyles();
}

/** @brief 목표 회전까지 Slerp하고 움직인 프레임만 RenderTarget을 다시 캡처한다. */
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

		// quaternion dot이 1에 가까우면 같은 회전이다. snap 후 캡처 애니메이션을 멈춘다.
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

/** @brief 여러 입력이 같은 프레임에 들어와도 마지막 DiceIndex만 다음 Tick에서 캡처하게 예약한다. */
void UDicePanelWidget::RequestDicePanelPreviewCapture(int32 DiceIndex)
{
	if (mDicePanelViews.IsValidIndex(DiceIndex) == false)
	{
		return;
	}

	mPendingDicePanelCaptureIndex = DiceIndex;
}

/** @brief 지정 주사위 액터의 현재 회전을 RT에 반영하고 UImage brush를 갱신한다. */
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

/** @brief WBP 카드 슬롯 위에 런타임 카드/RT Image/회전 상태 배열을 슬롯 수만큼 준비한다. */
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

/** @brief 공용 캡처 brush 적용 유틸로 UImage에 알파 합성 머티리얼을 연결한다. */
void UDicePanelWidget::ApplyDicePanelCaptureBrush(UImage* DiceImage, ACombatDiceCaptureActor* DiceActor, FVector2D BrushSize) const
{
	RDDiceCapturePreview::ApplyCaptureBrush(DiceImage, DiceActor, BrushSize);
}

/** @brief 패널 전용 원거리 위치에 캡처 액터를 스폰하고 이 위젯을 RT outer로 넘긴다. */
ACombatDiceCaptureActor* UDicePanelWidget::SpawnDicePanelCaptureActor(int32 DiceIndex, int32 RenderTargetSize)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	return RDDiceCapturePreview::SpawnCaptureActor(World, this, RDDicePanelLayout::GetPreviewActorLocation(DiceIndex), RenderTargetSize);
}

/** @brief 패널 생명주기에 묶인 모든 캡처 액터를 정리한다. */
void UDicePanelWidget::DestroyDicePanelCaptureActors()
{
	RDDiceCapturePreview::DestroyCaptureActors(mDicePanelPreviewActors);
}

/** @brief 표시 데이터와 선택 상태에 맞춰 캡처 액터 타입/면값/색/회전/brush를 갱신한다. */
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
		DicePreviewActor->SetDiceType(mDicePanelViews[DiceIndex].mFaceCount);
		DicePreviewActor->SetFaceData(mDicePanelViews[DiceIndex].mFaceValues, mDicePanelViews[DiceIndex].mFaceTextures);
		DicePreviewActor->SetActorScale3D(FVector(DiceScale));
		DicePreviewActor->SetBackdropVisible(false);
		DicePreviewActor->SetDiceColor(RDUIDice::GetDiceRarityColor(mDicePanelViews[DiceIndex].mRarityType, RDUIDice::EDiceRarityColorTone::DicePanel));
		// d2/d4는 idle 각도에서 숫자 가독성이 떨어져, 미굴림 상태도 1번 면을 정면으로 둔다.
		const bool bUseFaceOnReadyPose = mDicePanelViews[DiceIndex].mFaceCount == 2 || mDicePanelViews[DiceIndex].mFaceCount == 4;
		if (bUseFaceOnReadyPose
			&& mDicePanelViews[DiceIndex].mIsRolled == false
			&& mDicePanelSelectedFaceValues.IsValidIndex(DiceIndex)
			&& mDicePanelSelectedFaceValues[DiceIndex] == INDEX_NONE
			&& mDicePanelPreviewRotationAnimations.IsValidIndex(DiceIndex)
			&& mDicePanelPreviewRotationAnimations[DiceIndex] == false
			&& mDicePanelPreviewRotations.IsValidIndex(DiceIndex))
		{
			mDicePanelPreviewRotations[DiceIndex] = DicePreviewActor->GetSettledFaceRotation(1).Quaternion();
		}
		if (mDicePanelViews[DiceIndex].mIsRolled
			&& mDicePanelSelectedFaceValues.IsValidIndex(DiceIndex)
			&& mDicePanelSelectedFaceValues[DiceIndex] == INDEX_NONE
			&& mDicePanelPreviewRotationAnimations.IsValidIndex(DiceIndex)
			&& mDicePanelPreviewRotationAnimations[DiceIndex] == false)
		{
			const int32 SettledFaceOrdinal = GetDicePanelSettledFaceOrdinal(mDicePanelViews[DiceIndex]);
			mDicePanelSelectedFaceValues[DiceIndex] = SettledFaceOrdinal;
			if (mDicePanelPreviewRotations.IsValidIndex(DiceIndex))
			{
				mDicePanelPreviewRotations[DiceIndex] = DicePreviewActor->GetSettledFaceRotation(SettledFaceOrdinal).Quaternion();
			}
		}
		if (mDicePanelPreviewRotations.IsValidIndex(DiceIndex))
		{
			DicePreviewActor->SetDiceRotation(mDicePanelPreviewRotations[DiceIndex].Rotator());
		}
		DicePreviewActor->CaptureDice();
		ApplyDicePanelCaptureBrush(DiceImage, DicePreviewActor, RDDicePanelLayout::GetPreviewBrushSize());
	}
}

/** @brief 슬롯별 idle 회전을 레이아웃 정책에서 가져온다. */
FRotator UDicePanelWidget::GetDicePanelIdleRotation(int32 DiceIndex) const
{
	return RDDicePanelLayout::GetIdleRotation(DiceIndex);
}
