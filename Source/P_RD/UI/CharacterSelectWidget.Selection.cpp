#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameMode/FrontendGameMode.h"
#include "UI/CharacterSelectWidgetPrivate.h"

void UCharacterSelectWidget::RefreshCharacterOptions()
{
	const int32 PreviousSelectedCharacterIndex = mSelectedCharacterIndex;
	mCharacterOptions.Reset();
	mSelectedPlayerUnitId = FPrimaryAssetId();
	mSelectedCharacterIndex = INDEX_NONE;

	AFrontendGameMode* FrontendGameMode = GetFrontendGameMode();
	if (FrontendGameMode == nullptr || !FrontendGameMode->GetCharacterOptions(OUT mCharacterOptions))
	{
		RebuildCharacterCards();
		ClearSelectedCharacter();
		SetStatusText(mNoCharacterStatusText);
		SetConfirmButtonText(mConfirmText);
		return;
	}

	const FFrontendCharacterOption* PreservedOption = mCharacterOptions.FindByPredicate([PreviousSelectedCharacterIndex](const FFrontendCharacterOption& Option)
	{
		return Option.mIndex == PreviousSelectedCharacterIndex;
	});
	const FFrontendCharacterOption* FirstOption = mCharacterOptions.IsEmpty() ? nullptr : &mCharacterOptions[0];
	const FFrontendCharacterOption* FirstEnabledOption = mCharacterOptions.FindByPredicate([](const FFrontendCharacterOption& Option)
	{
		return Option.mSelectable;
	});
	const FFrontendCharacterOption* SelectedOption = PreservedOption != nullptr
		? PreservedOption
		: (FirstEnabledOption != nullptr ? FirstEnabledOption : FirstOption);

	if (SelectedOption != nullptr)
	{
		mSelectedCharacterIndex = SelectedOption->mIndex;
		mSelectedPlayerUnitId = SelectedOption->mSelectable ? SelectedOption->mPlayerUnitId : FPrimaryAssetId();
	}

	SetConfirmButtonText(mConfirmText);
	RebuildCharacterCards();
	SyncSelectedCharacter();
}

void UCharacterSelectWidget::SelectCharacter(int32 CharacterIndex)
{
	if (mStartRequested)
	{
		return;
	}

	const FFrontendCharacterOption* Option = GetCharacterOption(CharacterIndex);
	if (Option == nullptr)
	{
		SetStatusText(mNoCharacterStatusText);
		return;
	}

	mSelectedCharacterIndex = Option->mIndex;
	mSelectedPlayerUnitId = Option->mSelectable ? Option->mPlayerUnitId : FPrimaryAssetId();
	SyncCharacterCards();
	SyncSelectedCharacter();
}

void UCharacterSelectWidget::SyncSelectedCharacter()
{
	const FFrontendCharacterOption* SelectedOption = GetSelectedCharacterOption();
	if (SelectedOption == nullptr)
	{
		ClearSelectedCharacter();
		return;
	}

	if (mSelectedCharacterNameText != nullptr)
	{
		mSelectedCharacterNameText->SetText(SelectedOption->mDisplayName);
	}
	if (mSelectedCharacterRoleText != nullptr)
	{
		mSelectedCharacterRoleText->SetText(SelectedOption->mRoleText);
	}
	if (mSelectedCharacterStatText != nullptr)
	{
		mSelectedCharacterStatText->SetText(BuildCharacterStatText(*SelectedOption));
	}
	if (mSelectedCharacterDescriptionText != nullptr)
	{
		mSelectedCharacterDescriptionText->SetText(SelectedOption->mDescription);
	}
	if (mSelectedCharacterPortraitFallbackText != nullptr)
	{
		mSelectedCharacterPortraitFallbackText->SetVisibility(ESlateVisibility::Collapsed);
		mSelectedCharacterPortraitFallbackText->SetText(SelectedOption->mDisplayName);
	}

	SyncSelectedCharacterArt(SelectedOption->mJobType);
	BP_OnSelectedCharacterChanged(*SelectedOption);

	if (mConfirmButton != nullptr)
	{
		mConfirmButton->SetIsEnabled(SelectedOption->mSelectable && !mStartRequested);
	}

	SetStatusText(SelectedOption->mSelectable
		? FText::Format(RDCharacterSelect::Text(TEXT("SelectedCharacterFormat")), SelectedOption->mDisplayName)
		: (SelectedOption->mDisabledReason.IsEmpty() ? RDCharacterSelect::Text(TEXT("CharacterLockedStatus")) : SelectedOption->mDisabledReason));
}

void UCharacterSelectWidget::ClearSelectedCharacter()
{
	SyncSelectedCharacterArt(EPlayerJobType::None);
	BP_OnSelectedCharacterCleared();

	if (mSelectedCharacterNameText != nullptr)
	{
		mSelectedCharacterNameText->SetText(mCharacterSelectText);
	}
	if (mSelectedCharacterRoleText != nullptr)
	{
		mSelectedCharacterRoleText->SetText(FText::GetEmpty());
	}
	if (mSelectedCharacterStatText != nullptr)
	{
		mSelectedCharacterStatText->SetText(FText::GetEmpty());
	}
	if (mSelectedCharacterDescriptionText != nullptr)
	{
		mSelectedCharacterDescriptionText->SetText(mNoCharacterStatusText);
	}
	if (mSelectedCharacterPortraitFallbackText != nullptr)
	{
		mSelectedCharacterPortraitFallbackText->SetText(RDCharacterSelect::Text(TEXT("PortraitFallbackText")));
		mSelectedCharacterPortraitFallbackText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (mConfirmButton != nullptr)
	{
		mConfirmButton->SetIsEnabled(false);
	}
}

void UCharacterSelectWidget::SyncSelectedCharacterArt(EPlayerJobType JobType) const
{
	const auto SetActionImageVisibility = [](UImage* Image, bool bVisible)
	{
		if (Image != nullptr)
		{
			Image->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	};

	const bool bHasClassActionImages = mKnightActionImage != nullptr || mRogueActionImage != nullptr || mMageActionImage != nullptr;
	SetActionImageVisibility(mKnightActionImage, JobType == EPlayerJobType::Knight);
	SetActionImageVisibility(mRogueActionImage, JobType == EPlayerJobType::Archer);
	SetActionImageVisibility(mMageActionImage, JobType == EPlayerJobType::Mage);

	if (mSelectedCharacterPortraitImage != nullptr)
	{
		mSelectedCharacterPortraitImage->SetVisibility(bHasClassActionImages
			? ESlateVisibility::Collapsed
			: (JobType == EPlayerJobType::None ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible));
	}
}

const FFrontendCharacterOption* UCharacterSelectWidget::GetCharacterOption(int32 CharacterIndex) const
{
	return mCharacterOptions.FindByPredicate([CharacterIndex](const FFrontendCharacterOption& Option)
	{
		return Option.mIndex == CharacterIndex;
	});
}

const FFrontendCharacterOption* UCharacterSelectWidget::GetSelectedCharacterOption() const
{
	return GetCharacterOption(mSelectedCharacterIndex);
}

FText UCharacterSelectWidget::BuildCharacterStatText(const FFrontendCharacterOption& Option) const
{
	if (!Option.mStatSummary.IsEmpty())
	{
		return Option.mStatSummary;
	}

	return FText::Format(
		RDCharacterSelect::Text(TEXT("CharacterStatFormat")),
		FText::AsNumber(Option.mMaxHP),
		FText::AsNumber(Option.mDice),
		FText::AsNumber(Option.mGold));
}
