/*****************************************************************//**
 * @file   CombatLayoutCaptureTests.cpp
 * @brief  전투 HUD 배치안 WBP를 PNG로 뽑아 눈으로 비교할 수 있게 한다.
 * @details
 * 처음에는 배치안 스무 갈래를 나란히 놓고 고르려고 만들었다. 고르고 난
 * 지금은 **구운 WBP 가 뜻대로 나왔나 보는 눈**으로 쓴다. 에디터에서 열어
 * 보면 창 크기와 확대율이 매번 달라 비교가 안 되므로, 같은 해상도로
 * 오프스크린 렌더해 파일로 남긴다.
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
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_EDITOR

namespace CombatLayoutCapture
{
	/**
	 * @brief 이 위젯이 쓰는 텍스처를 다 올려 놓고 기다린다.
	 *
	 * @details
	 * 오프스크린으로 한 번 그리고 끝내므로, 스트리밍이 다음 프레임에 올릴
	 * 셈이면 영영 안 올라온다. 밉을 붙잡아 두고 스트리밍을 한 번 돌린다.
	 * @param Root 훑을 위젯
	 */
	void ForceTexturesResident(UWidget* Root)
	{
		UUserWidget* User = Cast<UUserWidget>(Root);
		if (User == nullptr || User->WidgetTree == nullptr)
		{
			return;
		}
		User->WidgetTree->ForEachWidget([](UWidget* Widget)
		{
			UImage* Image = Cast<UImage>(Widget);
			UTexture2D* Texture = Image != nullptr
				? Cast<UTexture2D>(Image->GetBrush().GetResourceObject()) : nullptr;
			if (Texture != nullptr)
			{
				// 밉을 붙잡아 두고 **그 자리에서 기다린다.** 스트리밍 관리자를
				// 통째로 돌리면 오히려 지금 올라와 있던 것까지 내려간다 --
				// 그렇게 해 봤더니 판이 하나도 안 남았다.
				Texture->bForceMiplevelsToBeResident = true;
				Texture->SetForceMipLevelsToBeResident(30.f);
				Texture->WaitForStreaming();
			}
		});
		FlushRenderingCommands();
	}

	/**
	 * @brief 찍을 배치. 시안4로 정해져서 하나뿐이다.
	 *
	 * @details
	 * 스무 갈래를 만들어 놓고 고르던 때에는 여기에 스무 줄이 있었다. 고르고
	 * 나서 열아홉을 지웠는데, **찍는 기계는 남겼다** -- 구운 WBP 가 뜻대로
	 * 나왔는지 눈으로 볼 길이 이것뿐이다. 코드를 읽고 "맞다" 고 말하는 것과
	 * 나온 그림이 그런 것은 다른 이야기다.
	 */
	const TCHAR* LayoutClassPaths[] = {
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"),
	};

	/** @brief 폰 가로 화면 실물 크기. 배치안 평가는 이 한 장이면 충분하다. */
	//: 시안 원본과 같은 크기로 찍는다.
	//:
	//: 배치는 1920x1080 캔버스에 짜여 있고, 인게임에서는 UI 스케일 규칙이
	//: 짧은변 941 을 만나 0.871 로 줄여 그린다. 캡처도 그 배율을 그대로 걸어야
	//: 시안(1672x941)과 픽셀이 1:1 로 맞는다. 1920 으로 찍고 시안을 확대해
	//: 비교하면 비율만 맞고 크기는 안 맞아, 요소 단위로 보면 어긋난다.
	constexpr int32 DesignWidth = 1920;
	constexpr int32 DesignHeight = 1080;
	constexpr int32 CaptureWidth = 1672;
	constexpr int32 CaptureHeight = 941;
	constexpr float CaptureScale = float(CaptureHeight) / float(DesignHeight);

	/** @brief 1920 캔버스를 1672 로 줄여 그리도록 감싼다. 인게임과 같은 배율. */
	TSharedRef<SWidget> ScaleToCapture(const TSharedRef<SWidget>& Inner)
	{
		return SNew(SScaleBox)
			.Stretch(EStretch::UserSpecified)
			.UserSpecifiedScale(CaptureScale)
			[
				SNew(SBox)
				.WidthOverride(float(DesignWidth))
				.HeightOverride(float(DesignHeight))
				[
					Inner
				]
			];
	}

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
			// 순서가 중요하다. UpdateResource() 가 그릴 자원을 만드는
			// 호출이고, 기다리기는 그 다음이다. 자원이 없는 텍스처는 아무리
			// 기다려도 안 올라온다 -- 한 번도 안 그려 본 텍스처가 그렇다.
			//
			// 이 줄을 지웠다가 열아홉 장이 통째로 비었다. 지우기 전에는
			// 기다린 뒤에 불러서, 방금 기다린 것이 무효가 되는 반대 문제가
			// 있었다. 빼는 것이 아니라 앞으로 옮기는 것이 답이다.
			Texture->UpdateResource();
			Texture->SetForceMipLevelsToBeResident(30.0f);
			Texture->WaitForStreaming();
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
				MakeResident(Bar->GetWidgetStyle().BackgroundImage);
				MakeResident(Bar->GetWidgetStyle().FillImage);
			}
			else if (const UButton* Button = Cast<UButton>(Candidate))
			{
				MakeResident(Button->GetStyle().Normal);
				MakeResident(Button->GetStyle().Hovered);
				MakeResident(Button->GetStyle().Pressed);
				MakeResident(Button->GetStyle().Disabled);
			}
		}
		FlushRenderingCommands();
		return Count;
	}

	/**
	 * @brief 배치안 하나를 렌더해서 PNG로 저장한다. 실패 사유는 OutError로.
	 * @param bShowMercenaryPanel 참이면 전투 기본 HUD 대신 보유 용병 탭을 찍는다.
	 */
	bool CaptureLayout(UWorld& World, const TCHAR* ClassPath, FString& OutError,
		const bool bShowMercenaryPanel = false)
	{
		UClass* LayoutClass = LoadClass<UUserWidget>(nullptr, ClassPath);
		if (LayoutClass == nullptr)
		{
			OutError = FString::Printf(TEXT("배치안 클래스를 못 찾음: %s"), ClassPath);
			return false;
		}

		// 공통 부모로 만든다. 캡처는 그려서 찍는 일이라 화면이 전투 배치안인지
		// 고용 게시판인지 알 필요가 없다.
		UUserWidget* Layout = CreateWidget<UUserWidget>(&World, LayoutClass);
		if (Layout == nullptr)
		{
			OutError = FString::Printf(TEXT("위젯 생성 실패: %s"), ClassPath);
			return false;
		}

		// URDUserWidget은 OpenUI() 전까지 Collapsed다. 여기서는 뷰포트에 올리지
		// 않고 그리기만 하므로 표시 상태를 직접 세운다.
		Layout->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		const TSharedRef<SWidget> LayoutSlate = Layout->TakeWidget();
		// TakeWidget()에서 NativeConstruct가 돌며 패널을 기본 Collapsed로
		// 되돌린다. 변형 상태는 Construct가 끝난 뒤 세워야 캡처에 남는다.
		if (bShowMercenaryPanel && Layout->WidgetTree != nullptr)
		{
			if (UWidget* MercenaryPanel =
				Layout->WidgetTree->FindWidget(TEXT("MercenaryPanel")))
			{
				MercenaryPanel->SetVisibility(ESlateVisibility::Visible);
			}
		}
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
		// 기지값 색 띠. 리니어 {0, 0.05, 0.2158, 1.0}은 감마 인코딩을 정확히
		// 한 번 거치면 sRGB {0, 65, 128, 255}가 된다. 다르게 읽히면 파이프라인
		// 어딘가에서 변환이 빠졌거나 두 번 들어간 것이다.
		const TSharedRef<SWidget> CaptureRoot =
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SColorBlock).Color(FLinearColor(0.008f, 0.009f, 0.011f, 1.0f))
			]
			+ SOverlay::Slot()
			[
				ScaleToCapture(LayoutSlate)
			]
			// 색 띠는 배치 위에 그린다.
			//
			// 아래에 깔았더니 라운드 판을 화면 맨 위까지 올린 순간 띠가 가려져
			// 열 장이 통째로 저장을 거부당했다. 검증용 표식이 검증 대상에
			// 가려지면 안 된다. 읽고 나서 지우므로 결과 그림에는 안 남는다.
			+ SOverlay::Slot()
			.HAlign(HAlign_Left).VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[ SNew(SBox).WidthOverride(40).HeightOverride(10)
					[ SNew(SColorBlock).Color(FLinearColor(0.f, 0.f, 0.f)) ] ]
				+ SHorizontalBox::Slot().AutoWidth()
				[ SNew(SBox).WidthOverride(40).HeightOverride(10)
					[ SNew(SColorBlock).Color(FLinearColor(0.05f, 0.05f, 0.05f)) ] ]
				+ SHorizontalBox::Slot().AutoWidth()
				[ SNew(SBox).WidthOverride(40).HeightOverride(10)
					[ SNew(SColorBlock).Color(FLinearColor(0.2158f, 0.2158f, 0.2158f)) ] ]
				+ SHorizontalBox::Slot().AutoWidth()
				[ SNew(SBox).WidthOverride(40).HeightOverride(10)
					[ SNew(SColorBlock).Color(FLinearColor(1.f, 1.f, 1.f)) ] ]
			];

		// 렌더러가 감마 공간에 직접 그린다.
		//
		// 세 번 틀리고 내린 결론: RGBA8 타깃에서는 ReadPixels의 LinearToGamma
		// 플래그가 적용되지 않는다. 변환을 읽기 단계에 미루면 리니어 값이
		// 바이트로 그대로 나가 절반쯤 어두운 그림이 남는다 -- 면 텍스처 111이
		// 48로 찍힌 원인. 그래서 변환은 렌더러가 하고, 읽기는 그대로 옮긴다.
		//
		// 예전에 이 조합을 "씻긴다"며 버렸는데, 그때 씻겨 보인 건 배경 색을
		// sRGB 감각으로 적어 놓고 리니어로 해석시킨 탓이었다. 파이프라인이
		// 아니라 배경 값이 문제였다.
		//
		// 그리고 이번부터 캡처가 스스로 증명한다: 아래에서 기지값 색 띠를
		// 같이 그려 읽은 값이 기대값과 다르면 캡처 자체를 실패로 처리한다.
		// 판 그림을 먼저 다 올려 놓는다.
		//
		// 스트리밍이 아직 안 올린 텍스처는 그리는 순간 빈 자리로 나온다. 매번
		// **다른 판**이 빠져서, 굽는 쪽이 깨진 것처럼 보였다 -- 한 번은 위쪽
		// 세 장이, 다음 번에는 두루마리 한 장이 없었다. 찍는 눈이 흔들리면
		// 없는 문제를 쫓게 된다.
		ForceTexturesResident(Layout);

		FWidgetRenderer Renderer(true, true);
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
		ReadFlags.SetLinearToGamma(false);
		if (!RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags)
			|| Pixels.Num() != CaptureWidth * CaptureHeight)
		{
			OutError = TEXT("렌더 결과를 읽지 못함");
			return false;
		}

		// 이 경로는 인코딩이 두 번 걸린다. 색 띠 실측: 리니어 0.05가 136으로
		// 읽혔는데 0.05를 두 번 인코딩하면 정확히 137이다. 그래서 저장 전에
		// 한 번 되돌린다 -- 결과는 정확히 한 번 인코딩된 sRGB가 되고, 아래
		// 검증이 그걸 확인한다. 엔진 플래그 조합을 더 뒤지는 것보다 측정값에
		// 맞춘 보정 한 줄이 낫고, 틀리면 검증이 잡는다.
		for (FColor& Pixel : Pixels)
		{
			Pixel.R = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
			Pixel.G = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
			Pixel.B = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));
		}

		// 색 띠 검증: 위 보정까지 거친 결과가 정확히 한 번 인코딩이라면
		// 이 값이 나와야 한다.
		{
			const int32 Expected[4] = { 0, 65, 128, 255 };
			const int32 SampleX[4] = { 20, 60, 100, 140 };
			for (int32 Step = 0; Step < 4; ++Step)
			{
				const FColor& Pixel = Pixels[5 * CaptureWidth + SampleX[Step]];
				if (FMath::Abs(int32(Pixel.R) - Expected[Step]) > 6)
				{
					OutError = FString::Printf(
						TEXT("감마 검증 실패: 띠 %d칸이 %d로 읽힘 (기대 %d). ")
						TEXT("이 캡처의 색은 믿을 수 없다"),
						Step, Pixel.R, Expected[Step]);
					return false;
				}
			}
			// 통과했으면 띠를 배경색으로 지워 그림을 깨끗하게 남긴다.
			const FColor Background = Pixels[30 * CaptureWidth + 400];
			for (int32 Y = 0; Y < 12; ++Y)
			{
				for (int32 X = 0; X < 170; ++X)
				{
					Pixels[Y * CaptureWidth + X] = Background;
				}
			}
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
			OutputDirectory(), FString::Printf(TEXT("%s%s.png"), *Stem,
				bShowMercenaryPanel ? TEXT("_Mercenaries") : TEXT("")));

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

