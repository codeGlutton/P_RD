#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UI/CombatResultOverlayWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr TCHAR DefeatWidgetPath[] =
		TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C");

	UWorld* FindAutomationWorld()
	{
		if (GEngine == nullptr)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() != nullptr)
			{
				return Context.World();
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDefeatWBPStructureTest,
	"P_RD.UI.CombatDefeat.Structure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDefeatWBPStructureTest::RunTest(const FString& Parameters)
{
	UClass* WidgetClass = LoadClass<UCombatResultOverlayWidget>(nullptr, DefeatWidgetPath);
	if (!TestNotNull(TEXT("패배 결과 WBP 클래스"), WidgetClass))
	{
		return false;
	}
	TestTrue(TEXT("패배 WBP는 결과 오버레이 C++ 클래스를 상속한다"),
		WidgetClass->IsChildOf(UCombatResultOverlayWidget::StaticClass()));

	UWidgetBlueprintGeneratedClass* Generated = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass);
	UWidgetTree* Tree = Generated != nullptr ? Generated->GetWidgetTreeArchetype() : nullptr;
	if (!TestNotNull(TEXT("패배 WBP 위젯 트리"), Tree))
	{
		return false;
	}

	TestNotNull(TEXT("모바일 해상도 대응 ScaleBox"),
		Cast<UScaleBox>(Tree->FindWidget(TEXT("DefeatResponsiveScale"))));
	TestNotNull(TEXT("타이틀 이동 버튼"),
		Cast<UButton>(Tree->FindWidget(TEXT("mTitleButton"))));
	TestNull(TEXT("로그라이크 패배판에는 재도전 버튼 없음"),
		Tree->FindWidget(TEXT("mRetryButton")));
	TestNull(TEXT("로그라이크 패배판에는 재도전 버튼 아트 없음"),
		Tree->FindWidget(TEXT("mRetryButtonArt")));
	TestNull(TEXT("로그라이크 패배판에는 재도전 버튼 텍스트 없음"),
		Tree->FindWidget(TEXT("mRetryButtonText")));
	for (const TCHAR* Name : {
		TEXT("mLocationText"), TEXT("mRoundText"), TEXT("mEnemyText"),
		TEXT("mGoldText"), TEXT("mExpText") })
	{
		TestNotNull(Name, Cast<UTextBlock>(Tree->FindWidget(FName(Name))));
	}

	// Every label is horizontally justified and vertically arranged at the center
	// of its explicit design rectangle. This catches the common UTextBlock failure
	// mode where a tall Canvas slot makes otherwise centered text sit at the top.
	Tree->ForEachWidget([this](UWidget* Widget)
	{
		UTextBlock* Text = Cast<UTextBlock>(Widget);
		if (Text == nullptr)
		{
			return;
		}
		TestTrue(*FString::Printf(TEXT("%s 순백색 글자"), *Text->GetName()),
			Text->GetColorAndOpacity().GetSpecifiedColor().Equals(FLinearColor::White));
		const FByteProperty* JustificationProperty = CastField<FByteProperty>(
			UTextLayoutWidget::StaticClass()->FindPropertyByName(TEXT("Justification")));
		if (TestNotNull(*FString::Printf(TEXT("%s 정렬 프로퍼티"), *Text->GetName()),
			JustificationProperty))
		{
			const uint8 Justification = JustificationProperty->GetPropertyValue_InContainer(Text);
			TestEqual(*FString::Printf(TEXT("%s 가로 중앙 정렬"), *Text->GetName()),
				Justification, static_cast<uint8>(ETextJustify::Center));
		}
		UOverlay* CenterMount = Cast<UOverlay>(Text->GetParent());
		UOverlaySlot* CenterSlot = Cast<UOverlaySlot>(Text->Slot);
		if (TestNotNull(*FString::Printf(TEXT("%s 중앙 마운트"), *Text->GetName()),
			CenterMount)
			&& TestNotNull(*FString::Printf(TEXT("%s 중앙 슬롯"), *Text->GetName()),
				CenterSlot))
		{
			TestEqual(*FString::Printf(TEXT("%s 가로 Fill"), *Text->GetName()),
				CenterSlot->GetHorizontalAlignment(), HAlign_Fill);
			TestEqual(*FString::Printf(TEXT("%s 세로 중앙"), *Text->GetName()),
				CenterSlot->GetVerticalAlignment(), VAlign_Center);
		}
	});

	UTextBlock* TitleButtonText = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("mTitleButtonText")));
	if (TestNotNull(TEXT("타이틀 버튼 텍스트"), TitleButtonText))
	{
		const FSlateFontInfo& TitleFont = TitleButtonText->GetFont();
		TestTrue(TEXT("타이틀 버튼 인게임 표준 폰트"),
			TitleFont.FontObject != nullptr
			&& TitleFont.FontObject->GetPathName()
				== TEXT("/Game/SVN/OutSideAsset/Fonts/F_HUD_Oswald.F_HUD_Oswald"));
		TestEqual(TEXT("타이틀 버튼 Bold Typeface"),
			TitleFont.TypefaceFontName, FName(TEXT("Bold")));
		TestEqual(TEXT("타이틀 버튼 30px"), TitleFont.Size, 30.f);
		TestEqual(TEXT("타이틀 버튼 외곽선 1px"),
			TitleFont.OutlineSettings.OutlineSize, 1);
		TestTrue(TEXT("타이틀 버튼 표준 외곽선 색"),
			TitleFont.OutlineSettings.OutlineColor.Equals(
				FLinearColor(.03f, .015f, .005f, 1.f)));
		TestTrue(TEXT("타이틀 버튼 순백색 글자"),
			TitleButtonText->GetColorAndOpacity().GetSpecifiedColor().Equals(
				FLinearColor::White));
		TestTrue(TEXT("타이틀 버튼 표준 그림자 위치"),
			TitleButtonText->GetShadowOffset().Equals(FVector2D(1.5f, 1.5f)));
		TestTrue(TEXT("타이틀 버튼 표준 그림자 색"),
			TitleButtonText->GetShadowColorAndOpacity().Equals(
				FLinearColor(0.f, 0.f, 0.f, .62f)));
	}

	int32 ButtonCount = 0;
	Tree->ForEachWidget([&ButtonCount](UWidget* Widget)
	{
		ButtonCount += Cast<UButton>(Widget) != nullptr ? 1 : 0;
	});
	TestEqual(TEXT("패배판 단일 CTA"), ButtonCount, 1);

	UButton* TitleButton = Cast<UButton>(Tree->FindWidget(TEXT("mTitleButton")));
	UImage* TitleButtonArt = Cast<UImage>(Tree->FindWidget(TEXT("mTitleButtonArt")));
	UCanvasPanelSlot* ButtonSlot = TitleButton != nullptr
		? Cast<UCanvasPanelSlot>(TitleButton->Slot) : nullptr;
	UCanvasPanelSlot* TextMountSlot = TitleButtonText != nullptr
		&& TitleButtonText->GetParent() != nullptr
		? Cast<UCanvasPanelSlot>(TitleButtonText->GetParent()->Slot) : nullptr;
	UCanvasPanelSlot* ArtSlot = TitleButtonArt != nullptr
		? Cast<UCanvasPanelSlot>(TitleButtonArt->Slot) : nullptr;
	const FVector2D ExpectedButtonPosition(558.f, 704.f);
	const FVector2D ExpectedButtonSize(420.f, 160.f);
	if (TestNotNull(TEXT("타이틀 버튼 클릭 슬롯"), ButtonSlot)
		&& TestNotNull(TEXT("타이틀 버튼 텍스트 마운트 슬롯"), TextMountSlot)
		&& TestNotNull(TEXT("타이틀 버튼 아트 슬롯"), ArtSlot))
	{
		TestTrue(TEXT("타이틀 버튼 하단 중앙 위치"),
			ButtonSlot->GetPosition().Equals(ExpectedButtonPosition, .01));
		TestTrue(TEXT("타이틀 버튼 충분한 클릭 크기"),
			ButtonSlot->GetSize().Equals(ExpectedButtonSize, .01));
		TestTrue(TEXT("타이틀 버튼 디자인 중앙축 정렬"),
			FMath::IsNearlyEqual(
				ButtonSlot->GetPosition().X + ButtonSlot->GetSize().X * .5f,
				768.f, .01f));
		TestTrue(TEXT("타이틀 텍스트 위치=클릭 위치"),
			TextMountSlot->GetPosition().Equals(ButtonSlot->GetPosition(), .01));
		TestTrue(TEXT("타이틀 텍스트 크기=클릭 크기"),
			TextMountSlot->GetSize().Equals(ButtonSlot->GetSize(), .01));
		TestTrue(TEXT("타이틀 아트는 클릭영역 안"),
			ArtSlot->GetSize().X <= ButtonSlot->GetSize().X + .01f
			&& ArtSlot->GetSize().Y <= ButtonSlot->GetSize().Y + .01f);
		TestTrue(TEXT("타이틀 아트와 클릭영역 중심 일치"),
			(ArtSlot->GetPosition() + ArtSlot->GetSize() * .5f).Equals(
				ButtonSlot->GetPosition() + ButtonSlot->GetSize() * .5f, .01));

		UTexture2D* TitleButtonTexture = Cast<UTexture2D>(
			TitleButtonArt->GetBrush().GetResourceObject());
		if (TestNotNull(TEXT("타이틀 버튼 아트 텍스처"), TitleButtonTexture))
		{
			const FIntPoint NativeSize = TitleButtonTexture->GetImportedSize();
			if (TestTrue(TEXT("타이틀 버튼 아트 원본 크기 유효"),
				NativeSize.X > 0 && NativeSize.Y > 0))
			{
				const double NativeRatio =
					static_cast<double>(NativeSize.X) / NativeSize.Y;
				const FVector2D PlacedSize = ArtSlot->GetSize();
				TestTrue(TEXT("타이틀 버튼 아트 원본 비율 유지"),
					FMath::IsNearlyEqual(
						PlacedSize.X / PlacedSize.Y, NativeRatio, .001));
			}
		}
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FString MountName = FString::Printf(TEXT("DefeatCardFrame_%dMount"), Index);
		const FString FrameName = FString::Printf(TEXT("DefeatCardFrame_%d"), Index);
		const FString PortraitName = FString::Printf(TEXT("mPartyPortrait%d"), Index);
		UCanvasPanel* Mount = Cast<UCanvasPanel>(Tree->FindWidget(FName(*MountName)));
		UImage* Frame = Cast<UImage>(Tree->FindWidget(FName(*FrameName)));
		UImage* Portrait = Cast<UImage>(Tree->FindWidget(FName(*PortraitName)));
		TestNotNull(*MountName, Mount);
		TestNotNull(*FrameName, Frame);
		TestNotNull(*PortraitName, Portrait);
		if (Mount != nullptr && Frame != nullptr && Portrait != nullptr)
		{
			TestTrue(*FString::Printf(TEXT("%s가 카드 마운트 하위"), *FrameName),
				Frame->GetParent() == Mount);
			TestTrue(*FString::Printf(TEXT("%s가 카드 마운트 하위"), *PortraitName),
				Portrait->GetParent() == Mount);
		}
	}

	struct FExpectedImageTexture
	{
		const TCHAR* WidgetName;
		const TCHAR* TexturePath;
	};
	const FExpectedImageTexture ExpectedImages[] =
	{
		{ TEXT("DefeatOuterFrame"),
			TEXT("/Game/UI/ResultBoards/Art/T_DF_BoardBlank_0809.T_DF_BoardBlank_0809") },
		{ TEXT("DefeatTitleBanner"),
			TEXT("/Game/UI/ResultBoards/Art/T_DF_RibbonBlank_0809.T_DF_RibbonBlank_0809") },
		{ TEXT("DefeatCardFrame_0"),
			TEXT("/Game/UI/ResultBoards/Art/T_DF_PortraitCardBlank_0809.T_DF_PortraitCardBlank_0809") },
		{ TEXT("mTitleButtonArt"),
			TEXT("/Game/UI/ResultBoards/Art/T_UI_ButtonSecondaryBlank_0809.T_UI_ButtonSecondaryBlank_0809") },
	};
	for (const FExpectedImageTexture& Expected : ExpectedImages)
	{
		const FString WidgetLabel = FString::Printf(
			TEXT("%s 이미지"), Expected.WidgetName);
		UImage* Image = Cast<UImage>(Tree->FindWidget(FName(Expected.WidgetName)));
		if (!TestNotNull(*WidgetLabel, Image))
		{
			continue;
		}
		UTexture2D* Texture = Cast<UTexture2D>(Image->GetBrush().GetResourceObject());
		const FString TextureLabel = FString::Printf(
			TEXT("%s 결과판 텍스처"), Expected.WidgetName);
		if (TestNotNull(*TextureLabel, Texture))
		{
			TestEqual(*TextureLabel, Texture->GetPathName(), FString(Expected.TexturePath));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDefeatWBPInteractionTest,
	"P_RD.UI.CombatDefeat.Interaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDefeatWBPInteractionTest::RunTest(const FString& Parameters)
{
	UWorld* World = FindAutomationWorld();
	if (World == nullptr)
	{
		AddInfo(TEXT("위젯 생성 월드가 없어 런타임 상호작용 검사를 건너뜀"));
		return true;
	}

	UClass* WidgetClass = LoadClass<UCombatResultOverlayWidget>(nullptr, DefeatWidgetPath);
	UCombatResultOverlayWidget* Widget = WidgetClass != nullptr
		? CreateWidget<UCombatResultOverlayWidget>(World, WidgetClass) : nullptr;
	if (!TestNotNull(TEXT("패배 결과 WBP 인스턴스"), Widget))
	{
		return false;
	}

	int32 TitleClicks = 0;
	FCombatResultUI Result;
	Result.mLocationName = FText::FromString(TEXT("잊힌 성채"));
	Result.mRound = 7;
	Result.mDefeatedMonsterCount = 12;
	const TCHAR* PortraitPaths[] = {
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"),
	};
	TArray<TObjectPtr<UTexture2D>> ExpectedPortraits;
	for (const TCHAR* PortraitPath : PortraitPaths)
	{
		UTexture2D* Portrait = LoadObject<UTexture2D>(nullptr, PortraitPath);
		TestNotNull(PortraitPath, Portrait);
		ExpectedPortraits.Add(Portrait);
		Result.mPartyPortraits.Add(Portrait);
	}
	Widget->ShowDefeatResult(Result,
		FSimpleDelegate::CreateLambda([&TitleClicks]() { ++TitleClicks; }));
	Widget->OpenUI();
	Widget->TakeWidget();
	// NullRHI 에디터 월드에서는 TakeWidget 뒤에 디자이너 트리가 확정된다.
	// 실제 HUD의 OpenRequested와 같은 시점으로 데이터를 한 번 더 밀어 넣는다.
	Widget->ShowDefeatResult(Result,
		FSimpleDelegate::CreateLambda([&TitleClicks]() { ++TitleClicks; }));
	Widget->WidgetTree->ForEachWidget([this](UWidget* Child)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Child))
		{
			TestTrue(*FString::Printf(TEXT("%s 런타임 순백색 글자"), *Text->GetName()),
				Text->GetColorAndOpacity().GetSpecifiedColor().Equals(FLinearColor::White));
		}
	});

	UButton* TitleButton = Cast<UButton>(Widget->WidgetTree->FindWidget(TEXT("mTitleButton")));
	if (!TestNotNull(TEXT("타이틀 버튼 인스턴스"), TitleButton))
	{
		return false;
	}
	TestTrue(TEXT("타이틀 버튼 클릭 동작 연결"), TitleButton->OnClicked.IsBound());
	TitleButton->OnClicked.Broadcast();
	TestEqual(TEXT("타이틀 콜백은 한 번 실행"), TitleClicks, 1);

	UTextBlock* LocationText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("mLocationText")));
	UTextBlock* RoundText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("mRoundText")));
	UTextBlock* EnemyText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("mEnemyText")));
	if (TestNotNull(TEXT("지역·라운드 통합 텍스트 인스턴스"), LocationText))
	{
		const FString LocationLine = LocationText->GetText().ToString();
		TestTrue(TEXT("지역명 반영"), LocationLine.Contains(TEXT("잊힌 성채")));
		TestTrue(TEXT("7라운드 값 반영"), LocationLine.Contains(TEXT("7")));
	}
	if (TestNotNull(TEXT("라운드 텍스트 인스턴스"), RoundText))
	{
		TestEqual(TEXT("구 라운드 단독 텍스트 숨김"), RoundText->GetVisibility(),
			ESlateVisibility::Collapsed);
	}
	if (TestNotNull(TEXT("몬스터 텍스트 인스턴스"), EnemyText))
	{
		TestTrue(TEXT("처치 수 12 반영"), EnemyText->GetText().ToString().Contains(TEXT("12")));
	}
	UTextBlock* SurvivorText = Cast<UTextBlock>(
		Widget->WidgetTree->FindWidget(TEXT("DefeatSurvivorValue")));
	if (TestNotNull(TEXT("생존 텍스트 인스턴스"), SurvivorText))
	{
		TestEqual(TEXT("전멸 파티 3명 반영"), SurvivorText->GetText().ToString(),
			FString(TEXT("0 / 3")));
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FString MountName = FString::Printf(TEXT("DefeatCardFrame_%dMount"), Index);
		const FString PortraitName = FString::Printf(TEXT("mPartyPortrait%d"), Index);
		UWidget* Mount = Widget->WidgetTree->FindWidget(FName(*MountName));
		UImage* PortraitImage = Cast<UImage>(
			Widget->WidgetTree->FindWidget(FName(*PortraitName)));
		if (TestNotNull(*MountName, Mount))
		{
			TestEqual(*FString::Printf(TEXT("파티 카드 %d 표시"), Index),
				Mount->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		}
		if (TestNotNull(*PortraitName, PortraitImage))
		{
			TestEqual(*FString::Printf(TEXT("파티 초상화 %d 표시"), Index),
				PortraitImage->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
			TestTrue(*FString::Printf(TEXT("파티 초상화 %d 브러시 반영"), Index),
				ExpectedPortraits.IsValidIndex(Index)
				&& PortraitImage->GetBrush().GetResourceObject() == ExpectedPortraits[Index].Get());
		}
	}

	// 실제 파티 초상은 정사각형이라는 보장이 없다. 넓은 결과판 텍스처를
	// 대입해도 카드의 136x136 허용영역 안에서 원본 비율로 center-fit되는지
	// 검증한다. SetBrushFromTexture만 호출하면 이 경우 정사각형으로 찌그러진다.
	UTexture2D* WidePortrait = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/UI/ResultBoards/Art/T_VR_HeaderBlank_0809.T_VR_HeaderBlank_0809"));
	if (TestNotNull(TEXT("비정방형 초상화 검사 텍스처"), WidePortrait))
	{
		FCombatResultUI WidePortraitResult = Result;
		WidePortraitResult.mPartyPortraits.Reset();
		WidePortraitResult.mPartyPortraits.Add(WidePortrait);
		Widget->ShowDefeatResult(WidePortraitResult, FSimpleDelegate());

		UImage* PortraitImage = Cast<UImage>(
			Widget->WidgetTree->FindWidget(TEXT("mPartyPortrait0")));
		UCanvasPanelSlot* PortraitSlot = PortraitImage != nullptr
			? Cast<UCanvasPanelSlot>(PortraitImage->Slot) : nullptr;
		if (TestNotNull(TEXT("비정방형 초상화 캔버스 슬롯"), PortraitSlot))
		{
			const FIntPoint ImportedSize = WidePortrait->GetImportedSize();
			const FVector2D PlacedSize = PortraitSlot->GetSize();
			const double NativeRatio =
				static_cast<double>(ImportedSize.X) / ImportedSize.Y;
			const double PlacedRatio = PlacedSize.X / PlacedSize.Y;
			TestTrue(TEXT("비정방형 초상화 원본 비율 유지"),
				FMath::IsNearlyEqual(PlacedRatio, NativeRatio, 0.001));
			TestTrue(TEXT("비정방형 초상화가 카드 허용영역 안에 배치"),
				PlacedSize.X <= 136.0 + KINDA_SMALL_NUMBER
				&& PlacedSize.Y <= 136.0 + KINDA_SMALL_NUMBER);
		}
	}

	FCombatResultUI EmptyPartyResult = Result;
	EmptyPartyResult.mPartyPortraits.Reset();
	Widget->ShowDefeatResult(EmptyPartyResult, FSimpleDelegate());
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FString MountName = FString::Printf(TEXT("DefeatCardFrame_%dMount"), Index);
		UWidget* Mount = Widget->WidgetTree->FindWidget(FName(*MountName));
		if (TestNotNull(*MountName, Mount))
		{
			TestEqual(*FString::Printf(TEXT("빈 파티 카드 %d 접힘"), Index),
				Mount->GetVisibility(), ESlateVisibility::Collapsed);
		}
	}
	Widget->CloseUI();
	return true;
}

#endif
