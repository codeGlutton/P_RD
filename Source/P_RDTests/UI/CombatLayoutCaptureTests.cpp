/*****************************************************************//**
 * @file   CombatLayoutCaptureTests.cpp
 * @brief  전투 HUD 배치안 WBP를 PNG로 뽑아 눈으로 비교할 수 있게 한다.
 * @details
 * 배치안은 열 개를 만들어 놓고 고르는 것이라, 열 장을 나란히 놓고 봐야
 * 판단이 된다. 에디터에서 하나씩 열어 보면 창 크기와 확대율이 매번 달라
 * 비교가 안 되므로, 같은 해상도로 오프스크린 렌더해 파일로 남긴다.
 *
 * 렌더 결과가 단색이면 실패로 처리한다. 위젯 수명이나 표시 상태가 깨지면
 * 빈 화면이 나오는데, 그것도 "성공한 캡처"처럼 보이기 때문이다.
 * @author 박용수
 * @date   2026-07-26
 *********************************************************************/

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Slate/WidgetRenderer.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/SOverlay.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_EDITOR

namespace CombatLayoutCapture
{
	/** @brief 잡아 볼 배치안 목록. WBP가 생기는 대로 여기에 줄을 늘린다. */
	const TCHAR* LayoutClassPaths[] = {
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_01_ClassicCRPG.WBP_CombatLayout_01_ClassicCRPG_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_02_LeftParty.WBP_CombatLayout_02_LeftParty_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_03_ActiveUnit.WBP_CombatLayout_03_ActiveUnit_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_04_Radial.WBP_CombatLayout_04_Radial_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_05_BottomBar.WBP_CombatLayout_05_BottomBar_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_06_Mirrored.WBP_CombatLayout_06_Mirrored_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_07_CardHand.WBP_CombatLayout_07_CardHand_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_08_Minimal.WBP_CombatLayout_08_Minimal_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_09_SplitBands.WBP_CombatLayout_09_SplitBands_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_10_Targeting.WBP_CombatLayout_10_Targeting_C"),
	};

	/** @brief 폰 가로 화면 실물 크기. 배치안 평가는 이 한 장이면 충분하다. */
	constexpr int32 CaptureWidth = 1920;
	constexpr int32 CaptureHeight = 1080;