namespace CombatLayoutCapture
{
	/**
	 * @brief 요소 하나만 남기고 전부 접은 뒤 그 자리를 잘라 저장한다.
	 *
	 * @details
	 * 전체 화면을 찍어 잘라내는 것과 다르다. 잘라내면 이웃 부품과 뒤판이 같이
	 * 들어와 그 요소가 실제로 어떻게 생겼는지 안 보인다. 여기서는 대상과 그
	 * 조상만 남기고 나머지를 접은 뒤 그린다 -- 요소만 홀로 남는다.
	 *
	 * 조상을 남기는 이유는 부모를 접으면 자식도 같이 사라지기 때문이다.
	 * 자리는 캐시된 기하에서 읽는다. 이 키트는 거의 전부 캔버스 패널이라
	 * 형제를 접어도 절대 좌표가 흔들리지 않는다.
	 */
	bool CaptureElements(UWorld& World, const TCHAR* ClassPath, FString& OutError)
	{
		UClass* LayoutClass = LoadClass<UUserWidget>(nullptr, ClassPath);
		if (LayoutClass == nullptr)
		{
			OutError = FString::Printf(TEXT("클래스를 못 찾음: %s"), ClassPath);
			return false;
		}
		// 공통 부모로 만든다. 캡처는 그려서 찍는 일이라 화면이 전투 배치안인지
		// 고용 게시판인지 알 필요가 없다.
		UUserWidget* Layout = CreateWidget<UUserWidget>(&World, LayoutClass);
		if (Layout == nullptr || Layout->WidgetTree == nullptr)
		{
			OutError = TEXT("위젯 생성 실패");
			return false;
		}
		Layout->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		const TSharedRef<SWidget> LayoutSlate = Layout->TakeWidget();
		Layout->ForceLayoutPrepass();
		ResidentBrushTextures(*Layout);

		TArray<UWidget*> All;
		Layout->WidgetTree->GetAllWidgets(All);

		FString Stem = FString(ClassPath);
		Stem.Split(TEXT("."), nullptr, &Stem, ESearchCase::CaseSensitive,
			ESearchDir::FromEnd);
		Stem.RemoveFromEnd(TEXT("_C"));
		const FString Dir = FPaths::Combine(OutputDirectory(), TEXT("Elements"), Stem);
		IFileManager::Get().DeleteDirectory(*Dir, false, true);
		IFileManager::Get().MakeDirectory(*Dir, true);

		// 한 번 그려서 기하를 채운다. 그리기 전에는 캐시가 비어 있다.
		FWidgetRenderer Probe(true, true);
		Probe.SetIsPrepassNeeded(true);
		const TSharedRef<SWidget> Scaled = ScaleToCapture(LayoutSlate);
		Probe.DrawWidget(Scaled, FVector2D(CaptureWidth, CaptureHeight));
		FlushRenderingCommands();

		TMap<UWidget*, ESlateVisibility> Original;
		for (UWidget* Widget : All)
		{
			Original.Add(Widget, Widget->GetVisibility());
		}

		int32 Saved = 0;
		for (UWidget* Target : All)
		{
			if (Target == nullptr || Target->GetName().StartsWith(TEXT("__")))
			{
				continue;
			}
			// 크기는 반드시 화면 크기로 읽는다. GetLocalSize 는 설계 좌표
			// (1920 캔버스) 값이라, 0.871 로 줄여 그린 화면 위치와 섞이면
			// 자를 상자가 실제보다 15% 커진다 -- 처음에 그렇게 나왔다.
			const FGeometry Geometry = Target->GetCachedGeometry();
			const FVector2D Size = FVector2D(Geometry.GetAbsoluteSize());
			const FVector2D Pos = FVector2D(Geometry.GetAbsolutePosition());
			if (Size.X < 8.0 || Size.Y < 8.0)
			{
				continue;
			}

			// 대상의 조상 사슬을 모은다. 부모를 접으면 대상도 사라진다.
			TSet<UWidget*> Keep;
			for (UWidget* Walk = Target; Walk != nullptr; Walk = Walk->GetParent())
			{
				Keep.Add(Walk);
			}
			for (UWidget* Widget : All)
			{
				const bool bUnder = Widget->IsChildOf(Target) || Keep.Contains(Widget);
				Widget->SetVisibility(bUnder
					? Original[Widget]
					: ESlateVisibility::Hidden);
			}

			FWidgetRenderer Renderer(true, true);
			Renderer.SetIsPrepassNeeded(true);
			UTextureRenderTarget2D* RT = Renderer.DrawWidget(
				Scaled, FVector2D(CaptureWidth, CaptureHeight));
			if (RT == nullptr)
			{
				continue;
			}
			FlushRenderingCommands();
			TArray<FColor> Pixels;
			FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
			ReadFlags.SetLinearToGamma(false);
			if (!RT->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags))
			{
				continue;
			}
			// 전체 캡처와 같은 역보정. 렌더러가 두 번 인코딩하므로 한 번 되돌린다.
			for (FColor& Pixel : Pixels)
			{
				Pixel.R = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
				Pixel.G = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
				Pixel.B = uint8(FMath::RoundToInt(255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));
			}

			// 잘라내지 않는다.
			//
			// 부품만 오려 내면 그게 화면 어디에 앉는지가 사라진다 -- 정작
			// 알고 싶은 게 그것이다. 전체 화면을 그대로 두고 그 부품만 켠
			// 그림을 남긴다. 시안 위에 그대로 겹칠 수 있고, 여러 장을 넘겨
			// 보면 부품들이 제자리에 있는지 한눈에 읽힌다.
			TArray64<uint8> Png;
			FImageUtils::PNGCompressImageArray(CaptureWidth, CaptureHeight, Pixels, Png);
			const FString File = FPaths::Combine(Dir, FString::Printf(
				TEXT("%s_%s_%dx%d_at%d_%d.png"),
				*Target->GetClass()->GetName(), *Target->GetName(),
				int32(Size.X), int32(Size.Y), int32(Pos.X), int32(Pos.Y)));
			if (FFileHelper::SaveArrayToFile(Png, *File))
			{
				++Saved;
			}
		}

