#include "UI/CharacterCardWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace
{
	FLinearColor GetFallbackIconColor(int32 CharacterIndex, bool bSelectable, bool bSelected)
	{
		static const FLinearColor BaseColors[] =
		{
			FLinearColor(0.72f, 0.16f, 0.12f, 1.0f),
			FLinearColor(0.15f, 0.56f, 0.24f, 1.0f),
			FLinearColor(0.34f, 0.18f, 0.66f, 1.0f),
			FLinearColor(0.70f, 0.55f, 0.18f, 1.0f),
			FLinearColor(0.18f, 0.55f, 0.68f, 1.0f)
		};

		const int32 ColorIndex = FMath::Abs(CharacterIndex) % UE_ARRAY_COUNT(BaseColors);
		const float Brightness = bSelectable ? (bSelected ? 1.0f : 0.55f) : (bSelected ? 0.38f : 0.22f);
		const FLinearColor BaseColor = BaseColors[ColorIndex];
		return FLinearColor(BaseColor.R * Brightness, BaseColor.G * Brightness, BaseColor.B * Brightness, 1.0f);
	}

	void HideLegacyText(UTextBlock* TextBlock)
	{
		if (TextBlock == nullptr)
		{
			return;
		}

		TextBlock->SetText(FText::GetEmpty());
		TextBlock->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UCharacterCardWidget::UCharacterCardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
}

void UCharacterCardWidget::SetCharacterOption(const FFrontendCharacterOption& InOption, bool bSelected)
{
	mCharacterOption = InOption;
	bIsSelected = bSelected;
	SyncCard();
}

void UCharacterCardWidget::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
	SyncCard();
}

void UCharacterCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ValidateDesignerBindings();

	if (CardButton != nullptr)
	{
		CardButton->OnClicked.AddUniqueDynamic(this, &UCharacterCardWidget::HandleCardButtonClicked);
	}

	SyncCard();
}

void UCharacterCardWidget::NativeDestruct()
{
	if (CardButton != nullptr)
	{
		CardButton->OnClicked.RemoveDynamic(this, &UCharacterCardWidget::HandleCardButtonClicked);
	}

	Super::NativeDestruct();
}

void UCharacterCardWidget::SyncCard() const
{
	HideLegacyText(NameText);
	HideLegacyText(RoleText);
	HideLegacyText(StatText);
	HideLegacyText(DescriptionText);
	HideLegacyText(StateText);

	if (CardButton != nullptr)
	{
		CardButton->SetIsEnabled(true);
	}

	if (IconBackground != nullptr)
	{
		IconBackground->SetBrushColor(GetFallbackIconColor(mCharacterOption.mIndex, mCharacterOption.bSelectable, bIsSelected));
	}

	if (IconImage == nullptr)
	{
		return;
	}

	UTexture2D* LoadedIcon = mCharacterOption.mIcon.IsValid()
		? mCharacterOption.mIcon.Get()
		: (mCharacterOption.mPortrait.IsValid() ? mCharacterOption.mPortrait.Get() : nullptr);
	if (LoadedIcon != nullptr)
	{
		IconImage->SetBrushFromTexture(LoadedIcon, true);
		IconImage->SetColorAndOpacity(mCharacterOption.bSelectable ? FLinearColor::White : FLinearColor(0.35f, 0.35f, 0.35f, 0.82f));
		IconImage->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		IconImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCharacterCardWidget::ValidateDesignerBindings() const
{
	if (CardButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterCardWidget: WBP_CharacterCard requires CardButton."));
	}
	if (IconBackground == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterCardWidget: WBP_CharacterCard requires IconBackground."));
	}
	if (IconImage == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterCardWidget: WBP_CharacterCard requires IconImage."));
	}
}

void UCharacterCardWidget::HandleCardButtonClicked()
{
	OnCharacterCardClicked.Broadcast(mCharacterOption.mIndex);
}
