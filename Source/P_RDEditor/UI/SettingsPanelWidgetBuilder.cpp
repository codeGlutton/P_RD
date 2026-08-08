#include "UI/SettingsPanelWidgetBuilder.h"
#include "UI/UIFont.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace SettingsPanelWidgetBuilder
{
	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	void GrowCanvasWidget(UWidgetBlueprint* Blueprint, const TCHAR* Name,
		const FVector2D MinimumSize)
	{
		UWidget* Widget = Blueprint->WidgetTree->FindWidget(FName(Name));
		UCanvasPanelSlot* Slot = Widget != nullptr
			? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
		if (Slot == nullptr)
		{
			return;
		}
		const FVector2D OldSize = Slot->GetSize();
		const FVector2D NewSize(FMath::Max(OldSize.X, MinimumSize.X),
			FMath::Max(OldSize.Y, MinimumSize.Y));
		Slot->SetPosition(Slot->GetPosition() - (NewSize - OldSize) * .5f);
		Slot->SetSize(NewSize);
	}

	void Build()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
			TEXT("/Game/UI/WBP_SettingsPanel.WBP_SettingsPanel"));
		UWidgetBlueprint* Combat = LoadObject<UWidgetBlueprint>(nullptr,
			TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04"));
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr
			|| Combat == nullptr || Combat->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_SETTINGS_READABILITY_BUILD missing WBP"));
			return;
		}
		UTextBlock* FontSource = Cast<UTextBlock>(
			Combat->WidgetTree->FindWidget(TEXT("RoundText")));
		if (FontSource == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_SETTINGS_READABILITY_BUILD missing font source"));
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		const FSlateFontInfo BaseFont = FontSource->GetFont();
		Blueprint->WidgetTree->ForEachWidget([&BaseFont](UWidget* Widget)
		{
			UTextBlock* Text = Cast<UTextBlock>(Widget);
			if (Text == nullptr)
			{
				return;
			}
			const FString Name = Text->GetName();
			int32 Size = FMath::Max(22, Text->GetFont().Size);
			if (Name.Contains(TEXT("SettingsTitle")))
			{
				Size = 50;
			}
			else if (Name.Contains(TEXT("SectionHeader"))
				|| Name.Contains(TEXT("Set_sec_")))
			{
				Size = 32;
			}
			else if (Name.Contains(TEXT("ButtonText")) || Name.Contains(TEXT("_text")))
			{
				Size = FMath::Max(Size, 25);
			}
			else if (Name.Contains(TEXT("_Label")) || Name.Contains(TEXT("_label")))
			{
				Size = FMath::Max(Size, 24);
			}
			FSlateFontInfo Font = UIFont::Make(BaseFont, Size);
			Font.OutlineSettings.OutlineSize = 1;
			Font.OutlineSettings.OutlineColor = FLinearColor(0.05f, 0.025f, 0.01f, .8f);
			Text->SetFont(Font);
			Text->SetShadowOffset(FVector2D(1.f, 1.f));
			Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .55f));
		});

		for (const TCHAR* Name : { TEXT("BackButtonPlateMount"),
			TEXT("ResetButtonPlateMount") })
		{
			GrowCanvasWidget(Blueprint, Name, FVector2D(250.f, 82.f));
		}
		for (const TCHAR* Name : { TEXT("Set_run_SaveAndExitButton"),
			TEXT("Set_run_AbandonRunButton") })
		{
			GrowCanvasWidget(Blueprint, Name, FVector2D(330.f, 82.f));
		}
		for (const TCHAR* Name : { TEXT("FpsThirtyButtonPlateMount"),
			TEXT("FpsSixtyButtonPlateMount"), TEXT("LowQualityButtonPlateMount"),
			TEXT("MediumQualityButtonPlateMount"), TEXT("HighQualityButtonPlateMount"),
			TEXT("LanguageKoreanButtonPlateMount"), TEXT("LanguageEnglishButtonPlateMount") })
		{
			GrowCanvasWidget(Blueprint, Name, FVector2D(118.f, 58.f));
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
			FSavePackageArgs()))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_SETTINGS_READABILITY_BUILD save failed"));
			return;
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_SETTINGS_READABILITY_BUILD success font=combat sizes=readable"));
	}
}

void RegisterSettingsPanelWidgetBuilderCommands()
{
	using namespace SettingsPanelWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildSettingsReadability"),
		TEXT("Use the combat UI font and larger controls in WBP_SettingsPanel."),
		FConsoleCommandDelegate::CreateStatic(&Build));
}

void UnregisterSettingsPanelWidgetBuilderCommands()
{
	SettingsPanelWidgetBuilder::BuildCommand.Reset();
}
