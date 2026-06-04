#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "GameMode/FrontendGameMode.h"
#include "TimerManager.h"
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
	bStartRequested = false;
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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mStageReadyPollTimerHandle);
	}

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
	if (ConfirmButton != nullptr)
	{
		ConfirmButton->OnClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleConfirmButtonClicked);
	}
	if (BackToMainButton != nullptr)
	{
		BackToMainButton->OnClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleBackToMainButtonClicked);
	}
}

void UCharacterSelectWidget::UnbindEvents()
{
	if (ConfirmButton != nullptr)
	{
		ConfirmButton->OnClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleConfirmButtonClicked);
	}
	if (BackToMainButton != nullptr)
	{
		BackToMainButton->OnClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleBackToMainButtonClicked);
	}

	for (UCharacterCardWidget* CardWidget : CharacterCardWidgets)
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
		const bool bIsLoading = FrontendGameMode != nullptr && FrontendGameMode->IsCharacterOptionsLoading();
		SetStatusText(bIsLoading ? mLoadingStatusText : mNoCharacterStatusText);
		SetConfirmButtonText(bIsLoading ? mLoadingStatusText : mConfirmText);
		return;
	}

	const FFrontendCharacterOption* PreservedOption = mCharacterOptions.FindByPredicate([PreviousSelectedCharacterIndex](const FFrontendCharacterOption& Option)
	{
		return Option.mIndex == PreviousSelectedCharacterIndex;
	});
	const FFrontendCharacterOption* FirstOption = mCharacterOptions.IsEmpty() ? nullptr : &mCharacterOptions[0];
	const FFrontendCharacterOption* FirstEnabledOption = mCharacterOptions.FindByPredicate([](const FFrontendCharacterOption& Option)
	{
		return Option.bEnabled;
	});
	const FFrontendCharacterOption* SelectedOption = PreservedOption != nullptr
		? PreservedOption
		: (FirstEnabledOption != nullptr ? FirstEnabledOption : FirstOption);

	if (SelectedOption != nullptr)
	{
		mSelectedCharacterIndex = SelectedOption->mIndex;
		mSelectedPlayerUnitId = SelectedOption->bEnabled ? SelectedOption->mPlayerUnitId : FPrimaryAssetId();
	}

	SetConfirmButtonText(mConfirmText);
	RebuildCharacterCards();
	SyncSelectedCharacter();
}

void UCharacterSelectWidget::RebuildCharacterCards()
{
	if (CharacterCardContainer == nullptr || CharacterCardWidgetClass == nullptr)
	{
		return;
	}

	for (UCharacterCardWidget* CardWidget : CharacterCardWidgets)
	{
		if (CardWidget != nullptr)
		{
			CardWidget->OnCharacterCardClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleCharacterCardClicked);
		}
	}
	CharacterCardWidgets.Reset();
	CharacterCardContainer->ClearChildren();

	for (const FFrontendCharacterOption& Option : mCharacterOptions)
	{
		UCharacterCardWidget* NewWidget = CreateWidget<UCharacterCardWidget>(this, CharacterCardWidgetClass);
		if (NewWidget == nullptr)
		{
			continue;
		}

		NewWidget->OnCharacterCardClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleCharacterCardClicked);
		CharacterCardContainer->AddChild(NewWidget);
		NewWidget->SetCharacterOption(Option, Option.mIndex == mSelectedCharacterIndex);
		CharacterCardWidgets.Add(NewWidget);
	}
}

void UCharacterSelectWidget::SyncCharacterCards() const
{
	for (UCharacterCardWidget* CardWidget : CharacterCardWidgets)
	{
		if (CardWidget != nullptr)
		{
			const int32 CardIndex = CharacterCardWidgets.IndexOfByKey(CardWidget);
			CardWidget->SetSelected(CardIndex == mSelectedCharacterIndex);
		}
	}
}

