#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Widgets/SWidget.h"
#include "UI/TitleMenuWidget.h"
#include "Components/TextBlock.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureCompiler.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTitleMenuRowButtonContractTest,
	"P_RD.UI.Title.MenuRowsClickable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTitleMenuRowButtonContractTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드"), World))
	{
		return false;
	}

	UClass* WidgetClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/UI/WBP_TitleMenu.WBP_TitleMenu_C"));
	if (!TestNotNull(TEXT("타이틀 WBP"), WidgetClass))
	{
		return false;
	}
	UUserWidget* Title = CreateWidget<UUserWidget>(World, WidgetClass);
	if (!TestNotNull(TEXT("타이틀 인스턴스"), Title))
	{
		return false;
	}
	const TSharedRef<SWidget> TitleSlate = Title->TakeWidget();

	for (const TCHAR* RowName : { TEXT("StartButton"), TEXT("ContinueButton"),
		TEXT("SettingsButton"), TEXT("ExitButton") })
	{
		UButton* Button = Cast<UButton>(Title->GetWidgetFromName(
			FName(*FString::Printf(TEXT("%s__base_16_9"), RowName))));
		if (Button == nullptr)
		{
			Button = Cast<UButton>(Title->GetWidgetFromName(FName(RowName)));
		}
		if (Button == nullptr)
		{
			AddError(FString::Printf(
				TEXT("Title: menu row '%s' has no button widget"), RowName));
			continue;
		}
		if (Button->OnClicked.IsBound() == false)
		{
			AddError(FString::Printf(
				TEXT("Title: menu row '%s' button is not bound"), RowName));
		}
	}

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleMenuResponsiveLayoutTest,
	"P_RD.UI.Title.FoldLayoutBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTitleMenuResponsiveLayoutTest::RunTest(const FString&)
{
	auto* World = GEditor->GetEditorWorldContext().World();
	auto* Class = LoadClass<UTitleMenuWidget>(nullptr, TEXT("/Game/UI/WBP_TitleMenu.WBP_TitleMenu_C"));
	if (!TestNotNull(TEXT("Actual title blueprint"), Class)) return false;
	auto* Title = CreateWidget<UTitleMenuWidget>(World, Class);
	const auto Slate = Title->TakeWidget();
	// Supply both save states explicitly without changing the user's actual run.
	Slate->SetCanTick(false);
	FTextureCompilingManager::Get().FinishAllCompilation();
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("UI/TitleFold");
	IFileManager::Get().MakeDirectory(*Dir, true);
	// Reuse one widget to exercise switching back from Fold to widescreen.
	for (const FVector2D Size : {FVector2D(2176,1812), FVector2D(1812,2176),
		FVector2D(1920,1080), FVector2D(2520,1080), FVector2D(2176,1812)})
	for (const bool bContinue : {false, true})
	{
		for (const TCHAR* Name : {TEXT("ContinueButton"), TEXT("ContinueButtonFrameImage"), TEXT("ContinueButtonText")})
		{
			auto* W = Title->GetWidgetFromName(FName(*FString::Printf(TEXT("%s__base_16_9"), Name)));
			if (W) W->SetVisibility(bContinue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
		FWidgetRenderer Renderer(true, true);
		for (int32 Pass=0; Pass<4; ++Pass)
		{
			Title->ApplyResponsiveLayoutForTest(Size, bContinue);
			Renderer.DrawWidget(Slate, Size);
		}
		auto* Target = Renderer.DrawWidget(Slate, Size);
		FlushRenderingCommands();
		for (const TCHAR* Name : {TEXT("TitleLogoImage"), TEXT("StartButton"), TEXT("ContinueButton"),
			TEXT("SettingsButton"), TEXT("ExitButton"), TEXT("VersionPlateImage"), TEXT("VersionText")})
		{
			if (!bContinue && FString(Name)==TEXT("ContinueButton")) continue;
			auto* Widget = Title->GetWidgetFromName(FName(*FString::Printf(TEXT("%s__base_16_9"), Name)));
			if (!TestNotNull(Name, Widget)) continue;
			const auto& G = Widget->GetCachedGeometry();
			const FVector2D Min = G.LocalToAbsolute(FVector2D::ZeroVector);
			const FVector2D Max = G.LocalToAbsolute(G.GetLocalSize());
			TestTrue(FString::Printf(TEXT("%s within %.0fx%.0f (%s to %s)"), Name, Size.X, Size.Y, *Min.ToString(), *Max.ToString()),
				Min.X>=0 && Min.Y>=0 && Max.X<=Size.X+1 && Max.Y<=Size.Y+1 && Max.X>Min.X && Max.Y>Min.Y);
			if (auto* Button = Cast<UButton>(Widget))
			{
				TestTrue(TEXT("Menu touch target remains usable"), Max.Y-Min.Y>=48);
				TestTrue(TEXT("Menu click binding preserved"), Button->OnClicked.IsBound());
			}
		}
		FBufferArchive Png;
		FImageUtils::ExportRenderTarget2DAsPNG(Target, Png);
		FFileHelper::SaveArrayToFile(Png, *(Dir / FString::Printf(TEXT("title-%.0fx%.0f-%s.png"), Size.X, Size.Y, bContinue ? TEXT("continue") : TEXT("new"))));
	}
	return !HasAnyErrors();
}
#endif
