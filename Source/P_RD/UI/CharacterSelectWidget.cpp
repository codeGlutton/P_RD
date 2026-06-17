#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateTypes.h"
#include "UI/CharacterCardWidget.h"
#include "UI/CharacterSelectWidgetPrivate.h"
#include "UObject/SoftObjectPath.h"

UCharacterSelectWidget::UCharacterSelectWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RefreshLocalizedTextCache();
	SetVisibility(ESlateVisibility::Visible);

	// 직업별 일러스트 텍스처 기본값(SVN 임포트 uasset). 직업 enum(Archer)과 아트 이름(rogue)이 다를 수 있어 여기서 매핑.
	mJobIllustrationAssets.Add(EPlayerJobType::Knight, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/SVN/OutSideAsset/AICreation/ClassSelect/classselect_knight_action_illustration_1920x1080.classselect_knight_action_illustration_1920x1080"))));
	mJobIllustrationAssets.Add(EPlayerJobType::Archer, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/SVN/OutSideAsset/AICreation/ClassSelect/classselect_rogue_action_illustration_1920x1080.classselect_rogue_action_illustration_1920x1080"))));
	mJobIllustrationAssets.Add(EPlayerJobType::Mage, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/SVN/OutSideAsset/AICreation/ClassSelect/classselect_mage_action_illustration_1920x1080.classselect_mage_action_illustration_1920x1080"))));
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
	ApplyButtonStyles();
	RefreshLocalizedTextCache();
	RefreshCharacterOptions();
	SetStatusText(mReadyStatusText);
}

void UCharacterSelectWidget::ApplyButtonStyles() const
{
	struct FButtonTexture { UButton* Button; const TCHAR* Path; };
	const FButtonTexture Targets[] = {
		{ mConfirmButton,    TEXT("/Game/SVN/OutSideAsset/AICreation/Title/UI_Button_Start_DarkFantasy_Base.UI_Button_Start_DarkFantasy_Base") },
		{ mBackToMainButton, TEXT("/Game/SVN/OutSideAsset/AICreation/Title/UI_Button_Settings_DarkFantasy_Base.UI_Button_Settings_DarkFantasy_Base") },
	};

	for (const FButtonTexture& Target : Targets)
	{
		if (Target.Button == nullptr)
		{
			continue;
		}
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, Target.Path);
		if (Texture == nullptr)
		{
			continue;
		}

		FSlateBrush NormalBrush;
		NormalBrush.DrawAs = ESlateBrushDrawType::Image;
		NormalBrush.ImageSize = FVector2D(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY()));
		NormalBrush.SetResourceObject(Texture);

		FSlateBrush HoveredBrush = NormalBrush;
		HoveredBrush.TintColor = FSlateColor(FLinearColor(1.12f, 1.12f, 1.12f, 1.0f));
		FSlateBrush PressedBrush = NormalBrush;
		PressedBrush.TintColor = FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f));

		FButtonStyle Style = Target.Button->GetStyle();
		Style.SetNormal(NormalBrush);
		Style.SetHovered(HoveredBrush);
		Style.SetPressed(PressedBrush);
		Style.SetDisabled(NormalBrush);
		Target.Button->SetStyle(Style);
	}
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
