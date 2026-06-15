#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "GameMode/FrontendGameMode.h"
#include "UI/CharacterCardWidget.h"

namespace
{
	FText TitleMenuText(const TCHAR* Key)
	{
		if (FCString::Strcmp(Key, TEXT("ConfirmText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "ConfirmText", "CONFIRM");
		}
		if (FCString::Strcmp(Key, TEXT("BackText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "BackText", "BACK");
		}
		if (FCString::Strcmp(Key, TEXT("ReadyStatusText")) == 0)
		{
			return FText::GetEmpty();
		}
		if (FCString::Strcmp(Key, TEXT("LoadingStatusText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "LoadingStatusText", "Loading");
		}
		if (FCString::Strcmp(Key, TEXT("FailedStatusText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "FailedStatusText", "Failed");
		}
		if (FCString::Strcmp(Key, TEXT("NoCharacterStatusText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "NoCharacterStatusText", "No character data");
		}
		if (FCString::Strcmp(Key, TEXT("CharacterSelectText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "CharacterSelectText", "Character Select");
		}
		if (FCString::Strcmp(Key, TEXT("SelectedCharacterFormat")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "SelectedCharacterFormat", "{0} selected");
		}
		if (FCString::Strcmp(Key, TEXT("CharacterLockedStatus")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "CharacterLockedStatus", "This character is not available");
		}
		if (FCString::Strcmp(Key, TEXT("PortraitFallbackText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "PortraitFallbackText", "No portrait");
		}
		if (FCString::Strcmp(Key, TEXT("CharacterStatFormat")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "CharacterStatFormat", "HP {0} / Dice {1} / Gold {2}");
		}
		return FText::FromString(Key);
	}
}

UCharacterSelectWidget::UCharacterSelectWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RefreshLocalizedTextCache();
	SetVisibility(ESlateVisibility::Visible);
}

void UCharacterSelectWidget::OpenCharacterSelect()
{
	mStartRequested = false;
	RefreshLocalizedTextCache();
	RefreshCharacterOptions();
	SetStatusText(mReadyStatusText);
	SetConfirmButtonText(mConfirmText);
}

void UCharacterSelectWidget::RefreshCharacterOptionsFromGameMode()
{
	RefreshCharacterOptions();
}

void UCharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ValidateDesignerBindings();
	BindEvents();
	RefreshLocalizedTextCache();
	RefreshCharacterOptions();
	SetStatusText(mReadyStatusText);
}

void UCharacterSelectWidget::NativeDestruct()
{
	CancelPortraitLoad();
	UnbindEvents();
	Super::NativeDestruct();
}

void UCharacterSelectWidget::RefreshLocalizedTextCache()
{
	mConfirmText = TitleMenuText(TEXT("ConfirmText"));
	mBackText = TitleMenuText(TEXT("BackText"));
	mReadyStatusText = TitleMenuText(TEXT("ReadyStatusText"));
	mLoadingStatusText = TitleMenuText(TEXT("LoadingStatusText"));
	mFailedStatusText = TitleMenuText(TEXT("FailedStatusText"));
	mNoCharacterStatusText = TitleMenuText(TEXT("NoCharacterStatusText"));
	mCharacterSelectText = TitleMenuText(TEXT("CharacterSelectText"));
}

void UCharacterSelectWidget::BindEvents()
{
	if (mConfirmButton != nullptr)
	{
		mConfirmButton->OnClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleConfirmButtonClicked);
	}
	if (mBackToMainButton != nullptr)
	{
		mBackToMainButton->OnClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleBackToMainButtonClicked);
	}
}

void UCharacterSelectWidget::UnbindEvents()
{
	if (mConfirmButton != nullptr)
	{
		mConfirmButton->OnClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleConfirmButtonClicked);
	}
	if (mBackToMainButton != nullptr)
	{
		mBackToMainButton->OnClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleBackToMainButtonClicked);
	}

	for (UCharacterCardWidget* CardWidget : mCharacterCardWidgets)
	{
		if (CardWidget != nullptr)
		{
			CardWidget->OnCharacterCardClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleCharacterCardClicked);
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

void UCharacterSelectWidget::RebuildCharacterCards()
{
	if (mCharacterCardContainer == nullptr || CharacterCardWidgetClass == nullptr)
	{
		return;
	}

	for (UCharacterCardWidget* CardWidget : mCharacterCardWidgets)
	{
		if (CardWidget != nullptr)
		{
			CardWidget->OnCharacterCardClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleCharacterCardClicked);
		}
	}
	mCharacterCardWidgets.Reset();
	mCharacterCardContainer->ClearChildren();

	for (const FFrontendCharacterOption& Option : mCharacterOptions)
	{
		UCharacterCardWidget* NewWidget = CreateWidget<UCharacterCardWidget>(this, CharacterCardWidgetClass);
		if (NewWidget == nullptr)
		{
			continue;
		}

		NewWidget->OnCharacterCardClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleCharacterCardClicked);
		mCharacterCardContainer->AddChild(NewWidget);
		NewWidget->SetCharacterOption(Option, Option.mIndex == mSelectedCharacterIndex);
		mCharacterCardWidgets.Add(NewWidget);
	}
}

void UCharacterSelectWidget::SyncCharacterCards() const
{
	for (UCharacterCardWidget* CardWidget : mCharacterCardWidgets)
	{
		if (CardWidget != nullptr)
		{
			const int32 CardIndex = mCharacterCardWidgets.IndexOfByKey(CardWidget);
			CardWidget->SetSelected(CardIndex == mSelectedCharacterIndex);
		}
	}
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
		const bool bHasPortrait = !SelectedOption->mPortrait.IsNull();
		mSelectedCharacterPortraitFallbackText->SetVisibility(bHasPortrait ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		mSelectedCharacterPortraitFallbackText->SetText(SelectedOption->mDisplayName);
	}

	SetPortraitImage(SelectedOption->mPortrait);

	if (mConfirmButton != nullptr)
	{
		mConfirmButton->SetIsEnabled(SelectedOption->mSelectable && !mStartRequested);
	}

	SetStatusText(SelectedOption->mSelectable
		? FText::Format(TitleMenuText(TEXT("SelectedCharacterFormat")), SelectedOption->mDisplayName)
		: (SelectedOption->mDisabledReason.IsEmpty() ? TitleMenuText(TEXT("CharacterLockedStatus")) : SelectedOption->mDisabledReason));
}

void UCharacterSelectWidget::ClearSelectedCharacter()
{
	CancelPortraitLoad();
	ApplyPortraitImage(nullptr);

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
		mSelectedCharacterPortraitFallbackText->SetText(TitleMenuText(TEXT("PortraitFallbackText")));
		mSelectedCharacterPortraitFallbackText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (mConfirmButton != nullptr)
	{
		mConfirmButton->SetIsEnabled(false);
	}
}

void UCharacterSelectWidget::SetStatusText(const FText& InText)
{
	if (mCharacterStatusText != nullptr)
	{
		const bool bHideDefaultStatus = InText.IsEmptyOrWhitespace() || InText.EqualTo(mReadyStatusText);
		mCharacterStatusText->SetVisibility(bHideDefaultStatus ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		mCharacterStatusText->SetText(InText);
	}

	OnStatusTextChanged.Broadcast(InText);
}

void UCharacterSelectWidget::SetConfirmButtonText(const FText& InText) const
{
	if (mConfirmButtonText != nullptr)
	{
		mConfirmButtonText->SetText(InText);
	}
	if (mBackToMainButtonText != nullptr)
	{
		mBackToMainButtonText->SetText(mBackText);
	}
}

void UCharacterSelectWidget::SetPortraitImage(const TSoftObjectPtr<UTexture2D>& Portrait)
{
	if (mSelectedCharacterPortraitImage == nullptr)
	{
		CancelPortraitLoad();
		return;
	}

	if (Portrait.IsNull())
	{
		CancelPortraitLoad();
		ApplyPortraitImage(nullptr);
		return;
	}

	if (UTexture2D* LoadedTexture = Portrait.Get())
	{
		CancelPortraitLoad();
		ApplyPortraitImage(LoadedTexture);
		return;
	}

	const FSoftObjectPath PortraitPath = Portrait.ToSoftObjectPath();
	if (!PortraitPath.IsValid())
	{
		CancelPortraitLoad();
		ApplyPortraitImage(nullptr);
		return;
	}

	if (mPortraitLoadHandle.IsValid() && mPendingPortraitPath == PortraitPath)
	{
		return;
	}

	CancelPortraitLoad();
	ApplyPortraitImage(nullptr);

	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		mPendingPortraitPath = PortraitPath;
		mPortraitLoadHandle = AssetManager->GetStreamableManager().RequestAsyncLoad(
			PortraitPath,
			FStreamableDelegate::CreateUObject(this, &UCharacterSelectWidget::HandlePortraitLoaded, PortraitPath));
	}
}

void UCharacterSelectWidget::ApplyPortraitImage(UTexture2D* Texture) const
{
	if (mSelectedCharacterPortraitImage == nullptr)
	{
		return;
	}

	if (Texture != nullptr)
	{
		mSelectedCharacterPortraitImage->SetBrushFromTexture(Texture, true);
		mSelectedCharacterPortraitImage->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		mSelectedCharacterPortraitImage->SetBrushFromTexture(nullptr);
		mSelectedCharacterPortraitImage->SetColorAndOpacity(FLinearColor(0.080f, 0.095f, 0.095f, 1.f));
	}
}

void UCharacterSelectWidget::HandlePortraitLoaded(FSoftObjectPath PortraitPath)
{
	if (mPendingPortraitPath != PortraitPath)
	{
		return;
	}

	ApplyPortraitImage(Cast<UTexture2D>(PortraitPath.ResolveObject()));
	mPortraitLoadHandle.Reset();
	mPendingPortraitPath.Reset();
}

void UCharacterSelectWidget::CancelPortraitLoad()
{
	if (mPortraitLoadHandle.IsValid())
	{
		mPortraitLoadHandle->CancelHandle();
		mPortraitLoadHandle.Reset();
	}

	mPendingPortraitPath.Reset();
}

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
		TitleMenuText(TEXT("CharacterStatFormat")),
		FText::AsNumber(Option.mMaxHP),
		FText::AsNumber(Option.mDice),
		FText::AsNumber(Option.mGold));
}

void UCharacterSelectWidget::ValidateDesignerBindings() const
{
	if (mCharacterCardContainer == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: WBP_CharacterSelect requires mCharacterCardContainer."));
	}
	if (CharacterCardWidgetClass == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: CharacterCardWidgetClass is not set."));
	}
	if (mConfirmButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: WBP_CharacterSelect requires mConfirmButton."));
	}
	if (mBackToMainButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: WBP_CharacterSelect requires mBackToMainButton."));
	}
}

void UCharacterSelectWidget::HandleCharacterCardClicked(int32 CharacterIndex)
{
	SelectCharacter(CharacterIndex);
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
			: TitleMenuText(TEXT("CharacterLockedStatus")));
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
