#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/CharacterCardWidget.h"
#include "UI/CharacterSelectWidgetPrivate.h"

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
	UnbindEvents();
	Super::NativeDestruct();
}

void UCharacterSelectWidget::RefreshLocalizedTextCache()
{
	mConfirmText = RDCharacterSelect::Text(TEXT("ConfirmText"));
	mBackText = RDCharacterSelect::Text(TEXT("BackText"));
	mReadyStatusText = RDCharacterSelect::Text(TEXT("ReadyStatusText"));
	mLoadingStatusText = RDCharacterSelect::Text(TEXT("LoadingStatusText"));
	mFailedStatusText = RDCharacterSelect::Text(TEXT("FailedStatusText"));
	mNoCharacterStatusText = RDCharacterSelect::Text(TEXT("NoCharacterStatusText"));
	mCharacterSelectText = RDCharacterSelect::Text(TEXT("CharacterSelectText"));
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

void UCharacterSelectWidget::ValidateDesignerBindings() const
{
	if (mCharacterCardContainer == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: WBP_CharacterSelect requires mCharacterCardContainer."));
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