		for (const TPair<UWidget*, ESlateVisibility>& Pair : Original)
		{
			Pair.Key->SetVisibility(Pair.Value);
		}
		UE_LOG(LogTemp, Display, TEXT("[CombatLayout] %s 요소 %d장 -> %s"),
			*Stem, Saved, *Dir);
		return Saved > 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatLayoutElementCaptureTest,
	"P_RD.UI.CombatLayout.CaptureElements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLayoutElementCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;

	// NullRHI 환경에서는 오프스크린 렌더가 불가능하므로 캡처 생략
	if (GUsingNullRHI == true)
	{
		AddInfo(TEXT("NullRHI 환경이라 캡처 생략"));
		return true;
	}

	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드가 있어야 위젯을 만들 수 있다"), World))
	{
		return false;
	}
	// 한 배치에 오백 장이 넘게 나온다.
	FString Error;
	if (!CaptureElements(*World, LayoutClassPaths[0], Error))
	{
		AddError(Error);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatLayoutCaptureTest,
	"P_RD.UI.CombatLayout.Capture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLayoutCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;

	// NullRHI 환경에서는 오프스크린 렌더가 불가능하므로 캡처 생략
	if (GUsingNullRHI == true)
	{
		AddInfo(TEXT("NullRHI 환경이라 캡처 생략"));
		return true;
	}

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
		Error.Reset();
		if (!CaptureLayout(*World, ClassPath, Error, true))
		{
			AddError(FString::Printf(TEXT("%s 용병 탭: %s"), ClassPath, *Error));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMercenaryHireLayoutCaptureTest,
	"P_RD.UI.MercenaryHireMarchbound.Capture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireLayoutCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatLayoutCapture;

	if (GUsingNullRHI == true)
	{
		AddInfo(TEXT("NullRHI 환경이라 캡처 생략"));
		return true;
	}

	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드가 있어야 용병 선택 WBP를 만들 수 있다"), World))
	{
		return false;
	}

	FString Error;
	if (!CaptureLayout(*World,
		TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound.WBP_MercenaryHire_Marchbound_C"),
		Error))
	{
		AddError(Error);
	}
	return true;
}

#endif // WITH_EDITOR
