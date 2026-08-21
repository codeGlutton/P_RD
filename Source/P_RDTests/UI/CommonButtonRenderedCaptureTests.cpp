#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/SOverlay.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Editor.h"

namespace CommonButtonRenderedCapture
{
	constexpr int32 CaptureWidth = 1672;
	constexpr int32 CaptureHeight = 941;

	struct FCaptureScenario
	{
		const TCHAR* Name;
		const TCHAR* ClassPath;
	};

	constexpr FCaptureScenario Scenarios[] =
	{
		{ TEXT("TitleMenu"), TEXT("/Game/UI/WBP_TitleMenu.WBP_TitleMenu_C") },
		{ TEXT("FrontendMapLandscape"), TEXT("/Game/UI/WorldMapLandscape/WBP_FrontendMapLandscape.WBP_FrontendMapLandscape_C") },
		{ TEXT("CombatDefeat"), TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C") },
		{ TEXT("RewardSettlement"), TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C") },
	};

	FString OutputDirectory()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UI"),
			TEXT("ButtonContractScreens"));
	}

	int32 MakeTexturesResident(UUserWidget& Widget)
	{
		if (Widget.WidgetTree == nullptr)
		{
			return 0;
		}

		TSet<UTexture2D*> Textures;
		auto CollectBrush = [&Textures](const FSlateBrush& Brush)
		{
			if (UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject()))
			{
				Textures.Add(Texture);
			}
		};
		auto CollectStyle = [&CollectBrush](const FSlateWidgetStyle& Style)
		{
			TArray<const FSlateBrush*> Brushes;
			Style.GetResources(Brushes);
			for (const FSlateBrush* Brush : Brushes)
			{
				if (Brush != nullptr)
				{
					CollectBrush(*Brush);
				}
			}
		};

		Widget.WidgetTree->ForEachWidgetAndDescendants(
			[&CollectBrush, &CollectStyle](UWidget* Child)
			{
				if (const UImage* Image = Cast<UImage>(Child))
				{
					CollectBrush(Image->GetBrush());
				}
				else if (const UBorder* Border = Cast<UBorder>(Child))
				{
					CollectBrush(Border->Background);
				}
				else if (const UButton* Button = Cast<UButton>(Child))
				{
					CollectStyle(Button->GetStyle());
				}
				else if (const UProgressBar* Progress = Cast<UProgressBar>(Child))
				{
					CollectStyle(Progress->GetWidgetStyle());
				}
				else if (const UCheckBox* CheckBox = Cast<UCheckBox>(Child))
				{
					CollectStyle(CheckBox->GetWidgetStyle());
				}
				else if (const USlider* Slider = Cast<USlider>(Child))
				{
					CollectStyle(Slider->GetWidgetStyle());
				}
			});

		for (UTexture2D* Texture : Textures)
		{
			Texture->UpdateResource();
			Texture->SetForceMipLevelsToBeResident(30.f);
			Texture->WaitForStreaming();
		}
		FlushRenderingCommands();
		return Textures.Num();
	}

	bool CaptureScenario(UWorld& World, const FCaptureScenario& Scenario,
		FString& OutError)
	{
		UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, Scenario.ClassPath);
		if (WidgetClass == nullptr)
		{
			OutError = FString::Printf(TEXT("WBP 클래스를 못 찾음: %s"),
				Scenario.ClassPath);
			return false;
		}
		UUserWidget* Widget = CreateWidget<UUserWidget>(&World, WidgetClass);
		if (Widget == nullptr || Widget->WidgetTree == nullptr)
		{
			OutError = FString::Printf(TEXT("%s 인스턴스 생성 실패"), Scenario.Name);
			return false;
		}

		const TSharedRef<SWidget> WidgetSlate = Widget->TakeWidget();
		Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Widget->ForceLayoutPrepass();
		const int32 TextureCount = MakeTexturesResident(*Widget);
		if (TextureCount == 0)
		{
			OutError = FString::Printf(TEXT("%s 브러시 텍스처가 없음"), Scenario.Name);
			return false;
		}

		const TSharedRef<SWidget> CaptureRoot =
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SColorBlock).Color(FLinearColor(.008f, .01f, .014f, 1.f))
			]
			+ SOverlay::Slot()
			[
				WidgetSlate
			];

		FWidgetRenderer Renderer(true, true);
		Renderer.SetIsPrepassNeeded(true);
		for (int32 Warmup = 0; Warmup < 4; ++Warmup)
		{
			Renderer.DrawWidget(CaptureRoot,
				FVector2D(CaptureWidth, CaptureHeight));
			FlushRenderingCommands();
		}
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(
			CaptureRoot, FVector2D(CaptureWidth, CaptureHeight));
		if (Target == nullptr)
		{
			OutError = FString::Printf(TEXT("%s 렌더 타깃 생성 실패"), Scenario.Name);
			return false;
		}

		FlushRenderingCommands();
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags Flags(RCM_UNorm);
		Flags.SetLinearToGamma(false);
		if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags)
			|| Pixels.Num() != CaptureWidth * CaptureHeight)
		{
			OutError = FString::Printf(TEXT("%s 픽셀 읽기 실패"), Scenario.Name);
			return false;
		}

		int32 ChangedPixels = 0;
		const FColor First = Pixels[0];
		for (FColor& Pixel : Pixels)
		{
			Pixel.R = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
			Pixel.G = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
			Pixel.B = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));
			if (FMath::Abs(int32(Pixel.R) - int32(First.R))
				+ FMath::Abs(int32(Pixel.G) - int32(First.G))
				+ FMath::Abs(int32(Pixel.B) - int32(First.B)) > 12)
			{
				++ChangedPixels;
			}
		}
		if (ChangedPixels < CaptureWidth * CaptureHeight / 100)
		{
			OutError = FString::Printf(TEXT("%s 캡처가 비어 있음"), Scenario.Name);
			return false;
		}

		TArray64<uint8> PngData;
		FImageUtils::PNGCompressImageArray(
			CaptureWidth, CaptureHeight, Pixels, PngData);
		IFileManager::Get().MakeDirectory(*OutputDirectory(), true);
		const FString OutputPath = FPaths::Combine(OutputDirectory(),
			FString::Printf(TEXT("WBP_%s.png"), Scenario.Name));
		if (!FFileHelper::SaveArrayToFile(PngData, *OutputPath))
		{
			OutError = FString::Printf(TEXT("%s 저장 실패"), *OutputPath);
			return false;
		}
		UE_LOG(LogTemp, Display, TEXT("[ButtonContractCapture] %s (%d textures) -> %s"),
			Scenario.Name, TextureCount, *OutputPath);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommonButtonRenderedCaptureTest,
	"P_RD.UI.Common.RenderedButtonScreens",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommonButtonRenderedCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CommonButtonRenderedCapture;
	if (GUsingNullRHI)
	{
		AddInfo(TEXT("NullRHI 환경이라 공통 버튼 화면 캡처 생략"));
		return true;
	}
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("공통 버튼 캡처 에디터 월드"), World))
	{
		return false;
	}

	const FString OriginalCulture =
		FInternationalization::Get().GetCurrentCulture()->GetName();
	FInternationalization::Get().SetCurrentCulture(TEXT("ko"));
	for (const FCaptureScenario& Scenario : Scenarios)
	{
		FString Error;
		if (!CaptureScenario(*World, Scenario, Error))
		{
			AddError(Error);
		}
	}
	FInternationalization::Get().SetCurrentCulture(OriginalCulture);
	return !HasAnyErrors();
}

#endif
