#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "GameMode/FrontendGameMode.h"
#include "UI/CharacterSelectWidgetPrivate.h"

bool UCharacterSelectWidget::BeginFirstRoomEntryWithSelectedCharacter()
{
	AFrontendGameMode* FrontendGameMode = GetFrontendGameMode();
	if (FrontendGameMode == nullptr || !mSelectedPlayerUnitId.IsValid())
	{
		return false;
	}

	if (!FrontendGameMode->StartNewRun(mSelectedPlayerUnitId, 1))
	{
		return false;
	}

	return true;
}

AFrontendGameMode* UCharacterSelectWidget::GetFrontendGameMode() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetAuthGameMode<AFrontendGameMode>();
	}

	return nullptr;
}

void UCharacterSelectWidget::HandleConfirmButtonClicked()
{
	if (mStartRequested)
	{
		return;
	}

	const FFrontendCharacterOption* SelectedOption = GetSelectedCharacterOption();
	if (SelectedOption == nullptr || !SelectedOption->mSelectable || !mSelectedPlayerUnitId.IsValid())
	{
		SetStatusText(SelectedOption != nullptr && !SelectedOption->mDisabledReason.IsEmpty()
			? SelectedOption->mDisabledReason
			: RDCharacterSelect::Text(TEXT("CharacterLockedStatus")));
		return;
	}

	mStartRequested = true;
	SetConfirmButtonText(mLoadingStatusText);
	SetStatusText(mLoadingStatusText);
	if (mConfirmButton != nullptr)
	{
		mConfirmButton->SetIsEnabled(false);
	}

	if (!BeginFirstRoomEntryWithSelectedCharacter())
	{
		mStartRequested = false;
		SetConfirmButtonText(mConfirmText);
		SetStatusText(mFailedStatusText);
		if (mConfirmButton != nullptr)
		{
			mConfirmButton->SetIsEnabled(true);
		}
	}
}

void UCharacterSelectWidget::HandleBackToMainButtonClicked()
{
	if (!mStartRequested)
	{
		OnBackToMainRequested.Broadcast();
	}
}