	FString OutputDirectory()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UI"), TEXT("CombatLayouts"));
	}

	/**
	 * @brief 브러시가 쓰는 텍스처를 전부 상주시킨다.
	 *
	 * @details
	 * 오프스크린 렌더는 한 번에 끝나서 스트리밍을 기다려 주지 않는다. 큰
	 * 텍스처는 아직 안 올라온 채로 그려지고, 그 결과가 "프레임 조각(작은
	 * 텍스처)만 보이고 초상화·링·아이콘은 안 보이는" 화면이다. 인게임에서는
	 * 정상적으로 스트리밍되므로 이건 캡처 쪽 문제지 WBP 문제가 아니다.
	 *
	 * @return 상주시킨 텍스처 수. 0이면 브러시가 비어 있다는 뜻이다.
	 */
	int32 ResidentBrushTextures(UUserWidget& Widget)
	{
		if (Widget.WidgetTree == nullptr)
		{
			return 0;
		}

		TArray<UWidget*> Widgets;
		Widget.WidgetTree->GetAllWidgets(Widgets);
		int32 Count = 0;

		auto MakeResident = [&Count](const FSlateBrush& Brush)
		{
			UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject());
			if (Texture == nullptr)
			{
				return;
			}
			Texture->SetForceMipLevelsToBeResident(30.0f);
			Texture->WaitForStreaming();
			Texture->UpdateResource();
			++Count;
		};

		for (UWidget* Candidate : Widgets)
		{
			// Every widget type that can carry a texture, not just the two that
			// happened to matter first. The HP bar drew nothing for a whole
			// pass because its brushes live inside a style struct and this walk
			// only looked at Image and Border.
			if (const UImage* Image = Cast<UImage>(Candidate))
			{
				MakeResident(Image->GetBrush());
			}
			else if (const UBorder* Border = Cast<UBorder>(Candidate))
			{
				MakeResident(Border->Background);
			}
			else if (const UProgressBar* Bar = Cast<UProgressBar>(Candidate))
			{
				MakeResident(Bar->WidgetStyle.BackgroundImage);
				MakeResident(Bar->WidgetStyle.FillImage);
			}
			else if (const UButton* Button = Cast<UButton>(Candidate))
			{
				MakeResident(Button->WidgetStyle.Normal);
				MakeResident(Button->WidgetStyle.Hovered);
				MakeResident(Button->WidgetStyle.Pressed);
				MakeResident(Button->WidgetStyle.Disabled);
			}
		}
		FlushRenderingCommands();
		return Count;
	}

	/** @brief 배치안 하나를 렌더해서 PNG로 저장한다. 실패 사유는 OutError로. */
	bool CaptureLayout(UWorld& World, const TCHAR* ClassPath, FString& OutError)
	{
		UClass* LayoutClass = LoadClass<UCombatLayoutHUDWidget>(nullptr, ClassPath);
		if (LayoutClass == nullptr)
		{
			OutError = FString::Printf(TEXT("배치안 클래스를 못 찾음: %s"), ClassPath);
			return false;
		}

		UCombatLayoutHUDWidget* Layout =
			CreateWidget<UCombatLayoutHUDWidget>(&World, LayoutClass);
		if (Layout == nullptr)
		{
			OutError = FString::Printf(TEXT("위젯 생성 실패: %s"), ClassPath);
			return false;
		}

		// URDUserWidget은 OpenUI() 전까지 Collapsed다. 여기서는 뷰포트에 올리지
		// 않고 그리기만 하므로 표시 상태를 직접 세운다.
		Layout->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		const TSharedRef<SWidget> LayoutSlate = Layout->TakeWidget();
		Layout->ForceLayoutPrepass();

		const int32 TextureCount = ResidentBrushTextures(*Layout);
		if (TextureCount == 0)
		{
			OutError = TEXT("브러시에 텍스처가 하나도 없다. 아트가 안 붙었다");
			return false;
		}
		UE_LOG(LogTemp, Display, TEXT("[CombatLayout] %d textures resident"),
			TextureCount);

		// 전장이 뒤에 깔린다고 가정한 어두운 바탕. 완전한 검정에 대고 보면
		// 패널이 실제보다 잘 읽혀서 배치 판단이 후해진다.
		const TSharedRef<SWidget> CaptureRoot =
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SColorBlock).Color(FLinearColor(0.014f, 0.016f, 0.021f, 1.0f))
			]
			+ SOverlay::Slot()
			[
				LayoutSlate
			];

		// 리니어로 렌더하고, 파일로 쓸 때 딱 한 번 sRGB로 인코딩한다.
		//
		// 두 번 다 틀려 봤고 값이 그걸 말한다.
		//   렌더러 보정 끔 + 읽기 변환 끔 : 면 밝기 125가 48로 찍혔다.
		//     0.49^2.2 = 0.20 -> 51. 인코딩이 아예 없었다.
		//   렌더러 보정 켬 + 읽기 변환 끔 : 배경 리니어 0.012가 110으로 찍혔다.
		//     0.012를 두 번 인코딩하면 0.39 -> 100. 렌더 타깃이 이미 sRGB라
		//     셰이더 보정이 얹혀 두 번 먹었다.
		// 그래서 렌더는 리니어로 두고 읽기에서 한 번만 변환한다.
		FWidgetRenderer Renderer(false, true);
		Renderer.SetIsPrepassNeeded(true);
		UTextureRenderTarget2D* RenderTarget = Renderer.DrawWidget(
			CaptureRoot, FVector2D(CaptureWidth, CaptureHeight));
		if (RenderTarget == nullptr)
		{
			OutError = TEXT("렌더 타깃이 만들어지지 않음");
			return false;
		}

		FlushRenderingCommands();
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(true);
		if (!RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags)
			|| Pixels.Num() != CaptureWidth * CaptureHeight)
		{
			OutError = TEXT("렌더 결과를 읽지 못함");
			return false;
		}

		// 단색이면 위젯이 안 그려진 것이다. 그대로 저장하면 "배경만 나온 캡처"가
		// 성공처럼 남는다.
		uint8 MinChannel = 255;
		uint8 MaxChannel = 0;
		for (const FColor& Pixel : Pixels)
		{
			MinChannel = FMath::Min3(MinChannel, Pixel.R, FMath::Min(Pixel.G, Pixel.B));
			MaxChannel = FMath::Max3(MaxChannel, Pixel.R, FMath::Max(Pixel.G, Pixel.B));
		}
		if (int32(MaxChannel) - int32(MinChannel) < 8)
		{
			OutError = TEXT("캡처가 단색이다. 위젯이 그려지지 않았다");
			return false;
		}

		FString Stem = FString(ClassPath);
		Stem.Split(TEXT("."), nullptr, &Stem, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		Stem.RemoveFromEnd(TEXT("_C"));
		const FString OutputPath = FPaths::Combine(
			OutputDirectory(), FString::Printf(TEXT("%s.png"), *Stem));

		TArray64<uint8> PngData;
		FImageUtils::PNGCompressImageArray(CaptureWidth, CaptureHeight, Pixels, PngData);
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true);
		if (!FFileHelper::SaveArrayToFile(PngData, *OutputPath))
		{
			OutError = FString::Printf(TEXT("파일을 쓰지 못함: %s"), *OutputPath);
			return false;
		}
		UE_LOG(LogTemp, Display, TEXT("[CombatLayout] captured %s"), *OutputPath);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatLayoutCaptureTest,
	"P_RD.UI.CombatLayout.Capture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLayoutCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;

	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드가 있어야 위젯을 만들 수 있다"), World))
	{
		return false;
	}

	for (const TCHAR* ClassPath : LayoutClassPaths)
	{
		FString Error;
		if (!CaptureLayout(*World, ClassPath, Error))
		{
			AddError(FString::Printf(TEXT("%s: %s"), ClassPath, *Error));
		}
	}
	return true;
}

#endif // WITH_EDITOR
