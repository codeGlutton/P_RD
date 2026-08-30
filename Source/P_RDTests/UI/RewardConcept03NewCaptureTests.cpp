/*****************************************************************//**
 * @file   RewardConcept03NewCaptureTests.cpp
 * @brief  독립 신규 4단계 보상 WBP를 배경·아이콘 없이 렌더한다.
 * @date   2026-08-17
 *********************************************************************/

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/SlateTypes.h"
#include "UI/Reward/RewardConcept03Widget.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/RunOptionsRailWidget.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/SOverlay.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace RewardConcept03NewCapture
{
	constexpr int32 CaptureWidth = 1672;
	constexpr int32 CaptureHeight = 941;
	constexpr TCHAR WidgetClassPath[] =
		TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_New.WBP_RewardConcept03_New_C");
	constexpr TCHAR NoArtifactWidgetClassPath[] =
		TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_New_NoArtifact.WBP_RewardConcept03_New_NoArtifact_C");
	constexpr TCHAR FramelessWidgetClassPath[] =
		TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_Frameless.WBP_RewardConcept03_Frameless_C");

	FString OutputDirectory()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UI"),
			TEXT("RewardConcept03New"));
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
		Widget.WidgetTree->ForEachWidget(
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

	bool Capture(UUserWidget& Widget, const TSharedRef<SWidget>& SlateWidget,
		const TCHAR* FileName, FString& OutError,
		const int32 ExpectedTextureCount = 23)
	{
		Widget.SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Widget.ForceLayoutPrepass();
		const int32 TextureCount = MakeTexturesResident(Widget);
		if (TextureCount != ExpectedTextureCount)
		{
			OutError = FString::Printf(
				TEXT("신규 WBP 텍스처 수 불일치: expected=%d actual=%d"),
				ExpectedTextureCount, TextureCount);
			return false;
		}

		// 환경 배경 아트는 사용하지 않고 검수용 단색만 둔다.
		const TSharedRef<SWidget> CaptureRoot =
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SColorBlock).Color(FLinearColor(.008f, .01f, .014f, 1.f))
			]
			+ SOverlay::Slot()
			[
				SlateWidget
			];

		FWidgetRenderer Renderer(true, true);
		Renderer.SetIsPrepassNeeded(true);
		for (int32 Warmup = 0; Warmup < 16; ++Warmup)
		{
			Renderer.DrawWidget(CaptureRoot,
				FVector2D(CaptureWidth, CaptureHeight));
			FlushRenderingCommands();
		}
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(
			CaptureRoot, FVector2D(CaptureWidth, CaptureHeight));
		if (Target == nullptr)
		{
			OutError = TEXT("신규 WBP 렌더 타깃 생성 실패");
			return false;
		}

		FlushRenderingCommands();
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags Flags(RCM_UNorm);
		Flags.SetLinearToGamma(false);
		if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags)
			|| Pixels.Num() != CaptureWidth * CaptureHeight)
		{
			OutError = TEXT("신규 WBP 픽셀 읽기 실패");
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
		if (ChangedPixels < CaptureWidth * CaptureHeight / 20)
		{
			OutError = TEXT("신규 WBP 캡처가 단색이거나 비어 있음");
			return false;
		}

		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(CaptureWidth, CaptureHeight, Pixels, Png);
		IFileManager::Get().MakeDirectory(*OutputDirectory(), true);
		const FString OutputPath = FPaths::Combine(OutputDirectory(), FileName);
		if (!FFileHelper::SaveArrayToFile(Png, *OutputPath))
		{
			OutError = FString::Printf(TEXT("신규 WBP 캡처 저장 실패: %s"),
				*OutputPath);
			return false;
		}
		UE_LOG(LogTemp, Display,
			TEXT("[RewardConcept03NewCapture] %s (%d new textures) -> %s"),
			FileName, TextureCount, *OutputPath);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardConcept03NewInteractionTest,
	"P_RD.UI.RewardConcept03New.Interaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardConcept03NewInteractionTest::RunTest(const FString& Parameters)
{
	// 한글 표시 문자열을 단언하므로 ko 컬처로 고정한다. en/ko 번역이 모두
	// 채워진 뒤로는 실행 컬처에 따라 표시가 달라진다(0823).
	struct FScopedKoreanCulture
	{
		FString mOriginal;
		FScopedKoreanCulture()
			: mOriginal(FInternationalization::Get().GetCurrentCulture()->GetName())
		{
			FInternationalization::Get().SetCurrentCulture(TEXT("ko"));
		}
		~FScopedKoreanCulture()
		{
			FInternationalization::Get().SetCurrentCulture(mOriginal);
		}
	};
	const FScopedKoreanCulture ScopedKoreanCulture;
	using namespace RewardConcept03NewCapture;
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("신규 WBP 상호작용 월드"), World))
	{
		return false;
	}
	UClass* WidgetClass = LoadClass<URewardConcept03Widget>(nullptr, WidgetClassPath);
	if (!TestNotNull(TEXT("신규 보상 위젯 클래스"), WidgetClass))
	{
		return false;
	}
	URewardConcept03Widget* Widget = CreateWidget<URewardConcept03Widget>(
		World, WidgetClass);
	if (!TestNotNull(TEXT("신규 보상 위젯 인스턴스"), Widget))
	{
		return false;
	}
	Widget->TakeWidget();
	Widget->InitializeInteractionBindingsForTest();
	URunOptionsRailWidget* OptionsRail = Widget->GetRunOptionsRailForTest();
	UClass* OptionsRailClass = LoadClass<URunOptionsRailWidget>(nullptr,
		TEXT("/Game/UI/Common/WBP_RunOptionsRail.WBP_RunOptionsRail_C"));
	TestNotNull(TEXT("보상 화면 공용 설정바 클래스"), OptionsRailClass);
	// EditorWorld has no runtime PlayerController, so the detached automation
	// widget cannot create a viewport rail. Validate its class contract here;
	// PIE validates the actual attached instance.
	if (OptionsRail != nullptr)
	{
		OptionsRail->TakeWidget();
		TestNotNull(TEXT("보상 화면 설정 버튼"),
			OptionsRail->GetWidgetFromName(TEXT("MenuButton_3")));
	}
	URewardUIModel* RewardModel = NewObject<URewardUIModel>(Widget);
	FRewardUI ExpReward;
	ExpReward.mExpGained = 50;
	FRewardMercenaryExpUI& LevelingMercenary =
		ExpReward.mMercenaryExp.AddDefaulted_GetRef();
	LevelingMercenary.mName = FText::FromString(TEXT("레벨업 시험 용병"));
	LevelingMercenary.mLevel = 2;
	LevelingMercenary.mLevelBefore = 1;
	LevelingMercenary.mLevelAfter = 2;
	LevelingMercenary.mExpBefore = 90.f;
	LevelingMercenary.mExpAfter = 40.f;
	LevelingMercenary.mMaxExp = 300.f;
	FRewardExpProgressStepUI& FirstExpStep =
		LevelingMercenary.mProgressSteps.AddDefaulted_GetRef();
	FirstExpStep.mLevelBefore = 1;
	FirstExpStep.mLevelAfter = 2;
	FirstExpStep.mExpBefore = 90.f;
	FirstExpStep.mExpAfter = 100.f;
	FirstExpStep.mMaxExp = 100.f;
	FRewardExpProgressStepUI& SecondExpStep =
		LevelingMercenary.mProgressSteps.AddDefaulted_GetRef();
	SecondExpStep.mLevelBefore = 2;
	SecondExpStep.mLevelAfter = 2;
	SecondExpStep.mExpBefore = 0.f;
	SecondExpStep.mExpAfter = 40.f;
	SecondExpStep.mMaxExp = 300.f;
	FRewardMercenaryExpUI& MultiLevelMercenary =
		ExpReward.mMercenaryExp.AddDefaulted_GetRef();
	MultiLevelMercenary.mName = FText::FromString(TEXT("다중 레벨업 시험 용병"));
	MultiLevelMercenary.mLevel = 3;
	MultiLevelMercenary.mLevelBefore = 1;
	MultiLevelMercenary.mLevelAfter = 3;
	MultiLevelMercenary.mExpBefore = 90.f;
	MultiLevelMercenary.mExpAfter = 50.f;
	MultiLevelMercenary.mMaxExp = 300.f;
	auto AddMultiLevelStep = [&MultiLevelMercenary](const int32 LevelBefore,
		const int32 LevelAfter, const float ExpBefore, const float ExpAfter,
		const float MaxExp)
	{
		FRewardExpProgressStepUI& Step =
			MultiLevelMercenary.mProgressSteps.AddDefaulted_GetRef();
		Step.mLevelBefore = LevelBefore;
		Step.mLevelAfter = LevelAfter;
		Step.mExpBefore = ExpBefore;
		Step.mExpAfter = ExpAfter;
		Step.mMaxExp = MaxExp;
	};
	AddMultiLevelStep(1, 2, 90.f, 100.f, 100.f);
	AddMultiLevelStep(2, 3, 0.f, 200.f, 200.f);
	AddMultiLevelStep(3, 3, 0.f, 50.f, 300.f);
	FRewardMercenaryExpUI& LegacyLevelingMercenary =
		ExpReward.mMercenaryExp.AddDefaulted_GetRef();
	LegacyLevelingMercenary.mName = FText::FromString(
		TEXT("구형 레벨업 시험 용병"));
	LegacyLevelingMercenary.mLevel = 2;
	LegacyLevelingMercenary.mLevelBefore = 1;
	LegacyLevelingMercenary.mLevelAfter = 2;
	LegacyLevelingMercenary.mExpBefore = 90.f;
	LegacyLevelingMercenary.mExpAfter = 40.f;
	LegacyLevelingMercenary.mMaxExp = 300.f;
	RewardModel->SetReward(ExpReward);
	TArray<FRewardChoiceUI> TestChoices;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FRewardChoiceUI Choice;
		Choice.mChoiceIndex = Index;
		Choice.mSourceAssetId = FPrimaryAssetId(
			TEXT("Artifact"), FName(*FString::Printf(TEXT("TestArtifact_%d"), Index)));
		Choice.mName = FText::FromString(FString::Printf(TEXT("시험 아티팩트 %d"),
			Index + 1));
		Choice.mDescription = FText::FromString(TEXT("롱프레스 상세 설명"));
		TestChoices.Add(Choice);
	}
	RewardModel->SetRewardChoices(TestChoices);
	Widget->BindUIModel(RewardModel);
	UTextBlock* LevelingLevel = Cast<UTextBlock>(
		Widget->GetWidgetFromName(TEXT("NewLevel_0")));
	UTextBlock* LevelingProgress = Cast<UTextBlock>(
		Widget->GetWidgetFromName(TEXT("NewProgress_0")));
	UTextBlock* LevelUpBadge = Cast<UTextBlock>(
		Widget->GetWidgetFromName(TEXT("NewLevelUp_0")));
	if (TestNotNull(TEXT("레벨업 대상 레벨 문구"), LevelingLevel)
		&& TestNotNull(TEXT("레벨업 대상 경험치 문구"), LevelingProgress)
		&& TestNotNull(TEXT("용병별 레벨업 배지"), LevelUpBadge))
	{
		TestEqual(TEXT("롤오버 전 레벨"), LevelingLevel->GetText().ToString(),
			FString(TEXT("Lv.1")));
		TestEqual(TEXT("롤오버 전 경험치"), LevelingProgress->GetText().ToString(),
			FString(TEXT("90 / 100")));
		TestEqual(TEXT("임계치 전 레벨업 배지 숨김"),
			LevelUpBadge->GetVisibility(), ESlateVisibility::Collapsed);
	}
	UTextBlock* LegacyLevel = Cast<UTextBlock>(
		Widget->GetWidgetFromName(TEXT("NewLevel_2")));
	UTextBlock* LegacyLevelBadge = Cast<UTextBlock>(
		Widget->GetWidgetFromName(TEXT("NewLevelUp_2")));
	if (TestNotNull(TEXT("구형 레벨업 레벨 문구"), LegacyLevel)
		&& TestNotNull(TEXT("구형 레벨업 배지"), LegacyLevelBadge))
	{
		// 모든 용병 행은 같은 타임라인으로 움직이므로 최초 상태는 첫 Advance
		// 전에 확인해야 한다.
		TestEqual(TEXT("구형 1→2 payload 시작 레벨"),
			LegacyLevel->GetText().ToString(), FString(TEXT("Lv.1")));
		TestEqual(TEXT("구형 payload 시작 시 배지 숨김"),
			LegacyLevelBadge->GetVisibility(), ESlateVisibility::Collapsed);
	}

	if (LevelingLevel != nullptr && LevelingProgress != nullptr
		&& LevelUpBadge != nullptr)
	{
		Widget->AdvanceExperienceAnimationForTest(1.10f);
		TestEqual(TEXT("임계치 도달 후 레벨 증가"),
			LevelingLevel->GetText().ToString(), FString(TEXT("Lv.2")));
		TestEqual(TEXT("임계치 도달 진행도"),
			LevelingProgress->GetText().ToString(), FString(TEXT("100 / 100")));
		TestEqual(TEXT("실제 레벨업 대상 배지 표시"),
			LevelUpBadge->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);

		Widget->AdvanceExperienceAnimationForTest(1.38f);
		TestEqual(TEXT("다음 레벨 최종 잔여 경험치"),
			LevelingProgress->GetText().ToString(), FString(TEXT("40 / 300")));
		TestEqual(TEXT("최종 레벨 유지"), LevelingLevel->GetText().ToString(),
			FString(TEXT("Lv.2")));
	}
	UTextBlock* MultiLevel = Cast<UTextBlock>(
		Widget->GetWidgetFromName(TEXT("NewLevel_1")));
	UTextBlock* MultiProgress = Cast<UTextBlock>(
		Widget->GetWidgetFromName(TEXT("NewProgress_1")));
	UTextBlock* MultiLevelBadge = Cast<UTextBlock>(
		Widget->GetWidgetFromName(TEXT("NewLevelUp_1")));
	Widget->AdvanceExperienceAnimationForTest(3.f);
	if (LegacyLevel != nullptr && LegacyLevelBadge != nullptr)
	{
		TestEqual(TEXT("구형 1→2 payload 최종 레벨"),
			LegacyLevel->GetText().ToString(), FString(TEXT("Lv.2")));
		TestEqual(TEXT("구형 1→2 payload 레벨업 배지"),
			LegacyLevelBadge->GetVisibility(),
			ESlateVisibility::SelfHitTestInvisible);
	}
	if (TestNotNull(TEXT("다중 레벨 최종 레벨 문구"), MultiLevel)
		&& TestNotNull(TEXT("다중 레벨 최종 경험치 문구"), MultiProgress)
		&& TestNotNull(TEXT("다중 레벨업 배지"), MultiLevelBadge))
	{
		TestEqual(TEXT("다중 레벨 최종 레벨"), MultiLevel->GetText().ToString(),
			FString(TEXT("Lv.3")));
		TestEqual(TEXT("다중 레벨 최종 잔여 경험치"),
			MultiProgress->GetText().ToString(), FString(TEXT("50 / 300")));
		TestEqual(TEXT("다중 레벨업 횟수 표시"),
			MultiLevelBadge->GetText().ToString(), FString(TEXT("레벨 업! ×2")));
	}
	auto TestChoiceLayout = [this, Widget, RewardModel, &TestChoices](
		const int32 ChoiceCount, const TArray<double>& ExpectedXs)
	{
		TArray<FRewardChoiceUI> VisibleChoices;
		for (int32 Index = 0; Index < ChoiceCount; ++Index)
		{
			VisibleChoices.Add(TestChoices[Index]);
		}
		RewardModel->SetRewardChoices(VisibleChoices);
		for (int32 Index = 0; Index < ExpectedXs.Num(); ++Index)
		{
			UWidget* Panel = Widget->GetWidgetFromName(*FString::Printf(
				TEXT("NewArtifactChoicePanel_%d"), Index));
			const UCanvasPanelSlot* PanelSlot = Panel != nullptr
				? Cast<UCanvasPanelSlot>(Panel->Slot) : nullptr;
			if (TestNotNull(*FString::Printf(
				TEXT("%d개 보상 카드 %d Canvas 슬롯"), ChoiceCount, Index),
				PanelSlot))
			{
				TestEqual(*FString::Printf(
					TEXT("%d개 보상 카드 %d X 위치"), ChoiceCount, Index),
					PanelSlot->GetPosition().X, ExpectedXs[Index]);
			}
		}
	};
	TestChoiceLayout(1, { 450.0 });
	TestChoiceLayout(2, { 285.0, 615.0 });
	TestChoiceLayout(3, { 120.0, 450.0, 780.0 });

	URewardConcept03Widget* GrantAllWidget = CreateWidget<URewardConcept03Widget>(
		World, WidgetClass);
	URewardUIModel* GrantAllModel = NewObject<URewardUIModel>(GrantAllWidget);
	if (TestNotNull(TEXT("일괄 지급 보상 위젯"), GrantAllWidget)
		&& TestNotNull(TEXT("일괄 지급 보상 모델"), GrantAllModel))
	{
		FRewardGrantBundleUI Bundle;
		Bundle.mItems.Add(TestChoices[0]);
		GrantAllModel->SetGrantBundle(Bundle);
		GrantAllWidget->BindUIModel(GrantAllModel);
		GrantAllWidget->TakeWidget();
		GrantAllWidget->InitializeInteractionBindingsForTest();
		GrantAllWidget->SetRewardPresentationManualTick(true);
		UWidget* OnlyPanel = GrantAllWidget->GetWidgetFromName(
			TEXT("NewArtifactChoicePanel_0"));
		const UCanvasPanelSlot* OnlyPanelSlot = OnlyPanel != nullptr
			? Cast<UCanvasPanelSlot>(OnlyPanel->Slot) : nullptr;
		if (TestNotNull(TEXT("일괄 지급 단일 카드 Canvas 슬롯"), OnlyPanelSlot))
		{
			TestEqual(TEXT("일괄 지급 단일 카드는 중앙 정렬"),
				OnlyPanelSlot->GetPosition().X, 450.0);
		}
		if (UWidget* Outline = GrantAllWidget->GetWidgetFromName(
			TEXT("NewArtifactSelection")))
		{
			TestEqual(TEXT("일괄 지급에서는 선택 외곽선 숨김"),
				Outline->GetVisibility(), ESlateVisibility::Collapsed);
		}
		GrantAllWidget->AdvanceRewardFlow();
		GrantAllWidget->OpenRewardChest();
		GrantAllWidget->SkipRewardPresentation();
		GrantAllWidget->SkipRewardPresentation();
		// 골드가 나온 뒤에는 사용자의 다음 터치를 기다린다.
		GrantAllWidget->AdvanceRewardFlow();
		GrantAllWidget->SkipRewardPresentation();
		TestEqual(TEXT("일괄 지급 아티팩트 단계 도달"),
			GrantAllWidget->GetCurrentStepIndex(), 3);
		TestFalse(TEXT("일괄 지급 아티팩트 공개 연출 완료"),
			GrantAllWidget->IsRewardPresentationPlaying());
		TestFalse(TEXT("일괄 지급 확정 전 흐름 미완료"),
			GrantAllWidget->IsRewardFlowCompleted());
		TestEqual(TEXT("일괄 지급 상세 대상 유지"),
			GrantAllModel->GetRewardChoices().Num(), 1);
		UButton* GrantAllCard = Cast<UButton>(GrantAllWidget->GetWidgetFromName(
			TEXT("NewArtifactChoiceButton_0")));
		if (TestNotNull(TEXT("일괄 지급 카드 롱프레스 버튼"), GrantAllCard))
		{
			TestTrue(TEXT("일괄 지급 카드 롱프레스 시작 바인딩"),
				GrantAllCard->OnPressed.IsBound());
			TestTrue(TEXT("일괄 지급 카드 롱프레스 종료 바인딩"),
				GrantAllCard->OnReleased.IsBound());
		}
	}
	for (const TCHAR* ButtonName : {
		TEXT("NewBottomActionButton"), TEXT("NewChestOpenButton"),
		TEXT("NewArtifactChoiceButton_0"),
		TEXT("NewArtifactChoiceButton_1"),
		TEXT("NewArtifactChoiceButton_2") })
	{
		TestNotNull(*FString::Printf(TEXT("기능 버튼 존재: %s"), ButtonName),
			Cast<UButton>(Widget->GetWidgetFromName(ButtonName)));
	}
	TestNull(TEXT("보상 WBP 내부 중복 상세 모달 제거"),
		Widget->GetWidgetFromName(TEXT("NewArtifactDetailOverlay")));
	auto TestFunctionalParent = [this, Widget](const TCHAR* ChildName,
		const TCHAR* ParentName)
	{
		UWidget* Child = Widget->GetWidgetFromName(FName(ChildName));
		TestNotNull(*FString::Printf(TEXT("기능 위젯 존재: %s"), ChildName), Child);
		TestTrue(*FString::Printf(TEXT("%s는 %s 내부에 배치"), ChildName, ParentName),
			Child != nullptr && Child->GetParent() != nullptr
			&& Child->GetParent()->GetFName() == FName(ParentName));
	};
	TestFunctionalParent(TEXT("NewTitleText"), TEXT("NewRewardHeaderPanel"));
	TestFunctionalParent(TEXT("NewRewardProgressSwitcher"),
		TEXT("NewRewardProgressPanel"));
	TestFunctionalParent(TEXT("NewRewardStepSwitcher"),
		TEXT("NewRewardFramePanel"));
	TestFunctionalParent(TEXT("NewRewardTabSwitcher"),
		TEXT("NewRewardTabPanel"));
	TestFunctionalParent(TEXT("NewBottomActionButton"),
		TEXT("NewBottomButtonPanel"));
	TestFunctionalParent(TEXT("NewProgress_0"), TEXT("NewProgressZone_0"));
	TestFunctionalParent(TEXT("NewLevel_0"), TEXT("NewLevel_0_Fit"));
	TestFunctionalParent(TEXT("NewFill_0"), TEXT("NewFillClip_0"));
	TestFunctionalParent(TEXT("NewPortraitImage_0"), TEXT("NewPortraitInner_0"));
	TestFunctionalParent(TEXT("NewCompletedProgress_1"),
		TEXT("NewCompletedProgressClip_1"));
	UImage* StepTrack = Cast<UImage>(
		Widget->GetWidgetFromName(TEXT("NewProgressTrackArt")));
	UImage* StepFill = Cast<UImage>(
		Widget->GetWidgetFromName(TEXT("NewCompletedProgress_1")));
	if (TestNotNull(TEXT("단계 빈 바 레이어"), StepTrack)
		&& TestNotNull(TEXT("단계 채움 바 레이어"), StepFill))
	{
		const UTexture2D* TrackTexture = Cast<UTexture2D>(
			StepTrack->GetBrush().GetResourceObject());
		const UTexture2D* FillTexture = Cast<UTexture2D>(
			StepFill->GetBrush().GetResourceObject());
		if (TestNotNull(TEXT("빈 바 텍스처"), TrackTexture)
			&& TestNotNull(TEXT("완전 채움 바 텍스처"), FillTexture))
		{
			TestEqual(TEXT("기존 진한 갈색 빈 바 원본 규격"),
				TrackTexture->GetImportedSize(), FIntPoint(1913, 168));
			TestEqual(TEXT("완전 채움 바 원본 규격"),
				FillTexture->GetImportedSize(), FIntPoint(936, 54));
		}
		const UCanvasPanelSlot* TrackSlot = Cast<UCanvasPanelSlot>(StepTrack->Slot);
		const UCanvasPanelSlot* FillSlot = Cast<UCanvasPanelSlot>(StepFill->Slot);
		if (TestNotNull(TEXT("빈 바 Canvas 슬롯"), TrackSlot)
			&& TestNotNull(TEXT("채움 바 Canvas 슬롯"), FillSlot))
		{
			TestEqual(TEXT("빈 바 표시 크기"),
				TrackSlot->GetSize(), FVector2D(900.f, 54.f));
			TestEqual(TEXT("채움 바 표시 크기"),
				FillSlot->GetSize(), TrackSlot->GetSize());
		}
	}
	UImage* ExperienceTrack = Cast<UImage>(
		Widget->GetWidgetFromName(TEXT("NewTrack_0")));
	UImage* ExperienceFill = Cast<UImage>(
		Widget->GetWidgetFromName(TEXT("NewFill_0")));
	if (TestNotNull(TEXT("경험치 빈 바 레이어"), ExperienceTrack)
		&& TestNotNull(TEXT("경험치 채움 바 레이어"), ExperienceFill)
		&& StepTrack != nullptr && StepFill != nullptr)
	{
		TestTrue(TEXT("단계 바와 경험치 바가 같은 빈 바 텍스처 사용"),
			ExperienceTrack->GetBrush().GetResourceObject()
				== StepTrack->GetBrush().GetResourceObject());
		TestTrue(TEXT("단계 바와 경험치 바가 같은 채움 텍스처 사용"),
			ExperienceFill->GetBrush().GetResourceObject()
				== StepFill->GetBrush().GetResourceObject());
		const UCanvasPanelSlot* TrackSlot =
			Cast<UCanvasPanelSlot>(ExperienceTrack->Slot);
		const UCanvasPanelSlot* FillSlot =
			Cast<UCanvasPanelSlot>(ExperienceFill->Slot);
		if (TestNotNull(TEXT("경험치 빈 바 Canvas 슬롯"), TrackSlot)
			&& TestNotNull(TEXT("경험치 채움 바 Canvas 슬롯"), FillSlot))
		{
			TestEqual(TEXT("경험치 빈 바 표시 크기"),
				TrackSlot->GetSize(), FVector2D(484.f, 36.f));
			TestEqual(TEXT("경험치 채움 바 표시 크기"),
				FillSlot->GetSize(), TrackSlot->GetSize());
		}
	}
	for (const TPair<const TCHAR*, FVector2D>& ClipExpectation : {
		TPair<const TCHAR*, FVector2D>(TEXT("NewCompletedProgressClip_1"),
			FVector2D(120.f, 54.f)),
		TPair<const TCHAR*, FVector2D>(TEXT("NewCompletedProgressClip_4"),
			FVector2D(900.f, 54.f)) })
	{
		UWidget* Clip = Widget->GetWidgetFromName(ClipExpectation.Key);
		if (TestNotNull(*FString::Printf(TEXT("진행도 클립 존재: %s"),
			ClipExpectation.Key), Clip))
		{
			const UCanvasPanelSlot* ClipSlot = Cast<UCanvasPanelSlot>(Clip->Slot);
			if (TestNotNull(TEXT("진행도 클립 Canvas 슬롯"), ClipSlot))
			{
				TestEqual(TEXT("진행도 클립 크기"),
					ClipSlot->GetSize(), ClipExpectation.Value);
				TestEqual(TEXT("진행도 클립 Y축 정렬"),
					ClipSlot->GetPosition().Y, 18.0);
			}
			TestEqual(TEXT("진행도 클립 활성"), Clip->GetClipping(),
				EWidgetClipping::ClipToBoundsAlways);
		}
	}
	TestFunctionalParent(TEXT("NewExperienceRewardText"),
		TEXT("NewExperienceRewardZone"));
	TestFunctionalParent(TEXT("NewChestMain"), TEXT("NewChestMainZone"));
	TestFunctionalParent(TEXT("NewChestOpenButton"),
		TEXT("NewChestVisualPanel"));
	TestFunctionalParent(TEXT("NewChestVisualSwitcher"),
		TEXT("NewChestVisualPanel"));
	TestFunctionalParent(TEXT("NewChoiceName_0"), TEXT("NewChoiceName_0_Fit"));
	TestNull(TEXT("요약판 하단 레벨 문구 제거"),
		Widget->GetWidgetFromName(TEXT("NewExperienceLevelResultText")));
	auto TestCenteredOverlayText = [this, Widget](const TCHAR* TextName)
	{
		UTextBlock* Text = Cast<UTextBlock>(
			Widget->GetWidgetFromName(FName(TextName)));
		if (!TestNotNull(*FString::Printf(TEXT("중앙정렬 텍스트 존재: %s"),
			TextName), Text))
		{
			return;
		}
		if (UOverlaySlot* Slot = Cast<UOverlaySlot>(Text->Slot))
		{
			TestEqual(*FString::Printf(TEXT("가로 Fill 중앙정렬 영역: %s"), TextName),
				Slot->GetHorizontalAlignment(), HAlign_Fill);
			TestEqual(*FString::Printf(TEXT("세로 중앙정렬 영역: %s"), TextName),
				Slot->GetVerticalAlignment(), VAlign_Center);
		}
		else if (UScaleBoxSlot* ScaleSlot = Cast<UScaleBoxSlot>(Text->Slot))
		{
			TestEqual(*FString::Printf(TEXT("축소 안전 가로 중앙: %s"), TextName),
				ScaleSlot->GetHorizontalAlignment(), HAlign_Center);
			TestEqual(*FString::Printf(TEXT("축소 안전 세로 중앙: %s"), TextName),
				ScaleSlot->GetVerticalAlignment(), VAlign_Center);
		}
		else
		{
			AddError(FString::Printf(TEXT("중앙정렬 슬롯 유형 불일치: %s"), TextName));
		}
	};
	for (const TCHAR* TextName : {
		TEXT("NewTitleText"), TEXT("NewTabText_1"),
		TEXT("NewButtonText_1"), TEXT("NewProgress_0"),
		TEXT("NewExperienceRewardText"), TEXT("NewChestMain"),
		TEXT("NewChestHint"), TEXT("NewGoldMain"),
		TEXT("NewChoiceName_1") })
	{
		TestCenteredOverlayText(TextName);
	}

	UImage* Parchment = Cast<UImage>(
		Widget->GetWidgetFromName(TEXT("NewPanelParchment")));
	if (TestNotNull(TEXT("프레임 내부 전체 채움 배경"), Parchment))
	{
		UCanvasPanelSlot* ParchmentSlot = Cast<UCanvasPanelSlot>(Parchment->Slot);
		if (TestNotNull(TEXT("배경 Canvas 슬롯"), ParchmentSlot))
		{
			TestEqual(TEXT("배경 0-offset 위치"),
				ParchmentSlot->GetPosition(), FVector2D::ZeroVector);
			TestEqual(TEXT("배경이 프레임 패널 전체 크기 사용"),
				ParchmentSlot->GetSize(), FVector2D(1320.f, 600.f));
		}
	}
	TestEqual(TEXT("초기 경험치 단계"), Widget->GetCurrentStepIndex(), 0);
	UWidgetSwitcher* ChestVisualSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewChestVisualSwitcher")));
	if (TestNotNull(TEXT("상자 닫힘/열림 상태 switcher"), ChestVisualSwitcher))
	{
		TestEqual(TEXT("상자 초기 닫힘 이미지"),
			ChestVisualSwitcher->GetActiveWidgetIndex(), 0);
		TestEqual(TEXT("상자 이미지 상태 수"),
			ChestVisualSwitcher->GetNumWidgets(), 5);
	}
	Widget->AdvanceRewardFlow();
	TestEqual(TEXT("상자 단계 이동"), Widget->GetCurrentStepIndex(), 1);
	Widget->AdvanceRewardFlow();
	TestEqual(TEXT("상자 개봉 전 진행 차단"), Widget->GetCurrentStepIndex(), 1);
	Widget->OpenRewardChest();
	Widget->SetRewardPresentationManualTick(true);
	TestTrue(TEXT("상자 개봉 상태"), Widget->IsRewardChestOpened());
	TestTrue(TEXT("상자 개봉 연출 재생"),
		Widget->IsRewardPresentationPlaying());
	UWidget* AtlasBlend = Widget->GetWidgetFromName(
		TEXT("NewChestSequenceBlendImage"));
	TestTrue(TEXT("상자 프레임 중첩 레이어 비활성"), AtlasBlend == nullptr
		|| AtlasBlend->GetVisibility() == ESlateVisibility::Collapsed);
	for (const TCHAR* EffectName : { TEXT("NewChestBurstGlow_0"),
		TEXT("NewChestBurstRing_0"), TEXT("NewChestBurstRays_0"),
		TEXT("NewChestBurstSpark_0") })
	{
		if (UWidget* Effect = Widget->GetWidgetFromName(EffectName))
		{
			TestEqual(*FString::Printf(TEXT("보조 상자 광원 비활성: %s"),
				EffectName), Effect->GetVisibility(), ESlateVisibility::Collapsed);
		}
	}
	if (ChestVisualSwitcher != nullptr)
	{
		TestEqual(TEXT("상자 개봉 연출은 닫힘 프레임에서 시작"),
			ChestVisualSwitcher->GetActiveWidgetIndex(), 0);
	}
	Widget->AdvanceRewardFlow();
	TestEqual(TEXT("수동 다음 입력은 개봉 연출을 건너뛰지 않음"),
		Widget->GetCurrentStepIndex(), 1);
	Widget->AdvanceRewardPresentation(.95f);
	TestEqual(TEXT("0.95초에는 상자 연출 유지"),
		Widget->GetCurrentStepIndex(), 1);
	Widget->AdvanceRewardPresentation(.21f);
	TestEqual(TEXT("개봉 후 골드 단계 자동 이동"),
		Widget->GetCurrentStepIndex(), 2);
	TestTrue(TEXT("골드 연출 재생"), Widget->IsRewardPresentationPlaying());
	Widget->SkipRewardPresentation();
	TestEqual(TEXT("골드 공개 뒤 다음 터치 대기"),
		Widget->GetCurrentStepIndex(), 2);
	Widget->AdvanceRewardFlow();
	TestEqual(TEXT("골드 확인 터치 후 아티팩트 단계 이동"),
		Widget->GetCurrentStepIndex(), 3);
	Widget->SkipRewardPresentation();
	TestFalse(TEXT("카드 등장 후 입력 대기"),
		Widget->IsRewardPresentationPlaying());
	Widget->ShowArtifactDetails(1);
	UUserWidget* DetailOverlay = Widget->GetArtifactDetailOverlayForTest();
	TestTrue(TEXT("기존 CombatDetail 상세 모달 표시"), DetailOverlay != nullptr
		&& DetailOverlay->GetVisibility()
			== ESlateVisibility::SelfHitTestInvisible);
	if (UTextBlock* DetailName = Cast<UTextBlock>(
		DetailOverlay != nullptr ? DetailOverlay->GetWidgetFromName(
			TEXT("DetailTitleText")) : nullptr))
	{
		TestEqual(TEXT("공용 상세 모달 선택 이름"), DetailName->GetText().ToString(),
			FString(TEXT("시험 아티팩트 2")));
	}
	Widget->HideArtifactDetails();
	TestTrue(TEXT("공용 상세 모달 닫기"), DetailOverlay != nullptr
		&& DetailOverlay->GetVisibility() == ESlateVisibility::Collapsed);
	Widget->SelectArtifact(2);
	TestEqual(TEXT("세 번째 아티팩트 선택"),
		Widget->GetSelectedArtifactIndex(), 2);
	Widget->AdvanceRewardFlow();
	TestFalse(TEXT("지급 confirmation 전에는 보상 흐름을 확정하지 않음"),
		Widget->IsRewardFlowCompleted());
	RewardModel->ConfirmSelectedReward(TestChoices[2].mSourceAssetId);
	TestTrue(TEXT("지급 confirmation 후 보상 흐름 확정"),
		Widget->IsRewardFlowCompleted());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardConcept03NewRenderedCaptureTest,
	"P_RD.UI.RewardConcept03New.RenderedCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardConcept03NewRenderedCaptureTest::RunTest(const FString& Parameters)
{
	using namespace RewardConcept03NewCapture;
	if (GUsingNullRHI)
	{
		AddInfo(TEXT("NullRHI 환경이라 신규 RewardConcept03 캡처 생략"));
		return true;
	}

	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("신규 WBP 캡처 월드"), World))
	{
		return false;
	}
	UClass* WidgetClass = LoadClass<URewardConcept03Widget>(nullptr, WidgetClassPath);
	if (!TestNotNull(TEXT("신규 WBP_RewardConcept03_New 클래스"), WidgetClass))
	{
		return false;
	}
	URewardConcept03Widget* Widget = CreateWidget<URewardConcept03Widget>(
		World, WidgetClass);
	if (!TestNotNull(TEXT("신규 WBP_RewardConcept03_New 인스턴스"), Widget))
	{
		return false;
	}
	TestFalse(TEXT("기존 RewardSettlementWidgetBase를 사용하지 않음"),
		WidgetClass->GetPathName().Contains(TEXT("RewardSettlement")));

	UWidgetSwitcher* StepSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardStepSwitcher")));
	UWidgetSwitcher* ProgressSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardProgressSwitcher")));
	UWidgetSwitcher* TabSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardTabSwitcher")));
	UWidgetSwitcher* ButtonSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardButtonSwitcher")));
	if (!TestNotNull(TEXT("신규 4단계 switcher"), StepSwitcher)
		|| !TestNotNull(TEXT("신규 진행 switcher"), ProgressSwitcher)
		|| !TestNotNull(TEXT("신규 탭 switcher"), TabSwitcher)
		|| !TestNotNull(TEXT("신규 버튼 switcher"), ButtonSwitcher))
	{
		return false;
	}
	TestEqual(TEXT("신규 WBP warmup 포함 자식 수"),
		StepSwitcher->GetNumWidgets(), 5);

	const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	// 새 프로세스의 첫 WidgetRenderer 호출은 UI 텍스처 리소스를 준비하는
	// 프라이밍 프레임이다. 최종 1단계 캡처 전에 한 번 버리고 다시 그린다.
	StepSwitcher->SetActiveWidgetIndex(1);
	ProgressSwitcher->SetActiveWidgetIndex(1);
	TabSwitcher->SetActiveWidgetIndex(1);
	ButtonSwitcher->SetActiveWidgetIndex(1);
	FString PrimerError;
	if (!Capture(*Widget, SlateWidget, TEXT("_Warmup.png"), PrimerError))
	{
		AddError(PrimerError);
		return false;
	}
	IFileManager::Get().Delete(*FPaths::Combine(
		OutputDirectory(), TEXT("_Warmup.png")), false, true);
	PrimerError.Reset();
	if (!Capture(*Widget, SlateWidget, TEXT("_Warmup2.png"), PrimerError))
	{
		AddError(PrimerError);
		return false;
	}
	IFileManager::Get().Delete(*FPaths::Combine(
		OutputDirectory(), TEXT("_Warmup2.png")), false, true);
	// 최초 상태를 다시 활성화할 때 Slate가 모든 이미지 브러시를 무효화하도록
	// 프라이밍 뒤 잠시 다른 상태로 전환한다.
	StepSwitcher->SetActiveWidgetIndex(2);
	ProgressSwitcher->SetActiveWidgetIndex(2);
	TabSwitcher->SetActiveWidgetIndex(2);
	ButtonSwitcher->SetActiveWidgetIndex(2);

	const TCHAR* FileNames[] = {
		TEXT("WBP_RewardConcept03_New_01_Experience.png"),
		TEXT("WBP_RewardConcept03_New_02_Chest.png"),
		TEXT("WBP_RewardConcept03_New_03_Gold.png"),
		TEXT("WBP_RewardConcept03_New_04_Artifact.png")
	};
	// 최초 활성 상태는 Slate 리소스 준비용으로 이미 충분히 그렸다. 최종 파일은
	// 2~4단계를 먼저 저장하고 1단계를 마지막에 다시 저장해 첫 상태도 완성한다.
	// 첫 렌더러 인스턴스에서 일부 UI 텍스처가 GPU에 올라오는 동안 생기는
	// 검수 이미지 누락을 피하도록 상자 단계는 마지막에 캡처한다.
	const int32 CaptureOrder[] = { 2, 3, 0, 1 };
	for (const int32 Step : CaptureOrder)
	{
		const int32 SwitcherIndex = Step + 1;
		StepSwitcher->SetActiveWidgetIndex(SwitcherIndex);
		ProgressSwitcher->SetActiveWidgetIndex(SwitcherIndex);
		TabSwitcher->SetActiveWidgetIndex(SwitcherIndex);
		ButtonSwitcher->SetActiveWidgetIndex(SwitcherIndex);
		FString CaptureError;
		if (!Capture(*Widget, SlateWidget, FileNames[Step], CaptureError))
		{
			AddError(CaptureError);
			return false;
		}
	}

	Widget->ResetRewardFlow();
	Widget->AdvanceRewardFlow();
	Widget->OpenRewardChest();
	Widget->AdvanceRewardPresentation(1.f);
	constexpr TCHAR OpenedChestFileName[] =
		TEXT("WBP_RewardConcept03_New_02_ChestOpened.png");
	FString OpenedChestCaptureError;
	if (!Capture(*Widget, SlateWidget, OpenedChestFileName,
		OpenedChestCaptureError))
	{
		AddError(OpenedChestCaptureError);
		return false;
	}

	for (const TCHAR* FileName : FileNames)
	{
		const FString OutputPath = FPaths::Combine(OutputDirectory(), FileName);
		TestTrue(*FString::Printf(TEXT("신규 캡처 생성: %s"), FileName),
			IFileManager::Get().FileExists(*OutputPath));
		TestTrue(*FString::Printf(TEXT("신규 캡처 유효: %s"), FileName),
			IFileManager::Get().FileSize(*OutputPath) > 1024);
	}
	const FString OpenedChestOutputPath = FPaths::Combine(
		OutputDirectory(), OpenedChestFileName);
	TestTrue(TEXT("열린 상자 캡처 생성"),
		IFileManager::Get().FileExists(*OpenedChestOutputPath));
	TestTrue(TEXT("열린 상자 캡처 유효"),
		IFileManager::Get().FileSize(*OpenedChestOutputPath) > 1024);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardConcept03FramelessRenderedCaptureTest,
	"P_RD.UI.RewardConcept03Frameless.RenderedCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardConcept03FramelessRenderedCaptureTest::RunTest(
	const FString& Parameters)
{
	using namespace RewardConcept03NewCapture;
	if (GUsingNullRHI)
	{
		AddInfo(TEXT("NullRHI 환경이라 Frameless RewardConcept03 캡처 생략"));
		return true;
	}
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("Frameless WBP 캡처 월드"), World))
	{
		return false;
	}
	UClass* WidgetClass = LoadClass<URewardConcept03Widget>(
		nullptr, FramelessWidgetClassPath);
	if (!TestNotNull(TEXT("WBP_RewardConcept03_Frameless 클래스"), WidgetClass))
	{
		return false;
	}
	URewardConcept03Widget* Widget = CreateWidget<URewardConcept03Widget>(
		World, WidgetClass);
	if (!TestNotNull(TEXT("WBP_RewardConcept03_Frameless 인스턴스"), Widget))
	{
		return false;
	}

	UWidgetSwitcher* StepSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardStepSwitcher")));
	UWidgetSwitcher* ProgressSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardProgressSwitcher")));
	UWidgetSwitcher* ButtonSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardButtonSwitcher")));
	if (!TestNotNull(TEXT("Frameless 단계 switcher"), StepSwitcher)
		|| !TestNotNull(TEXT("Frameless 진행 switcher"), ProgressSwitcher)
		|| !TestNotNull(TEXT("Frameless 버튼 switcher"), ButtonSwitcher))
	{
		return false;
	}
	TestNull(TEXT("외부 메인 프레임 아트가 생성되지 않음"),
		Widget->GetWidgetFromName(TEXT("NewMainFrameArt")));
	TestNull(TEXT("외부 프레임용 전체 양피지 배경이 생성되지 않음"),
		Widget->GetWidgetFromName(TEXT("NewPanelParchment")));
	TestNull(TEXT("상자 우측 설명 패널이 생성되지 않음"),
		Widget->GetWidgetFromName(TEXT("NewChestPanel")));
	TestNull(TEXT("좌측 상단 단계명 패널이 생성되지 않음"),
		Widget->GetWidgetFromName(TEXT("NewRewardTabPanel")));
	TestNull(TEXT("좌측 상단 단계명 switcher가 생성되지 않음"),
		Widget->GetWidgetFromName(TEXT("NewRewardTabSwitcher")));
	TestNotNull(TEXT("중앙 상자 입력 패널 유지"),
		Widget->GetWidgetFromName(TEXT("NewChestVisualPanel")));
	TestNull(TEXT("골드 우측 설명판 제거"),
		Widget->GetWidgetFromName(TEXT("NewGoldPanel")));
	TestNotNull(TEXT("골드 단계에서 열린 상자 배경 유지"),
		Widget->GetWidgetFromName(TEXT("NewGoldBackgroundChestImage")));
	TestNotNull(TEXT("유지된 상자 전용 블러 레이어"),
		Widget->GetWidgetFromName(TEXT("NewGoldChestBlur")));
	TestNotNull(TEXT("중앙 골드 금액 영역"),
		Widget->GetWidgetFromName(TEXT("NewGoldMainZone")));
	UImage* ChestSequence = Cast<UImage>(
		Widget->GetWidgetFromName(TEXT("NewChestSequenceImage")));
	UImage* ChestBlendSequence = Cast<UImage>(
		Widget->GetWidgetFromName(TEXT("NewChestSequenceBlendImage")));
	if (TestNotNull(TEXT("단일 아틀라스 현재 프레임 레이어"), ChestSequence)
		&& TestNotNull(TEXT("단일 아틀라스 다음 프레임 레이어"),
			ChestBlendSequence))
	{
		TestTrue(TEXT("두 레이어가 같은 상주 텍스처 사용"),
			ChestSequence->GetBrush().GetResourceObject()
			== ChestBlendSequence->GetBrush().GetResourceObject());
	}

	const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	Widget->SetRewardPresentationManualTick(true);
	Widget->AdvanceRewardFlow();
	Widget->OpenRewardChest();
	Widget->AdvanceRewardPresentation(.06f);
	if (ChestSequence != nullptr && ChestBlendSequence != nullptr)
	{
		TestEqual(TEXT("현재 아틀라스 프레임은 불투명하게 표시"),
			ChestSequence->GetRenderOpacity(), 1.f);
		TestEqual(TEXT("보조 크로스페이드 레이어는 사용하지 않음"),
			ChestBlendSequence->GetVisibility(), ESlateVisibility::Collapsed);
		TestEqual(TEXT("보조 크로스페이드 레이어 투명도"),
			ChestBlendSequence->GetRenderOpacity(), 0.f);
	}
	Widget->ResetRewardFlow();
	StepSwitcher->SetActiveWidgetIndex(1);
	ProgressSwitcher->SetActiveWidgetIndex(1);
	ButtonSwitcher->SetActiveWidgetIndex(1);
	FString PrimerError;
	if (!Capture(*Widget, SlateWidget, TEXT("_FramelessWarmup.png"),
		PrimerError, 21))
	{
		AddError(PrimerError);
		return false;
	}
	IFileManager::Get().Delete(*FPaths::Combine(
		OutputDirectory(), TEXT("_FramelessWarmup.png")), false, true);

	const TCHAR* FileNames[] = {
		TEXT("WBP_RewardConcept03_Frameless_01_Experience.png"),
		TEXT("WBP_RewardConcept03_Frameless_02_Chest.png"),
		TEXT("WBP_RewardConcept03_Frameless_03_Gold.png"),
		TEXT("WBP_RewardConcept03_Frameless_04_Artifact.png")
	};
	const int32 CaptureOrder[] = { 1, 2, 3, 0 };
	for (const int32 Step : CaptureOrder)
	{
		const int32 SwitcherIndex = Step + 1;
		StepSwitcher->SetActiveWidgetIndex(SwitcherIndex);
		ProgressSwitcher->SetActiveWidgetIndex(SwitcherIndex);
		ButtonSwitcher->SetActiveWidgetIndex(SwitcherIndex);
		FString CaptureError;
		if (!Capture(*Widget, SlateWidget, FileNames[Step], CaptureError, 21))
		{
			AddError(CaptureError);
			return false;
		}
	}
	for (const TCHAR* FileName : FileNames)
	{
		const FString OutputPath = FPaths::Combine(OutputDirectory(), FileName);
		TestTrue(*FString::Printf(TEXT("Frameless 캡처 생성: %s"), FileName),
			IFileManager::Get().FileExists(*OutputPath));
		TestTrue(*FString::Printf(TEXT("Frameless 캡처 유효: %s"), FileName),
			IFileManager::Get().FileSize(*OutputPath) > 1024);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardConcept03NewNoArtifactInteractionTest,
	"P_RD.UI.RewardConcept03New.NoArtifact.Interaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardConcept03NewNoArtifactInteractionTest::RunTest(
	const FString& Parameters)
{
	// 한글 표시 문자열을 단언하므로 ko 컬처로 고정한다. en/ko 번역이 모두
	// 채워진 뒤로는 실행 컬처에 따라 표시가 달라진다(0823).
	struct FScopedKoreanCulture
	{
		FString mOriginal;
		FScopedKoreanCulture()
			: mOriginal(FInternationalization::Get().GetCurrentCulture()->GetName())
		{
			FInternationalization::Get().SetCurrentCulture(TEXT("ko"));
		}
		~FScopedKoreanCulture()
		{
			FInternationalization::Get().SetCurrentCulture(mOriginal);
		}
	};
	const FScopedKoreanCulture ScopedKoreanCulture;
	using namespace RewardConcept03NewCapture;
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("3단계 WBP 상호작용 월드"), World))
	{
		return false;
	}
	UClass* WidgetClass = LoadClass<URewardConcept03NoArtifactWidget>(
		nullptr, NoArtifactWidgetClassPath);
	if (!TestNotNull(TEXT("3단계 보상 위젯 클래스"), WidgetClass))
	{
		return false;
	}
	URewardConcept03NoArtifactWidget* Widget =
		CreateWidget<URewardConcept03NoArtifactWidget>(World, WidgetClass);
	if (!TestNotNull(TEXT("3단계 보상 위젯 인스턴스"), Widget))
	{
		return false;
	}
	TestFalse(TEXT("3단계 변형은 아티팩트 보상 없음"),
		Widget->HasArtifactReward());
	TestNull(TEXT("3단계 변형은 아티팩트 단계 미생성"),
		Widget->GetWidgetFromName(TEXT("NewArtifactStep")));
	TestNull(TEXT("3단계 변형은 아티팩트 버튼 미생성"),
		Widget->GetWidgetFromName(TEXT("NewArtifactChoiceButton_0")));
	UWidgetSwitcher* Steps = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardStepSwitcher")));
	if (!TestNotNull(TEXT("3단계 switcher"), Steps))
	{
		return false;
	}
	TestEqual(TEXT("warmup 포함 3단계 자식 수"), Steps->GetNumWidgets(), 4);
	UTextBlock* GoldActionText = Cast<UTextBlock>(
		Widget->GetWidgetFromName(TEXT("NewButtonText_3")));
	if (TestNotNull(TEXT("골드 단계 확정 문구"), GoldActionText))
	{
		TestEqual(TEXT("3단계 마지막 버튼은 확정"),
			GoldActionText->GetText().ToString(), FString(TEXT("확정")));
	}

	Widget->AdvanceRewardFlow();
	TestEqual(TEXT("3단계 상자 단계 이동"), Widget->GetCurrentStepIndex(), 1);
	Widget->AdvanceRewardFlow();
	TestEqual(TEXT("3단계 상자 개봉 전 진행 차단"),
		Widget->GetCurrentStepIndex(), 1);
	Widget->OpenRewardChest();
	Widget->SkipRewardPresentation();
	TestEqual(TEXT("3단계 골드 단계 자동 이동"),
		Widget->GetCurrentStepIndex(), 2);
	TestTrue(TEXT("3단계 골드 연출 재생"),
		Widget->IsRewardPresentationPlaying());
	Widget->SkipRewardPresentation();
	TestFalse(TEXT("무아티팩트는 골드 연출 뒤 확정 대기"),
		Widget->IsRewardPresentationPlaying());
	Widget->AdvanceRewardFlow();
	TestTrue(TEXT("골드에서 바로 3단계 흐름 완료"),
		Widget->IsRewardFlowCompleted());
	TestEqual(TEXT("완료 뒤 골드 단계 유지"), Widget->GetCurrentStepIndex(), 2);
	TestEqual(TEXT("아티팩트 선택값 없음"),
		Widget->GetSelectedArtifactIndex(), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardConcept03NewNoArtifactRenderedCaptureTest,
	"P_RD.UI.RewardConcept03New.NoArtifact.RenderedCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardConcept03NewNoArtifactRenderedCaptureTest::RunTest(
	const FString& Parameters)
{
	using namespace RewardConcept03NewCapture;
	if (GUsingNullRHI)
	{
		AddInfo(TEXT("NullRHI 환경이라 3단계 RewardConcept03 캡처 생략"));
		return true;
	}
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("3단계 WBP 캡처 월드"), World))
	{
		return false;
	}
	UClass* WidgetClass = LoadClass<URewardConcept03NoArtifactWidget>(
		nullptr, NoArtifactWidgetClassPath);
	if (!TestNotNull(TEXT("3단계 WBP 클래스"), WidgetClass))
	{
		return false;
	}
	URewardConcept03NoArtifactWidget* Widget =
		CreateWidget<URewardConcept03NoArtifactWidget>(World, WidgetClass);
	if (!TestNotNull(TEXT("3단계 WBP 인스턴스"), Widget))
	{
		return false;
	}
	UWidgetSwitcher* StepSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardStepSwitcher")));
	UWidgetSwitcher* ProgressSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardProgressSwitcher")));
	UWidgetSwitcher* TabSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardTabSwitcher")));
	UWidgetSwitcher* ButtonSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("NewRewardButtonSwitcher")));
	if (!TestNotNull(TEXT("3단계 콘텐츠 switcher"), StepSwitcher)
		|| !TestNotNull(TEXT("3단계 진행 switcher"), ProgressSwitcher)
		|| !TestNotNull(TEXT("3단계 탭 switcher"), TabSwitcher)
		|| !TestNotNull(TEXT("3단계 버튼 switcher"), ButtonSwitcher))
	{
		return false;
	}
	for (UWidgetSwitcher* Switcher : {
		StepSwitcher, ProgressSwitcher, TabSwitcher, ButtonSwitcher })
	{
		TestEqual(TEXT("3단계 switcher의 warmup 포함 자식 수"),
			Switcher->GetNumWidgets(), 4);
		Switcher->SetActiveWidgetIndex(1);
	}

	const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	for (const TCHAR* WarmupName : {
		TEXT("_NoArtifactWarmup.png"), TEXT("_NoArtifactWarmup2.png") })
	{
		FString CaptureError;
		if (!Capture(*Widget, SlateWidget, WarmupName, CaptureError, 21))
		{
			AddError(CaptureError);
			return false;
		}
		IFileManager::Get().Delete(*FPaths::Combine(
			OutputDirectory(), WarmupName), false, true);
	}

	const TCHAR* FileNames[] = {
		TEXT("WBP_RewardConcept03_New_NoArtifact_01_Experience.png"),
		TEXT("WBP_RewardConcept03_New_NoArtifact_02_Chest.png"),
		TEXT("WBP_RewardConcept03_New_NoArtifact_03_Gold.png")
	};
	const int32 CaptureOrder[] = { 1, 2, 0 };
	for (const int32 Step : CaptureOrder)
	{
		const int32 SwitcherIndex = Step + 1;
		StepSwitcher->SetActiveWidgetIndex(SwitcherIndex);
		ProgressSwitcher->SetActiveWidgetIndex(SwitcherIndex);
		TabSwitcher->SetActiveWidgetIndex(SwitcherIndex);
		ButtonSwitcher->SetActiveWidgetIndex(SwitcherIndex);
		FString CaptureError;
		if (!Capture(*Widget, SlateWidget, FileNames[Step], CaptureError, 21))
		{
			AddError(CaptureError);
			return false;
		}
	}
	for (const TCHAR* FileName : FileNames)
	{
		const FString OutputPath = FPaths::Combine(OutputDirectory(), FileName);
		TestTrue(*FString::Printf(TEXT("3단계 캡처 생성: %s"), FileName),
			IFileManager::Get().FileExists(*OutputPath));
		TestTrue(*FString::Printf(TEXT("3단계 캡처 유효: %s"), FileName),
			IFileManager::Get().FileSize(*OutputPath) > 1024);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
