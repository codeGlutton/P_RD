#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "UI/CreditsPanelWidget.h"
#include "UI/SettingsPanelWidget.h"
#include "UObject/UnrealType.h"
#include "Widgets/SWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

namespace
{
	TSharedPtr<SButton> FindCreditsButton(const TSharedRef<SWidget>& Widget, const FString& Label, TSharedPtr<SButton> ParentButton = nullptr)
	{
		if (Widget->GetTypeAsString() == TEXT("SButton")) { ParentButton = StaticCastSharedRef<SButton>(Widget); }
		if (Widget->GetTypeAsString() == TEXT("STextBlock") && StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString() == Label) { return ParentButton; }
		FChildren* Children = Widget->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			if (TSharedPtr<SButton> Found = FindCreditsButton(Children->GetChildAt(Index), Label, ParentButton)) { return Found; }
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreditsSettingsEntryTest, "P_RD.UI.Credits.SettingsEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCreditsSettingsEntryTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	UClass* SettingsClass = LoadClass<USettingsPanelWidget>(nullptr, TEXT("/Game/UI/WBP_SettingsPanel.WBP_SettingsPanel_C"));
	if (!TestNotNull(TEXT("Settings class"), SettingsClass)) { return false; }
	USettingsPanelWidget* Settings = CreateWidget<USettingsPanelWidget>(World, SettingsClass);
	TSharedRef<SWidget> SettingsSlate = Settings->TakeWidget();
	Settings->SetVisibility(ESlateVisibility::Visible);
	FObjectPropertyBase* ReaderProperty = FindFProperty<FObjectPropertyBase>(Settings->GetClass(), TEXT("mCreditsReader"));
	if (!TestNotNull(TEXT("Reader ownership property"), ReaderProperty)) { return false; }
	for (const ESettingsPanelMode Mode : { ESettingsPanelMode::Title, ESettingsPanelMode::InGame })
	{
		Settings->SetPanelMode(Mode);
		for (bool bKorean : {true, false})
		{
			FSettingsPanelValueModel Model = Settings->GetValueModel();
			Model.mUseKoreanLanguage = bKorean;
			Settings->ApplyValueModel(Model);
			for (bool bLicense : {false, true})
			{
				UButton* Button = Cast<UButton>(Settings->GetWidgetFromName(bLicense ? TEXT("RuntimeLicensesButton") : TEXT("RuntimeCreditsButton")));
				if (!TestNotNull(TEXT("Visible entry button"), Button)) { return false; }
				TestTrue(TEXT("Entry is visible"), Button->IsVisible());
				const FString Expected = bKorean ? (bLicense ? TEXT("개인정보 · 라이선스") : TEXT("크레딧")) : (bLicense ? TEXT("Privacy & Licenses") : TEXT("Credits"));
				TestEqual(TEXT("Entry follows selected language"), CastChecked<UTextBlock>(Button->GetContent())->GetText().ToString(), Expected);
				Button->OnClicked.Broadcast();
				UCreditsPanelWidget* Reader = Cast<UCreditsPanelWidget>(ReaderProperty->GetObjectPropertyValue_InContainer(Settings));
				if (!TestNotNull(TEXT("Button creates reader"), Reader)) { return false; }
				TestEqual(TEXT("Correct initial tab"), Reader->IsLicensePage(), bLicense);
				TestEqual(TEXT("Reader follows language"), Reader->IsKorean(), bKorean);
				TestFalse(TEXT("Underlying settings blocked"), Settings->GetIsEnabled());
				if (bLicense) { TestTrue(TEXT("Mobile Back is handled by the reader"), Reader->HandleBackNavigation()); }
				else { Reader->ReturnToSettings(); }
				TestTrue(TEXT("Returning restores settings input"), Settings->GetIsEnabled());
				TestFalse(TEXT("Reader closes"), Reader->IsVisible());
			}
		}
	}
	Settings->ShowCreditsPage(true);
	UCreditsPanelWidget* LastReader = Cast<UCreditsPanelWidget>(ReaderProperty->GetObjectPropertyValue_InContainer(Settings));
	Settings->CloseUI();
	TestFalse(TEXT("Parent close also closes reader"), LastReader && LastReader->IsVisible());
	TestNull(TEXT("Parent releases reader"), ReaderProperty->GetObjectPropertyValue_InContainer(Settings));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreditsDocumentsTest, "P_RD.UI.Credits.Documents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCreditsDocumentsTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* Relative : { TEXT("Credits/Models.txt"), TEXT("Credits/AI.txt"), TEXT("Credits/AI_KO.txt"), TEXT("Credits/Audio.txt"), TEXT("Credits/VFX.txt"), TEXT("Credits/Fonts.txt"), TEXT("Credits/Engine.txt"),
		TEXT("Licenses/GowunBatang.txt"), TEXT("Licenses/LINESeedKR.txt"), TEXT("Licenses/Oswald.txt"), TEXT("Licenses/Roboto.txt"),
		TEXT("Licenses/Noto.txt"), TEXT("Licenses/DroidSans.txt"), TEXT("Licenses/LastResort.txt"), TEXT("Policies/Privacy.txt"), TEXT("Policies/Privacy_KO.txt") })
	{
		FString Text;
		TestTrue(Relative, FFileHelper::LoadFileToString(Text, *(UCreditsPanelWidget::LegalDirectory() / Relative)));
		TestFalse(TEXT("No missing copyright placeholder"), Text.Contains(TEXT("<Copyright Holder>")));
		TestFalse(TEXT("No audit notes in player content"), Text.Contains(TEXT("미확인")));
		TestTrue(TEXT("Document is not empty"), Text.Len() > 100);
	}
	FString EngineText;
	FFileHelper::LoadFileToString(EngineText, *(UCreditsPanelWidget::LegalDirectory() / TEXT("Credits/Engine.txt")));
	TestTrue(TEXT("UE trademark notice present"), EngineText.Contains(TEXT("in the United States of America and elsewhere.")));
	TestTrue(TEXT("UE copyright notice present"), EngineText.Contains(TEXT("Epic Games, Inc. All rights reserved.")));
	FString LastResort;
	FFileHelper::LoadFileToString(LastResort, *(UCreditsPanelWidget::LegalDirectory() / TEXT("Licenses/LastResort.txt")));
	TestTrue(TEXT("Last Resort full document includes final section"), LastResort.Contains(TEXT("9. Complete Agreement; Governing Language")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreditsCaptureTest, "P_RD.UI.Credits.Capture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCreditsCaptureTest::RunTest(const FString& Parameters)
{
	if (GUsingNullRHI) { AddError(TEXT("Credits capture requires a rendering RHI.")); return false; }
	UWorld* World = GEditor->GetEditorWorldContext().World();
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("UI/Credits") / (FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + TEXT("_") + FGuid::NewGuid().ToString(EGuidFormats::Short));
	IFileManager::Get().MakeDirectory(*Directory, true);
	for (const FIntPoint Size : {FIntPoint(1672, 941), FIntPoint(2176, 1812)})
	{
		for (int32 CapturePage : {0, 1, 2})
		{
		const bool bLicense = CapturePage == 2;
		for (bool bKorean : {true, false})
		{
			UCreditsPanelWidget* Reader = CreateWidget<UCreditsPanelWidget>(World);
			Reader->ShowPage(bLicense, bKorean);
			Reader->SetVisibility(ESlateVisibility::Visible);
			for (UTexture2D* Texture : {Reader->GetBookTexture(), Reader->GetRibbonTexture(), Reader->GetChoiceTexture(), Reader->GetSelectedChoiceTexture(), Reader->GetNavigationTexture(), Reader->GetBackTexture(), Reader->GetDividerTexture()})
			{
				if (!TestNotNull(TEXT("Authored ledger art"), Texture)) { return false; }
				Texture->SetForceMipLevelsToBeResident(30.f);
				Texture->WaitForStreaming();
			}
			TSharedRef<SWidget> Slate = Reader->TakeWidget();
			if (CapturePage == 1)
			{
				TSharedPtr<SButton> AIButton = FindCreditsButton(Slate, bKorean ? TEXT("이미지 · AI 제작") : TEXT("Images & AI"));
				if (!TestTrue(TEXT("AI disclosure is reachable from category navigation"), AIButton.IsValid())) { return false; }
				AIButton->SimulateClick();
			}
			FWidgetRenderer Renderer(true, true);
			UTextureRenderTarget2D* Target = Renderer.DrawWidget(Slate, FVector2D(Size.X, Size.Y));
			// Auto-wrapped text needs the measured column width from the first pass.
			Renderer.DrawWidget(Target, Slate, FVector2D(Size.X, Size.Y), 0.f);
			FlushRenderingCommands();
			TArray<FColor> Pixels;
			FReadSurfaceDataFlags Flags(RCM_UNorm);
			Flags.SetLinearToGamma(false);
			if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags)) { AddError(TEXT("Pixel readback failed")); return false; }
			for (FColor& Pixel : Pixels)
			{
				Pixel.R = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
				Pixel.G = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
				Pixel.B = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));
				Pixel.A = 255;
			}
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
			const TCHAR* CaptureName = bLicense ? TEXT("Licenses") : (CapturePage == 1 ? TEXT("AI") : TEXT("Credits"));
			const FString Path = Directory / FString::Printf(TEXT("%s_%s_%dx%d.png"), CaptureName, bKorean ? TEXT("KO") : TEXT("EN"), Size.X, Size.Y);
			TestTrue(TEXT("Capture saved"), FFileHelper::SaveArrayToFile(PNG, *Path));
			Reader->CloseUI();
		}
		}
	}
	return true;
}
#endif
