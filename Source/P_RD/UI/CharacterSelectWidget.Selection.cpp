#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameMode/FrontendGameMode.h"
#include "UI/CharacterSelectWidgetPrivate.h"
#include "UI/UITextureLoader.h"

namespace
{
	/**
	 * @brief 직업별 캐릭터 일러스트 PNG의 Content 상대 경로를 돌려준다.
	 *
	 * @details
	 * 아트 원본은 팀 정책상 SVN(Content/SVN) 아래에 있고, 직업 enum 이름(Archer)과
	 * 아트 파일 이름(rogue)이 다를 수 있어 여기서 한 번만 매핑한다.
	 * 경로가 없는 직업(None 등)은 nullptr을 돌려 호출부에서 포트레이트를 숨기게 한다.
	 */
	const TCHAR* GetJobIllustrationContentPath(EPlayerJobType JobType)
	{
		switch (JobType)
		{
		case EPlayerJobType::Knight:
			return TEXT("SVN/OutSideAsset/AICreation/ClassSelect/classselect_knight_action_illustration_1920x1080.png");
		case EPlayerJobType::Archer:
			return TEXT("SVN/OutSideAsset/AICreation/ClassSelect/classselect_rogue_action_illustration_1920x1080.png");
		case EPlayerJobType::Mage:
			return TEXT("SVN/OutSideAsset/AICreation/ClassSelect/classselect_mage_action_illustration_1920x1080.png");
		default:
			return nullptr;
		}
	}
}

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

void UCharacterSelectWidget::SyncSelectedCharacterArt(EPlayerJobType JobType)
{
	/*
	 * 직업별 액션 이미지는 WBP에 이미 배치돼 있지만 브러시가 비어 있다.
	 * 아트 원본은 팀 정책상 git이 아니라 SVN(Content/SVN)에만 있으므로, 타이틀 로고와 동일하게
	 * 런타임에 PNG를 로드해 각 이미지에 채우고(캐시), 선택된 직업의 이미지만 표시한다.
	 */
	const auto ApplyJobImage = [this](UImage* Image, EPlayerJobType ImageJob, EPlayerJobType SelectedJob)
	{
		if (Image == nullptr)
		{
			return;
		}

		if (ImageJob != SelectedJob)
		{
			Image->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}

		if (UTexture2D* Illustration = GetOrLoadJobIllustration(ImageJob))
		{
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.ImageSize = FVector2D(
				static_cast<float>(Illustration->GetSizeX()),
				static_cast<float>(Illustration->GetSizeY()));
			Brush.SetResourceObject(Illustration);
			Image->SetBrush(Brush);
			Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image->SetVisibility(ESlateVisibility::Collapsed);
		}
	};

	ApplyJobImage(mKnightActionImage, EPlayerJobType::Knight, JobType);
	ApplyJobImage(mRogueActionImage, EPlayerJobType::Archer, JobType);
	ApplyJobImage(mMageActionImage, EPlayerJobType::Mage, JobType);

	const bool bHasClassActionImages = mKnightActionImage != nullptr || mRogueActionImage != nullptr || mMageActionImage != nullptr;
	if (mSelectedCharacterPortraitImage == nullptr)
	{
		return;
	}

	// 직업별 액션 이미지를 쓰는 경우 포트레이트 이미지는 숨긴다. 액션 이미지가 하나도 없을 때만 포트레이트에 직접 채운다.
	if (bHasClassActionImages)
	{
		mSelectedCharacterPortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (UTexture2D* Illustration = GetOrLoadJobIllustration(JobType))
	{
		FSlateBrush PortraitBrush;
		PortraitBrush.DrawAs = ESlateBrushDrawType::Image;
		PortraitBrush.ImageSize = FVector2D(
			static_cast<float>(Illustration->GetSizeX()),
			static_cast<float>(Illustration->GetSizeY()));
		PortraitBrush.SetResourceObject(Illustration);
		mSelectedCharacterPortraitImage->SetBrush(PortraitBrush);
		mSelectedCharacterPortraitImage->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (mSelectedCharacterPortraitFallbackText != nullptr)
		{
			mSelectedCharacterPortraitFallbackText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		mSelectedCharacterPortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		if (mSelectedCharacterPortraitFallbackText != nullptr)
		{
			mSelectedCharacterPortraitFallbackText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

UTexture2D* UCharacterSelectWidget::GetOrLoadJobIllustration(EPlayerJobType JobType)
{
	if (const TObjectPtr<UTexture2D>* Cached = mJobIllustrationCache.Find(JobType))
	{
		if (*Cached != nullptr)
		{
			return *Cached;
		}
	}

	const TCHAR* ContentPath = GetJobIllustrationContentPath(JobType);
	if (ContentPath == nullptr)
	{
		return nullptr;
	}

	UTexture2D* Loaded = RDUITexture::LoadTextureFromContentPng(ContentPath, TEXT("CharacterSelectWidget"));
	if (Loaded != nullptr)
	{
		mJobIllustrationCache.Add(JobType, Loaded);
	}
	return Loaded;
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
