#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::PrepareIntroDiceRoll()
{
	EnsureDicePreviewActors();

	for (FDiceViewData& DiceView : mDiceViews)
	{
		DiceView.mIsRolled = false;
	}
	RefreshOwnedDiceCards();

	if (mDicePreviewActors.IsEmpty() || mDiceViews.IsEmpty())
	{
		SetDiceRollVisibility(ESlateVisibility::Collapsed);
		mIntroDiceRollReady = false;
		mIntroDiceRollActive = false;
		mIntroDiceResultWaitingForDismiss = false;
		return;
	}

	for (int32 DiceIndex = 0; DiceIndex < mDicePreviewActors.Num(); ++DiceIndex)
	{
		if (ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex])
		{
			const FLinearColor DiceColor = mDiceViews.IsValidIndex(DiceIndex)
				? RDUIDice::GetDiceRarityColor(mDiceViews[DiceIndex].mRarityType)
				: RDUIDice::GetDiceRarityColor(ERarityType::Common);
			const FRotator StartRotation = GetReadableDiceIdleRotation(DiceIndex);
			DicePreviewActor->SetDiceColor(DiceColor);
			DicePreviewActor->SetDiceRotation(StartRotation);
			if (mIntroDiceSettleStartRotations.IsValidIndex(DiceIndex))
			{
				mIntroDiceSettleStartRotations[DiceIndex] = StartRotation;
			}
			DicePreviewActor->CaptureDice();
		}
	}

	mIntroDiceRollElapsed = 0.0f;
	mIntroDiceRollActive = false;
	mIntroDiceRollReady = true;
	mIntroDiceResultWaitingForDismiss = false;
	mIntroDiceSettling = false;
	mIntroDiceResultsApplied = false;

	SetDiceRollVisibility(ESlateVisibility::HitTestInvisible);
	if (DiceRollStatusText != nullptr)
	{
		DiceRollStatusText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceReadyAllFormat", "TAP TO ROLL\n{0} DICE"),
			FText::AsNumber(mDiceViews.Num())
		));
	}
}

void UCombatTileMapHUDWidget::StartIntroDiceRoll()
{
	if (mIntroDiceRollReady == false)
	{
		PrepareIntroDiceRoll();
	}

	if (mIntroDiceRollReady == false || mDicePreviewActors.IsEmpty() || mDiceViews.IsEmpty())
	{
		SetDiceRollVisibility(ESlateVisibility::Collapsed);
		return;
	}

	for (FDiceViewData& DiceView : mDiceViews)
	{
		DiceView.mIsRolled = false;
	}
	RefreshOwnedDiceCards();

	mIntroDiceRollElapsed = 0.0f;
	mIntroDiceRollActive = true;
	mIntroDiceRollReady = false;
	mIntroDiceResultWaitingForDismiss = false;
	mIntroDiceSettling = false;
	mIntroDiceResultsApplied = false;

	SetDiceRollVisibility(ESlateVisibility::HitTestInvisible);
	if (DiceRollStatusText != nullptr)
	{
		DiceRollStatusText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceRollingAllFormat", "ROLLING DICE\n{0} DICE"),
			FText::AsNumber(mDiceViews.Num())
		));
	}
}

