/*****************************************************************//**
 * @file   SettingsPanelCaptureTests.cpp
 * @brief  설정 장부 WBP의 타이틀/인게임 상태를 고정 해상도 PNG로 남긴다.
 * @details
 * 디자이너 미리보기 대신 실제 USettingsPanelWidget 생명주기를 거쳐 그린다.
 * 따라서 모드별 버튼 표시, 선택 상태, 슬라이더 값과 최종 WBP 아트가 함께
 * 검증된다. 결과는 Saved/UI/Settings 아래에 1672x941 PNG로 저장한다.
 * @date   2026-08-12
 *********************************************************************/

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
#include "Internationalization/Internationalization.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/SlateTypes.h"
#include "UI/SettingsPanelWidget.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/SOverlay.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_EDITOR

namespace SettingsPanelCapture
{
	constexpr int32 LandscapeCaptureWidth = 1672;
	constexpr int32 LandscapeCaptureHeight = 941;
	constexpr TCHAR SettingsClassPath[] =
		TEXT("/Game/UI/WBP_SettingsPanel.WBP_SettingsPanel_C");

	FString OutputDirectory()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UI"), TEXT("Settings"));
	}

	/** @brief 위젯 트리의 모든 브러시 텍스처를 첫 오프스크린 draw 전에 준비한다. */
	int32 MakeBrushTexturesResident(USettingsPanelWidget& Panel)
	{
		if (Panel.WidgetTree == nullptr)
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

		Panel.WidgetTree->ForEachWidget(
			[&CollectBrush, &CollectStyle](UWidget* Widget)
			{
				if (const UImage* Image = Cast<UImage>(Widget))
				{
					CollectBrush(Image->GetBrush());
				}
				else if (const UBorder* Border = Cast<UBorder>(Widget))
				{
					CollectBrush(Border->Background);
				}
				else if (const UButton* Button = Cast<UButton>(Widget))
				{
					CollectStyle(Button->GetStyle());
				}
				else if (const UProgressBar* Progress = Cast<UProgressBar>(Widget))
				{
					CollectStyle(Progress->GetWidgetStyle());
				}
				else if (const UCheckBox* CheckBox = Cast<UCheckBox>(Widget))
				{
					CollectStyle(CheckBox->GetWidgetStyle());
				}
				else if (const USlider* Slider = Cast<USlider>(Widget))
				{
					CollectStyle(Slider->GetWidgetStyle());
				}
			});

		for (UTexture2D* Texture : Textures)
		{
			Texture->UpdateResource();
			Texture->SetForceMipLevelsToBeResident(30.0f);
			Texture->WaitForStreaming();
		}
		FlushRenderingCommands();
		return Textures.Num();
	}

	/** @brief 사람이 비교하기 좋은 고정값으로 각 컨트롤의 상태를 맞춘다. */
	void ApplyCaptureValues(USettingsPanelWidget& Panel)
	{
		FSettingsPanelValueModel Values;
		Values.mMasterVolume = 0.82f;
		Values.mBgmVolume = 0.64f;
		Values.mSfxVolume = 0.48f;
		Values.mUiVolume = 0.72f;
		Values.mScreenShakeEnabled = true;
		Values.mVibrationEnabled = false;
		Values.mEffectsEnabled = true;
		Values.mQualityLevel = ESettingsQualityLevel::Medium;
		Values.mFpsLimit = 60;
		Values.mUseKoreanLanguage = true;
		Panel.ApplyValueModel(Values);
		Panel.SetStatusText(FText::GetEmpty());
		Panel.HideAbandonConfirm();
	}

	bool CaptureMode(UWorld& World, const ESettingsPanelMode Mode,
		const TCHAR* ModeName, FString& OutError,
		const bool bShowAbandonConfirm = false,
		const bool bShowSaveAndExitConfirm = false,
		const int32 CaptureWidth = LandscapeCaptureWidth,
		const int32 CaptureHeight = LandscapeCaptureHeight)
	{
		UClass* SettingsClass = LoadClass<USettingsPanelWidget>(nullptr, SettingsClassPath);
		if (SettingsClass == nullptr)
		{
			OutError = FString::Printf(TEXT("설정 WBP 클래스를 못 찾음: %s"),
				SettingsClassPath);
			return false;
		}

		USettingsPanelWidget* Panel = CreateWidget<USettingsPanelWidget>(&World, SettingsClass);
		if (Panel == nullptr || Panel->WidgetTree == nullptr)
		{
			OutError = TEXT("설정 WBP 인스턴스/WidgetTree 생성 실패");
			return false;
		}

		// TakeWidget에서 NativeConstruct가 끝난 뒤 캡처 상태를 덮어써야 한다.
		const TSharedRef<SWidget> PanelSlate = Panel->TakeWidget();
		Panel->SetPanelMode(Mode);
		Panel->RefreshPanelState(Mode == ESettingsPanelMode::InGame,
			Mode == ESettingsPanelMode::InGame);
		ApplyCaptureValues(*Panel);
		if (bShowAbandonConfirm)
		{
			Panel->ShowAbandonConfirm();
		}
		else if (bShowSaveAndExitConfirm)
		{
			Panel->ShowSaveAndExitConfirm();
		}
		Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		UWidget* RunActions = Panel->WidgetTree->FindWidget(TEXT("RunActionsPanel"));
		if (RunActions == nullptr)
		{
			OutError = TEXT("RunActionsPanel 바인딩이 없어 모드별 화면을 검증할 수 없음");
			return false;
		}
		const ESlateVisibility ExpectedRunActions = Mode == ESettingsPanelMode::InGame
			? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
		if (RunActions->GetVisibility() != ExpectedRunActions)
		{
			OutError = FString::Printf(
				TEXT("%s 모드의 RunActionsPanel 가시성이 반대임"), ModeName);
			return false;
		}
		UWidget* AbandonConfirm = Panel->WidgetTree->FindWidget(
			TEXT("AbandonConfirmPanel"));
		if (AbandonConfirm == nullptr)
		{
			OutError = TEXT("AbandonConfirmPanel 바인딩이 없어 확인창을 검증할 수 없음");
			return false;
		}
		const ESlateVisibility ExpectedConfirmVisibility =
			(bShowAbandonConfirm || bShowSaveAndExitConfirm)
			? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
		if (AbandonConfirm->GetVisibility() != ExpectedConfirmVisibility)
		{
			OutError = FString::Printf(
				TEXT("%s 캡처의 AbandonConfirmPanel 가시성이 반대임"), ModeName);
			return false;
		}

		Panel->ForceLayoutPrepass();
		const int32 TextureCount = MakeBrushTexturesResident(*Panel);
		if (TextureCount == 0)
		{
			OutError = TEXT("설정 WBP 브러시에 Texture2D가 하나도 없음");
			return false;
		}

		const TSharedRef<SWidget> CaptureRoot =
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SColorBlock).Color(FLinearColor(0.008f, 0.009f, 0.018f, 1.0f))
			]
			+ SOverlay::Slot()
			[
				PanelSlate
			];

		FWidgetRenderer Renderer(true, true);
		Renderer.SetIsPrepassNeeded(true);
		UTextureRenderTarget2D* RenderTarget = Renderer.DrawWidget(
			CaptureRoot, FVector2D(CaptureWidth, CaptureHeight));
		if (RenderTarget == nullptr)
		{
			OutError = TEXT("FWidgetRenderer가 렌더 타깃을 만들지 못함");
			return false;
		}

		FlushRenderingCommands();
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(false);
		if (!RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags)
			|| Pixels.Num() != CaptureWidth * CaptureHeight)
		{
			OutError = FString::Printf(TEXT("%dx%d 렌더 결과를 읽지 못함"),
				CaptureWidth, CaptureHeight);
			return false;
		}

		// CombatLayout 캡처와 같은 RGBA8 경로는 감마가 두 번 들어가므로 한 번 되돌린다.
		uint8 MinChannel = 255;
		uint8 MaxChannel = 0;
		for (FColor& Pixel : Pixels)
		{
			Pixel.R = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
			Pixel.G = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
			Pixel.B = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));
			MinChannel = FMath::Min3(MinChannel, Pixel.R,
				FMath::Min(Pixel.G, Pixel.B));
			MaxChannel = FMath::Max3(MaxChannel, Pixel.R,
				FMath::Max(Pixel.G, Pixel.B));
		}
		if (int32(MaxChannel) - int32(MinChannel) < 16)
		{
			OutError = TEXT("캡처가 단색이다. 설정 WBP가 그려지지 않음");
			return false;
		}

		TArray64<uint8> PngData;
		FImageUtils::PNGCompressImageArray(
			CaptureWidth, CaptureHeight, Pixels, PngData);
		const FString OutputPath = FPaths::Combine(OutputDirectory(),
			FString::Printf(TEXT("WBP_SettingsPanel_%s.png"), ModeName));
		IFileManager::Get().MakeDirectory(*OutputDirectory(), true);
		if (!FFileHelper::SaveArrayToFile(PngData, *OutputPath))
		{
			OutError = FString::Printf(TEXT("캡처 파일 저장 실패: %s"), *OutputPath);
			return false;
		}

		UE_LOG(LogTemp, Display,
			TEXT("[SettingsCapture] %s (%d textures) -> %s"),
			ModeName, TextureCount, *OutputPath);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSettingsPanelCaptureTest,
	"P_RD.UI.Settings.Capture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSettingsPanelCaptureTest::RunTest(const FString& Parameters)
{
	using namespace SettingsPanelCapture;

	if (GUsingNullRHI == true)
	{
		AddInfo(TEXT("NullRHI 환경이라 설정 WBP 캡처 생략"));
		return true;
	}

	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드가 있어야 설정 WBP를 만들 수 있다"), World))
	{
		return false;
	}
	const FString OriginalCulture =
		FInternationalization::Get().GetCurrentCulture()->GetName();
	FInternationalization::Get().SetCurrentCulture(TEXT("ko"));

	for (const TPair<ESettingsPanelMode, const TCHAR*> Mode : {
		TPair<ESettingsPanelMode, const TCHAR*>(ESettingsPanelMode::Title, TEXT("Title")),
		TPair<ESettingsPanelMode, const TCHAR*>(ESettingsPanelMode::InGame, TEXT("InGame")) })
	{
		FString Error;
		if (!CaptureMode(*World, Mode.Key, Mode.Value, Error))
		{
			AddError(FString::Printf(TEXT("%s: %s"), Mode.Value, *Error));
		}
	}
	FString AbandonConfirmError;
	if (!CaptureMode(*World, ESettingsPanelMode::InGame,
		TEXT("InGame_AbandonConfirm"), AbandonConfirmError, true))
	{
		AddError(FString::Printf(TEXT("InGame_AbandonConfirm: %s"),
			*AbandonConfirmError));
	}
	FString SaveAndExitConfirmError;
	if (!CaptureMode(*World, ESettingsPanelMode::InGame,
		TEXT("InGame_SaveAndExitConfirm"), SaveAndExitConfirmError, false, true))
	{
		AddError(FString::Printf(TEXT("InGame_SaveAndExitConfirm: %s"),
			*SaveAndExitConfirmError));
	}
	// Galaxy Fold capture from the report: the viewport-space dim must cover the
	// full 2176x1812 canvas while the dialog itself remains aspect-fitted.
	FString FoldAbandonConfirmError;
	if (!CaptureMode(*World, ESettingsPanelMode::InGame,
		TEXT("Fold2176x1812_AbandonConfirm"), FoldAbandonConfirmError,
		true, false, 2176, 1812))
	{
		AddError(FString::Printf(TEXT("Fold2176x1812_AbandonConfirm: %s"),
			*FoldAbandonConfirmError));
	}
	FString FoldSaveConfirmError;
	if (!CaptureMode(*World, ESettingsPanelMode::InGame,
		TEXT("Fold2176x1812_SaveAndExitConfirm"), FoldSaveConfirmError,
		false, true, 2176, 1812))
	{
		AddError(FString::Printf(TEXT("Fold2176x1812_SaveAndExitConfirm: %s"),
			*FoldSaveConfirmError));
	}
	for (const TCHAR* FoldCaptureName : {
		TEXT("WBP_SettingsPanel_Fold2176x1812_AbandonConfirm.png"),
		TEXT("WBP_SettingsPanel_Fold2176x1812_SaveAndExitConfirm.png") })
	{
		const FString FoldCapturePath = FPaths::Combine(
			OutputDirectory(), FoldCaptureName);
		TestTrue(*FString::Printf(TEXT("%s 생성됨"), FoldCaptureName),
			IFileManager::Get().FileExists(*FoldCapturePath));
		TestTrue(*FString::Printf(TEXT("%s 는 빈 파일이 아님"), FoldCaptureName),
			IFileManager::Get().FileSize(*FoldCapturePath) > 100000);
	}
	FInternationalization::Get().SetCurrentCulture(OriginalCulture);
	return true;
}

#endif // WITH_EDITOR
