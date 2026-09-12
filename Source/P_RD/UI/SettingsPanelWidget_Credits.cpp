#include "UI/SettingsPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/CreditsPanelWidget.h"

void USettingsPanelWidget::EnsureCreditsButtons()
{
	// The old mockup labels are retired. Add functional controls to the current
	// design canvas so they inherit the settings book's existing aspect fitting.
	UCanvasPanel* Canvas = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("SettingsModalCanvas"))) : nullptr;
	if (!Canvas) { return; }
	const auto MakeButton = [this, Canvas](const TCHAR* Name, FVector2D Position)
	{
		UButton* Button = Cast<UButton>(WidgetTree->FindWidget(Name));
		if (!Button)
		{
			Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
			FButtonStyle Style = Button->GetStyle();
			// Share the settings ledger's authored gold-and-wood choice plate.
			UTexture2D* Texture = GetDefault<UCreditsPanelWidget>()->GetChoiceTexture();
			FSlateBrush Plate;
			Plate.SetResourceObject(Texture);
			Plate.DrawAs = ESlateBrushDrawType::Image;
			Plate.ImageSize = FVector2D(245.f, 90.f);
			FSlateBrush Hovered = Plate;
			Hovered.TintColor = FLinearColor(1.14f, 1.14f, 1.14f, 1.f);
			FSlateBrush Pressed = Plate;
			Pressed.TintColor = FLinearColor(.72f, .72f, .72f, 1.f);
			Style.SetNormal(Plate).SetHovered(Hovered).SetPressed(Pressed);
			Style.SetNormalPadding(FMargin(24.f, 16.f, 24.f, 22.f));
			Style.SetPressedPadding(FMargin(24.f, 16.f, 24.f, 22.f));
			Button->SetStyle(Style);
			UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Button);
			Slot->SetPosition(Position);
			Slot->SetSize(FVector2D(245.f, 90.f));
			Slot->SetZOrder(30);
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
			FSlateFontInfo Font = SettingsTitleText ? SettingsTitleText->GetFont() : Label->GetFont();
			Font.Size = 27;
			Font.OutlineSettings.OutlineSize = 1;
			Font.OutlineSettings.OutlineColor = FLinearColor(.025f, .012f, .004f, 1.f);
			Label->SetFont(Font);
			Label->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, .87f, .60f, 1.f)));
			Label->SetJustification(ETextJustify::Center);
			Label->SetVisibility(ESlateVisibility::HitTestInvisible);
			Button->SetContent(Label);
		}
		Button->SetVisibility(ESlateVisibility::Visible);
		return Button;
	};
	mCreditsButton = MakeButton(TEXT("RuntimeCreditsButton"), FVector2D(315.f, 733.f));
	mLicensesButton = MakeButton(TEXT("RuntimeLicensesButton"), FVector2D(595.f, 733.f));
	mCreditsButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleCreditsClicked);
	mLicensesButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleLicensesClicked);
}

void USettingsPanelWidget::HandleCreditsClicked() { ShowCreditsPage(false); }
void USettingsPanelWidget::HandleLicensesClicked() { ShowCreditsPage(true); }

void USettingsPanelWidget::ShowCreditsPage(bool bLicenses)
{
	CloseCreditsReader();
	mCreditsReader = CreateWidget<UCreditsPanelWidget>(GetWorld(), UCreditsPanelWidget::StaticClass());
	if (!mCreditsReader) { return; }
	mCreditsReturnFocus = bLicenses ? mLicensesButton : mCreditsButton;
	mCreditsReader->OnReturnToSettings.BindUObject(this, &USettingsPanelWidget::HandleCreditsReaderClosed);
	HideAbandonConfirm();
	SetIsEnabled(false);
	mCreditsReader->ShowPage(bLicenses, mValueModel.mUseKoreanLanguage);
}

void USettingsPanelWidget::HandleCreditsReaderClosed()
{
	SetIsEnabled(true);
	if (mCreditsReturnFocus && GetOwningPlayer())
	{
		mCreditsReturnFocus->SetUserFocus(GetOwningPlayer());
	}
}

void USettingsPanelWidget::CloseCreditsReader()
{
	if (mCreditsReader)
	{
		mCreditsReader->OnReturnToSettings.Unbind();
		mCreditsReader->CloseUI();
		mCreditsReader = nullptr;
	}
	SetIsEnabled(true);
}

void USettingsPanelWidget::CloseUI(FOnEndUICloseAnimation Callback)
{
	CloseCreditsReader();
	Super::CloseUI(MoveTemp(Callback));
}
