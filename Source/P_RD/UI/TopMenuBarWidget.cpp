#include "UI/TopMenuBarWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/TitleMenuWidget.h"

UTopMenuBarWidget::UTopMenuBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, mTitleText(NSLOCTEXT("TopMenuBarWidget", "TitleText", "P_RD"))
	, mMenuText(NSLOCTEXT("TopMenuBarWidget", "MenuText", "Run"))
	, mMapText(NSLOCTEXT("TopMenuBarWidget", "MapText", "MAP"))
	, mDiceText(NSLOCTEXT("TopMenuBarWidget", "DiceText", "DICE 0"))
	, mSkillText(NSLOCTEXT("TopMenuBarWidget", "SkillText", "SKILL 0"))
	, mSettingsText(NSLOCTEXT("TopMenuBarWidget", "SettingsText", "SET"))
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTopMenuBarWidget::SetTitle(const FText& InText)
{
	mTitleText = InText;
	if (TitleTextBlock != nullptr)
	{
		TitleTextBlock->SetText(mTitleText);
	}
}

void UTopMenuBarWidget::SetSummary(const FText& InText)
{
	mMenuText = InText;
	if (SummaryTextBlock != nullptr)
	{
		SummaryTextBlock->SetText(mMenuText);
	}
}

void UTopMenuBarWidget::SetDiceLabel(const FText& InText)
{
	mDiceText = InText;
	if (DiceTextBlock != nullptr)
	{
		DiceTextBlock->SetText(mDiceText);
	}
}

void UTopMenuBarWidget::SetSkillLabel(const FText& InText)
{
	mSkillText = InText;
	if (SkillTextBlock != nullptr)
	{
		SkillTextBlock->SetText(mSkillText);
	}
}

void UTopMenuBarWidget::OpenUI_Implementation()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UTopMenuBarWidget::CloseUI_Implementation()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTopMenuBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MapButton != nullptr)
	{
		MapButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleMapButtonClicked);
	}
	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleSettingsButtonClicked);
	}

	SyncText();
}

void UTopMenuBarWidget::NativeDestruct()
{
	if (MapButton != nullptr)
	{
		MapButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleMapButtonClicked);
	}
	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleSettingsButtonClicked);
	}

	Super::NativeDestruct();
}

void UTopMenuBarWidget::SyncText()
{
	SetTitle(mTitleText);
	SetSummary(mMenuText);
	SetDiceLabel(mDiceText);
	SetSkillLabel(mSkillText);

	if (MapButtonText != nullptr)
	{
		MapButtonText->SetText(mMapText);
	}
	if (SettingsButtonText != nullptr)
	{
		SettingsButtonText->SetText(mSettingsText);
	}
}

void UTopMenuBarWidget::OpenMap() const
{
	if (UWorld* World = GetWorld())
	{
		if (UWorldWidgetSubsystem* WorldWidgetSubsystem = World->GetSubsystem<UWorldWidgetSubsystem>())
		{
			if (UTitleMenuWidget* TitleMenuWidget = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>())
			{
				TitleMenuWidget->OpenMapFromTopBar();
			}
		}
	}
}

void UTopMenuBarWidget::OpenSettings() const
{
	if (UWorld* World = GetWorld())
	{
		if (UWorldWidgetSubsystem* WorldWidgetSubsystem = World->GetSubsystem<UWorldWidgetSubsystem>())
		{
			if (UTitleMenuWidget* TitleMenuWidget = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>())
			{
				TitleMenuWidget->OpenSettingsFromTopBar();
			}
			else
			{
				WorldWidgetSubsystem->ShowMsgNotify(NSLOCTEXT("TopMenuBarWidget", "SettingsUnavailable", "Settings are not ready in this room yet."), 2.f);
			}
		}
	}
}

void UTopMenuBarWidget::HandleMapButtonClicked()
{
	OpenMap();
}

void UTopMenuBarWidget::HandleSettingsButtonClicked()
{
	OpenSettings();
}