void UCombatTileMapHUDWidget::UpdateIntroDiceRoll(float InDeltaTime)
{
	if (mDicePreviewActors.IsEmpty())
	{
		mIntroDiceRollActive = false;
		return;
	}

	mIntroDiceRollElapsed += InDeltaTime;

	if (mIntroDiceRollElapsed < mIntroDiceRollDuration)
	{
		const float Alpha = FMath::Clamp(mIntroDiceRollElapsed / mIntroDiceRollDuration, 0.0f, 1.0f);
		const float SettleStartAlpha = 0.66f;

		if (Alpha < SettleStartAlpha)
		{
			const float SpinTime = mIntroDiceRollElapsed;
			for (int32 DiceIndex = 0; DiceIndex < mDicePreviewActors.Num(); ++DiceIndex)
			{
				ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex];
				if (DicePreviewActor == nullptr)
				{
					continue;
				}

				const float DiceOffset = StaticCast<float>(DiceIndex);
				const FRotator RollingRotation(
					26.0f + SpinTime * (980.0f + DiceOffset * 145.0f),
					-35.0f + SpinTime * (1360.0f + DiceOffset * 125.0f),
					18.0f + SpinTime * (720.0f + DiceOffset * 95.0f)
				);

				if (mIntroDiceSettleStartRotations.IsValidIndex(DiceIndex))
				{
					mIntroDiceSettleStartRotations[DiceIndex] = RollingRotation;
				}
				DicePreviewActor->SetDiceRotation(RollingRotation);
				DicePreviewActor->CaptureDice();
			}
			return;
		}

		if (mIntroDiceSettling == false)
		{
			mIntroDiceSettling = true;
		}

		const float SettleAlpha = EaseOutCubic((Alpha - SettleStartAlpha) / (1.0f - SettleStartAlpha));
		for (int32 DiceIndex = 0; DiceIndex < mDicePreviewActors.Num(); ++DiceIndex)
		{
			ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex];
			if (DicePreviewActor == nullptr || mIntroDiceSettleStartRotations.IsValidIndex(DiceIndex) == false)
			{
				continue;
			}

			const int32 ResultValue = mDiceViews.IsValidIndex(DiceIndex) ? mDiceViews[DiceIndex].mResultValue : 1;
			const FQuat StartQuat = mIntroDiceSettleStartRotations[DiceIndex].Quaternion();
			const FQuat EndQuat = ACombatDiceCaptureActor::GetSettledFaceRotation(ResultValue).Quaternion();
			DicePreviewActor->SetDiceRotation(FQuat::Slerp(StartQuat, EndQuat, SettleAlpha).Rotator());
			DicePreviewActor->CaptureDice();
		}
		return;
	}

	for (int32 DiceIndex = 0; DiceIndex < mDicePreviewActors.Num(); ++DiceIndex)
	{
		if (ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex])
		{
			const int32 ResultValue = mDiceViews.IsValidIndex(DiceIndex) ? mDiceViews[DiceIndex].mResultValue : 1;
			DicePreviewActor->SettleToFace(ResultValue);
			DicePreviewActor->CaptureDice();
		}
	}

	if (mIntroDiceResultsApplied == false)
	{
		MarkAllDiceRolled();
		mIntroDiceResultsApplied = true;
	}

	const float HoldElapsed = mIntroDiceRollElapsed - mIntroDiceRollDuration;
	if (DiceRollStatusText != nullptr)
	{
		FString ResultText;
		for (int32 DiceIndex = 0; DiceIndex < mDiceViews.Num(); ++DiceIndex)
		{
			if (DiceIndex > 0)
			{
				ResultText += TEXT(" / ");
			}
			ResultText += FString::FromInt(mDiceViews[DiceIndex].mResultValue);
		}

		DiceRollStatusText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceResultAllFormat", "DICE RESULT\n{0}"),
			FText::FromString(ResultText)
		));
	}

	if (HoldElapsed >= mIntroDiceHoldDuration)
	{
		mIntroDiceRollActive = false;
		mIntroDiceResultWaitingForDismiss = true;
		if (DiceRollStatusText != nullptr)
		{
			FString ResultText;
			for (int32 DiceIndex = 0; DiceIndex < mDiceViews.Num(); ++DiceIndex)
			{
				if (DiceIndex > 0)
				{
					ResultText += TEXT(" / ");
				}
				ResultText += FString::FromInt(mDiceViews[DiceIndex].mResultValue);
			}

			DiceRollStatusText->SetText(FText::Format(
				NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceResultDismissAllFormat", "DICE RESULT\n{0}\nTAP TO CLOSE"),
				FText::FromString(ResultText)
			));
		}
	}
}

void UCombatTileMapHUDWidget::MarkAllDiceRolled()
{
	for (FDiceViewData& DiceView : mDiceViews)
	{
		DiceView.mIsRolled = true;
	}
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}

void UCombatTileMapHUDWidget::SetDiceRollVisibility(ESlateVisibility NewVisibility) const
{
	for (UImage* DiceImage : mDiceRollImages)
	{
		if (DiceImage != nullptr)
		{
			DiceImage->SetVisibility(NewVisibility);
		}
	}

	if (DiceRollStatusText != nullptr)
	{
		DiceRollStatusText->SetVisibility(NewVisibility);
	}

	if (mDiceRollInputButton != nullptr)
	{
		mDiceRollInputButton->SetVisibility(NewVisibility == ESlateVisibility::Collapsed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UCombatTileMapHUDWidget::HandleDiceRollInputButtonClicked()
{
	if (mIntroDiceRollActive == true)
	{
		return;
	}

	if (mIntroDiceResultWaitingForDismiss == true)
	{
		DismissIntroDiceRoll();
		return;
	}

	StartIntroDiceRoll();
}

void UCombatTileMapHUDWidget::DismissIntroDiceRoll()
{
	mIntroDiceRollActive = false;
	mIntroDiceRollReady = false;
	mIntroDiceResultWaitingForDismiss = false;
	SetDiceRollVisibility(ESlateVisibility::Collapsed);
}