void UCharacterSelectWidget::SelectCharacter(int32 CharacterIndex)
{
	if (bStartRequested)
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
	mSelectedPlayerUnitId = Option->bEnabled ? Option->mPlayerUnitId : FPrimaryAssetId();
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

	if (SelectedCharacterNameText != nullptr)
	{
		SelectedCharacterNameText->SetText(SelectedOption->mName);
	}
	if (SelectedCharacterRoleText != nullptr)
	{
		SelectedCharacterRoleText->SetText(SelectedOption->mRole);
	}
	if (SelectedCharacterStatText != nullptr)
	{
		SelectedCharacterStatText->SetText(BuildCharacterStatText(*SelectedOption));
	}
	if (SelectedCharacterDescriptionText != nullptr)
	{
		SelectedCharacterDescriptionText->SetText(SelectedOption->mDescription);
	}
	if (SelectedCharacterPortraitFallbackText != nullptr)
	{
		const bool bHasPortrait = !SelectedOption->mPortrait.IsNull();
		SelectedCharacterPortraitFallbackText->SetVisibility(bHasPortrait ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		SelectedCharacterPortraitFallbackText->SetText(SelectedOption->mName);
	}

	SetPortraitImage(SelectedOption->mPortrait);

	if (ConfirmButton != nullptr)
	{
		ConfirmButton->SetIsEnabled(SelectedOption->bEnabled && !bStartRequested);
	}

	SetStatusText(SelectedOption->bEnabled
		? FText::Format(TitleMenuText(TEXT("SelectedCharacterFormat")), SelectedOption->mName)
		: TitleMenuText(TEXT("CharacterLockedStatus")));
}

void UCharacterSelectWidget::ClearSelectedCharacter()
{
	CancelPortraitLoad();
	ApplyPortraitImage(nullptr);

	if (SelectedCharacterNameText != nullptr)
	{
		SelectedCharacterNameText->SetText(mCharacterSelectText);
	}
	if (SelectedCharacterRoleText != nullptr)
	{
		SelectedCharacterRoleText->SetText(FText::GetEmpty());
	}
	if (SelectedCharacterStatText != nullptr)
	{
		SelectedCharacterStatText->SetText(FText::GetEmpty());
	}
	if (SelectedCharacterDescriptionText != nullptr)
	{
		SelectedCharacterDescriptionText->SetText(mNoCharacterStatusText);
	}
	if (SelectedCharacterPortraitFallbackText != nullptr)
	{
		SelectedCharacterPortraitFallbackText->SetText(TitleMenuText(TEXT("PortraitFallbackText")));
		SelectedCharacterPortraitFallbackText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (ConfirmButton != nullptr)
	{
		ConfirmButton->SetIsEnabled(false);
	}
}

void UCharacterSelectWidget::SetStatusText(const FText& InText)
{
	if (CharacterStatusText != nullptr)
	{
		const bool bHideDefaultStatus = InText.IsEmptyOrWhitespace() || InText.EqualTo(mReadyStatusText);
		CharacterStatusText->SetVisibility(bHideDefaultStatus ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		CharacterStatusText->SetText(InText);
	}

	OnStatusTextChanged.Broadcast(InText);
}

void UCharacterSelectWidget::SetConfirmButtonText(const FText& InText) const
{
	if (ConfirmButtonText != nullptr)
	{
		ConfirmButtonText->SetText(InText);
	}
	if (BackToMainButtonText != nullptr)
	{
		BackToMainButtonText->SetText(mBackText);
	}
}

void UCharacterSelectWidget::SetPortraitImage(const TSoftObjectPtr<UTexture2D>& Portrait)
{
	if (SelectedCharacterPortraitImage == nullptr)
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
	if (SelectedCharacterPortraitImage == nullptr)
	{
		return;
	}

	if (Texture != nullptr)
	{
		SelectedCharacterPortraitImage->SetBrushFromTexture(Texture, true);
		SelectedCharacterPortraitImage->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		SelectedCharacterPortraitImage->SetBrushFromTexture(nullptr);
		SelectedCharacterPortraitImage->SetColorAndOpacity(FLinearColor(0.080f, 0.095f, 0.095f, 1.f));
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

bool UCharacterSelectWidget::BeginRunPreviewWithSelectedCharacter()
{
	AFrontendGameMode* FrontendGameMode = GetFrontendGameMode();
	if (FrontendGameMode == nullptr || !mSelectedPlayerUnitId.IsValid())
	{
		return false;
	}

	if (!FrontendGameMode->PrepareRunMapWithPlayerUnit(mSelectedPlayerUnitId))
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(mStageReadyPollTimerHandle, this, &UCharacterSelectWidget::HandleRunPreviewReady, 0.1f, true);
	}
	return true;
}

void UCharacterSelectWidget::HandleRunPreviewReady()
{
	if (!IsRunMapPreviewReady())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mStageReadyPollTimerHandle);
	}

	bStartRequested = false;
	SetConfirmButtonText(mConfirmText);
	SetStatusText(mReadyStatusText);
	OnRunPreviewReady.Broadcast();
}

bool UCharacterSelectWidget::IsRunMapPreviewReady() const
{
	TArray<FFrontendMapRoomView> Rooms;
	AFrontendGameMode* FrontendGameMode = GetFrontendGameMode();
	return FrontendGameMode != nullptr && FrontendGameMode->GetMapRoomViews(OUT Rooms);
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
	return FText::Format(
		TitleMenuText(TEXT("CharacterStatFormat")),
		FText::AsNumber(Option.mMaxHP),
		FText::AsNumber(Option.mDice),
		FText::AsNumber(Option.mGold));
}

void UCharacterSelectWidget::ValidateDesignerBindings() const
{
	if (CharacterCardContainer == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: WBP_CharacterSelect requires CharacterCardContainer."));
	}
	if (CharacterCardWidgetClass == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: CharacterCardWidgetClass is not set."));
	}
	if (ConfirmButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: WBP_CharacterSelect requires ConfirmButton."));
	}
	if (BackToMainButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: WBP_CharacterSelect requires BackToMainButton."));
	}
}

void UCharacterSelectWidget::HandleCharacterCardClicked(int32 CharacterIndex)
{
	SelectCharacter(CharacterIndex);
}

void UCharacterSelectWidget::HandleConfirmButtonClicked()
{
	if (bStartRequested)
	{
		return;
	}

	const FFrontendCharacterOption* SelectedOption = GetSelectedCharacterOption();
	if (SelectedOption == nullptr || !SelectedOption->bEnabled || !mSelectedPlayerUnitId.IsValid())
	{
		SetStatusText(TitleMenuText(TEXT("CharacterLockedStatus")));
		return;
	}

	bStartRequested = true;
	SetConfirmButtonText(mLoadingStatusText);
	SetStatusText(mLoadingStatusText);
	if (ConfirmButton != nullptr)
	{
		ConfirmButton->SetIsEnabled(false);
	}

	if (!BeginRunPreviewWithSelectedCharacter())
	{
		bStartRequested = false;
		SetConfirmButtonText(mConfirmText);
		SetStatusText(mFailedStatusText);
		if (ConfirmButton != nullptr)
		{
			ConfirmButton->SetIsEnabled(true);
		}
	}
}

void UCharacterSelectWidget::HandleBackToMainButtonClicked()
{
	if (!bStartRequested)
	{
		OnBackToMainRequested.Broadcast();
	}
}
