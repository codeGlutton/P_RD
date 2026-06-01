#include "UI/TitleMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"

namespace
{
	constexpr int32 MainScreenIndex = 0;
	constexpr int32 CharacterScreenIndex = 1;
	constexpr int32 SettingsScreenIndex = 2;
	constexpr int32 MapScreenIndex = 3;

	struct FDefaultCharacterView
	{
		const TCHAR* Name;
		const TCHAR* Stat;
		const TCHAR* Description;
	};

	const FDefaultCharacterView DefaultCharacters[] = {
		{ TEXT("Warrior"), TEXT("HP 30 | Dice 3 | Gold 50"), TEXT("Balanced starter hero.") },
		{ TEXT("Archer"), TEXT("HP 22 | Dice 4 | Gold 40"), TEXT("Preview slot for ranged play.") },
		{ TEXT("Mage"), TEXT("HP 18 | Dice 5 | Gold 35"), TEXT("Preview slot for spell play.") },
	};

	constexpr float MapGraphWidth = 1040.f;
	constexpr float MapGraphHeight = 520.f;
	constexpr float MapNodeWidth = 150.f;
	constexpr float MapNodeHeight = 66.f;
	constexpr float MapGraphSidePadding = 68.f;
	constexpr float MapGraphTopPadding = 42.f;

	FLinearColor GetMapRoomTypeColor(ERoomType RoomType)
	{
		switch (RoomType)
		{
		case ERoomType::Monster:
			return FLinearColor(0.210f, 0.430f, 0.470f, 1.f);
		case ERoomType::EliteMonster:
			return FLinearColor(0.550f, 0.355f, 0.190f, 1.f);
		case ERoomType::BossMonster:
			return FLinearColor(0.540f, 0.140f, 0.145f, 1.f);
		case ERoomType::Shop:
			return FLinearColor(0.205f, 0.390f, 0.245f, 1.f);
		case ERoomType::Treasure:
			return FLinearColor(0.590f, 0.460f, 0.180f, 1.f);
		default:
			return FLinearColor(0.250f, 0.285f, 0.305f, 1.f);
		}
	}

	FVector2D GetMapRoomNodeCenter(const TArray<FTitleMapRoomView>& Rooms, const FTitleMapRoomView& Room)
	{
		int32 MaxRow = 0;
		int32 MaxColumn = 0;
		for (const FTitleMapRoomView& Candidate : Rooms)
		{
			MaxRow = FMath::Max(MaxRow, Candidate.mRow);
			MaxColumn = FMath::Max(MaxColumn, Candidate.mColumn);
		}

		const float GraphRight = MapGraphWidth - MapGraphSidePadding;
		const float GraphBottom = MapGraphHeight - MapGraphTopPadding;
		const float X = MaxColumn <= 0
			? MapGraphWidth * 0.5f
			: MapGraphSidePadding + (static_cast<float>(Room.mColumn) / static_cast<float>(MaxColumn)) * (GraphRight - MapGraphSidePadding);
		const float Y = MaxRow <= 0
			? MapGraphHeight * 0.5f
			: MapGraphTopPadding + (static_cast<float>(MaxRow - Room.mRow) / static_cast<float>(MaxRow)) * (GraphBottom - MapGraphTopPadding);

		const FVector2D Offset(Room.mPositionOffsetRate.X * 20.f, Room.mPositionOffsetRate.Y * 12.f);
		return FVector2D(
			FMath::Clamp(X + Offset.X, MapNodeWidth * 0.5f, MapGraphWidth - MapNodeWidth * 0.5f),
			FMath::Clamp(Y + Offset.Y, MapNodeHeight * 0.5f, MapGraphHeight - MapNodeHeight * 0.5f));
	}

	int32 GetScreenIndex(UTitleMenuWidget::ETitleScreen Screen)
	{
		switch (Screen)
		{
		case UTitleMenuWidget::ETitleScreen::CharacterSelect:
			return CharacterScreenIndex;
		case UTitleMenuWidget::ETitleScreen::Settings:
			return SettingsScreenIndex;
		case UTitleMenuWidget::ETitleScreen::Map:
			return MapScreenIndex;
		default:
			return MainScreenIndex;
		}
	}
}

UTitleMenuWidget::UTitleMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, mTitleText(NSLOCTEXT("TitleMenuWidget", "TitleText", "Rogue The Dice"))
	, mStartText(NSLOCTEXT("TitleMenuWidget", "StartText", "START"))
	, mContinueText(NSLOCTEXT("TitleMenuWidget", "ContinueText", "CONTINUE"))
	, mSettingsText(NSLOCTEXT("TitleMenuWidget", "SettingsText", "SETTING"))
	, mCloseText(NSLOCTEXT("TitleMenuWidget", "CloseText", "CLOSE"))
	, mSaveRunText(NSLOCTEXT("TitleMenuWidget", "SaveRunText", "SAVE RUN"))
	, mAbandonRunText(NSLOCTEXT("TitleMenuWidget", "AbandonRunText", "ABANDON RUN"))
	, mRunControlText(NSLOCTEXT("TitleMenuWidget", "RunControlText", "RUN CONTROL"))
	, mCharacterSelectText(NSLOCTEXT("TitleMenuWidget", "CharacterSelectText", "SELECT HERO"))
	, mConfirmText(NSLOCTEXT("TitleMenuWidget", "ConfirmText", "CONFIRM"))
	, mBackText(NSLOCTEXT("TitleMenuWidget", "BackText", "BACK"))
	, mMapText(NSLOCTEXT("TitleMenuWidget", "MapText", "WORLD MAP"))
	, mEnterText(NSLOCTEXT("TitleMenuWidget", "EnterText", "ENTER"))
	, mReadyStatusText(FText::GetEmpty())
	, mSettingsHintText(NSLOCTEXT("TitleMenuWidget", "SettingsHintText", "Language preview only. Run controls appear after a run exists."))
	, mLanguageText(NSLOCTEXT("TitleMenuWidget", "LanguageText", "LANGUAGE"))
	, mEnglishText(NSLOCTEXT("TitleMenuWidget", "EnglishText", "ENGLISH"))
	, mKoreanText(NSLOCTEXT("TitleMenuWidget", "KoreanText", "KOREAN"))
	, mLanguageAppliedText(NSLOCTEXT("TitleMenuWidget", "LanguageAppliedText", "Language preview updated."))
	, mMapReadyStatusText(NSLOCTEXT("TitleMenuWidget", "MapReadyStatusText", "Map preview ready"))
{
	SetVisibility(ESlateVisibility::Visible);
}

void UTitleMenuWidget::OpenCharacterSelectFromTitle()
{
	SwitchScreen(ETitleScreen::CharacterSelect);
}

void UTitleMenuWidget::OpenMapFromTopBar()
{
	SwitchScreen(ETitleScreen::Map);
}

void UTitleMenuWidget::OpenSettingsFromTopBar()
{
	mPreviousScreenBeforeSettings = mCurrentScreen;
	SwitchScreen(ETitleScreen::Settings);
}

void UTitleMenuWidget::SetRunControlsVisible(bool bVisible)
{
	bHasActiveRun = bVisible;
	SetWidgetVisible(SaveRunButton, bVisible);
	SetWidgetVisible(AbandonRunButton, bVisible);
	RefreshSettingsText();
}

void UTitleMenuWidget::SetMapRoomViews(const TArray<FTitleMapRoomView>& RoomViews, bool bInHasActiveRun)
{
	bHasActiveRun = bInHasActiveRun;
	mMapRoomViews = RoomViews;
	SetRunControlsVisible(bInHasActiveRun);
	RefreshMapText();
	RefreshMapGraph();
	RefreshMapRoomList();
}

void UTitleMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StartButton != nullptr)
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleStartButtonClicked);
	}
	if (ContinueButton != nullptr)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleContinueButtonClicked);
	}
	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsButtonClicked);
	}
	if (BackToMainButton != nullptr)
	{
		BackToMainButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleBackToMainButtonClicked);
	}
	if (SettingsBackButton != nullptr)
	{
		SettingsBackButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleBackToMainButtonClicked);
	}
	if (ConfirmButton != nullptr)
	{
		ConfirmButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleConfirmButtonClicked);
	}
	if (SaveRunButton != nullptr)
	{
		SaveRunButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSaveRunButtonClicked);
	}
	if (AbandonRunButton != nullptr)
	{
		AbandonRunButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleAbandonRunButtonClicked);
	}
	if (EnglishLanguageButton != nullptr)
	{
		EnglishLanguageButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleEnglishLanguageButtonClicked);
	}
	if (KoreanLanguageButton != nullptr)
	{
		KoreanLanguageButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleKoreanLanguageButtonClicked);
	}
	if (CharacterButton0 != nullptr)
	{
		CharacterButton0->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleCharacterButton0Clicked);
	}
	if (CharacterButton1 != nullptr)
	{
		CharacterButton1->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleCharacterButton1Clicked);
	}
	if (CharacterButton2 != nullptr)
	{
		CharacterButton2->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleCharacterButton2Clicked);
	}
	if (BackToCharacterButton != nullptr)
	{
		BackToCharacterButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleBackToCharacterButtonClicked);
	}
	if (EnterRoomButton != nullptr)
	{
		EnterRoomButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleEnterRoomButtonClicked);
	}

	SyncText();
	SetRunControlsVisible(false);
	SwitchScreen(ETitleScreen::Main);
}

void UTitleMenuWidget::NativeDestruct()
{
	if (StartButton != nullptr)
	{
		StartButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleStartButtonClicked);
	}
	if (ContinueButton != nullptr)
	{
		ContinueButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleContinueButtonClicked);
	}
	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleSettingsButtonClicked);
	}
	if (BackToMainButton != nullptr)
	{
		BackToMainButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleBackToMainButtonClicked);
	}
	if (SettingsBackButton != nullptr)
	{
		SettingsBackButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleBackToMainButtonClicked);
	}
	if (ConfirmButton != nullptr)
	{
		ConfirmButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleConfirmButtonClicked);
	}
	if (SaveRunButton != nullptr)
	{
		SaveRunButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleSaveRunButtonClicked);
	}
	if (AbandonRunButton != nullptr)
	{
		AbandonRunButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleAbandonRunButtonClicked);
	}
	if (EnglishLanguageButton != nullptr)
	{
		EnglishLanguageButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleEnglishLanguageButtonClicked);
	}
	if (KoreanLanguageButton != nullptr)
	{
		KoreanLanguageButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleKoreanLanguageButtonClicked);
	}
	if (CharacterButton0 != nullptr)
	{
		CharacterButton0->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleCharacterButton0Clicked);
	}
	if (CharacterButton1 != nullptr)
	{
		CharacterButton1->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleCharacterButton1Clicked);
	}
	if (CharacterButton2 != nullptr)
	{
		CharacterButton2->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleCharacterButton2Clicked);
	}
	if (BackToCharacterButton != nullptr)
	{
		BackToCharacterButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleBackToCharacterButtonClicked);
	}
	if (EnterRoomButton != nullptr)
	{
		EnterRoomButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleEnterRoomButtonClicked);
	}

	Super::NativeDestruct();
}

void UTitleMenuWidget::SwitchScreen(ETitleScreen Screen)
{
	mCurrentScreen = Screen;
	if (ScreenSwitcher != nullptr)
	{
		ScreenSwitcher->SetActiveWidgetIndex(GetScreenIndex(Screen));
	}
}

void UTitleMenuWidget::SyncText()
{
	SetText(TitleText, LocalizedText(TEXT("Rogue The Dice"), TEXT("로그 더 다이스")));
	SetText(StartButtonText, LocalizedText(TEXT("START"), TEXT("시작")));
	SetText(ContinueButtonText, LocalizedText(TEXT("CONTINUE"), TEXT("이어하기")));
	SetText(SettingsButtonText, LocalizedText(TEXT("SETTING"), TEXT("설정")));
	SetText(StatusText, FText::GetEmpty());
	SetWidgetVisible(StatusText, false);
	SetText(CharacterTitleText, LocalizedText(TEXT("SELECT HERO"), TEXT("영웅 선택")));
	SetText(CharacterStatusText, FText::GetEmpty());
	SetWidgetVisible(CharacterStatusText, false);
	SetText(BackToMainButtonText, LocalizedText(TEXT("BACK"), TEXT("뒤로")));
	SetText(ConfirmButtonText, LocalizedText(TEXT("CONFIRM"), TEXT("확인")));
	SetText(SettingsBackButtonText, LocalizedText(TEXT("BACK"), TEXT("뒤로")));
	SetText(SaveRunButtonText, LocalizedText(TEXT("SAVE RUN"), TEXT("런 저장")));
	SetText(AbandonRunButtonText, LocalizedText(TEXT("ABANDON RUN"), TEXT("런 포기")));
	SetText(BackToCharacterButtonText, LocalizedText(TEXT("BACK"), TEXT("뒤로")));
	SetText(EnterButtonText, FText::GetEmpty());
	SetWidgetVisible(EnterRoomButton, false);

	RefreshCharacterCards();
	RefreshSelectedCharacterDetails();
	RefreshSettingsText();
	RefreshMapText();
	RefreshLanguageText();
}

void UTitleMenuWidget::RefreshCharacterCards()
{
	UTextBlock* MutableNameBlocks[] = { CharacterNameText0.Get(), CharacterNameText1.Get(), CharacterNameText2.Get() };
	UTextBlock* RoleBlocks[] = { CharacterRoleText0.Get(), CharacterRoleText1.Get(), CharacterRoleText2.Get() };
	UTextBlock* StatBlocks[] = { CharacterStatText0.Get(), CharacterStatText1.Get(), CharacterStatText2.Get() };
	UTextBlock* DescriptionBlocks[] = { CharacterDescriptionText0.Get(), CharacterDescriptionText1.Get(), CharacterDescriptionText2.Get() };
	UTextBlock* StateBlocks[] = { CharacterStateText0.Get(), CharacterStateText1.Get(), CharacterStateText2.Get() };
	UTextBlock* FallbackBlocks[] = { CharacterIconFallbackText0.Get(), CharacterIconFallbackText1.Get(), CharacterIconFallbackText2.Get() };

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(DefaultCharacters); ++Index)
	{
		SetText(MutableNameBlocks[Index], FText::FromString(DefaultCharacters[Index].Name));
		SetText(RoleBlocks[Index], FText::GetEmpty());
		SetWidgetVisible(RoleBlocks[Index], false);
		SetText(StatBlocks[Index], FText::FromString(DefaultCharacters[Index].Stat));
		SetText(DescriptionBlocks[Index], FText::FromString(DefaultCharacters[Index].Description));
		SetText(StateBlocks[Index], Index == mSelectedCharacterIndex
			? LocalizedText(TEXT("SELECTED"), TEXT("선택됨"))
			: FText::GetEmpty());
		SetWidgetVisible(StateBlocks[Index], Index == mSelectedCharacterIndex);
		SetText(FallbackBlocks[Index], FText::FromString(DefaultCharacters[Index].Name));
	}
}

void UTitleMenuWidget::RefreshSelectedCharacterDetails()
{
	const int32 CharacterIndex = mSelectedCharacterIndex == INDEX_NONE ? 0 : mSelectedCharacterIndex;
	const FDefaultCharacterView& Character = DefaultCharacters[FMath::Clamp(CharacterIndex, 0, UE_ARRAY_COUNT(DefaultCharacters) - 1)];

	SetText(SelectedCharacterNameText, FText::FromString(Character.Name));
	SetText(SelectedCharacterRoleText, FText::GetEmpty());
	SetWidgetVisible(SelectedCharacterRoleText, false);
	SetText(SelectedCharacterStatText, FText::FromString(Character.Stat));
	SetText(SelectedCharacterDescriptionText, FText::FromString(Character.Description));
	SetText(SelectedCharacterPortraitFallbackText, FText::GetEmpty());
	SetWidgetVisible(SelectedCharacterPortraitFallbackText, false);
}

void UTitleMenuWidget::RefreshSettingsText()
{
	SetText(SettingsTitleText, LocalizedText(TEXT("RUN CONTROL"), TEXT("런 관리")));
	SetText(SettingsBodyText, bHasActiveRun
		? LocalizedText(TEXT("Run controls are available."), TEXT("런 관리 기능을 사용할 수 있습니다."))
		: LocalizedText(TEXT("Run controls appear after a run exists."), TEXT("런 생성 후 런 관리 기능이 표시됩니다.")));
	SetText(SettingsStateText, FText::GetEmpty());
	SetWidgetVisible(SettingsStateText, false);
}

void UTitleMenuWidget::RefreshMapText()
{
	SetText(MapTitleText, LocalizedText(TEXT("WORLD MAP"), TEXT("월드 맵")));
	SetText(MapStatusText, FText::GetEmpty());
	SetWidgetVisible(MapStatusText, false);
	SetText(MapPreviewTitleText, bHasActiveRun
		? LocalizedText(TEXT("Current stage"), TEXT("현재 스테이지"))
		: LocalizedText(TEXT("No active run"), TEXT("진행 중인 런 없음")));
	SetText(MapPreviewDescriptionText, bHasActiveRun
		? LocalizedText(TEXT("The generated stage path is shown below."), TEXT("생성된 스테이지 경로를 표시합니다."))
		: LocalizedText(TEXT("Select a hero and confirm to preview a generated map."), TEXT("영웅을 선택하고 확인하면 생성된 지도가 표시됩니다.")));
	SetText(MapPreviewStateText, FText::GetEmpty());
	SetWidgetVisible(MapPreviewStateText, false);
	SetWidgetVisible(EnterRoomButton, false);
}

void UTitleMenuWidget::RefreshLanguageText()
{
	SetText(LanguageTitleText, LocalizedText(TEXT("LANGUAGE"), TEXT("언어")));
	SetText(EnglishLanguageButtonText, FText::FromString(TEXT("ENGLISH")));
	SetText(KoreanLanguageButtonText, FText::FromString(TEXT("한국어")));
	SetText(LanguageStateText, bUseKoreanLanguage
		? FText::FromString(TEXT("한국어 미리보기"))
		: FText::FromString(TEXT("English preview")));
}

void UTitleMenuWidget::RefreshMapGraph()
{
	if (MapGraphCanvas == nullptr)
	{
		return;
	}

	MapGraphCanvas->ClearChildren();

	if (!bHasActiveRun || WidgetTree == nullptr)
	{
		return;
	}

	for (const FTitleMapRoomView& Room : mMapRoomViews)
	{
		UButton* RoomButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), NAME_None);
		RoomButton->SetIsEnabled(false);

		UBorder* RoomPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), NAME_None);
		RoomPanel->SetBrushColor(Room.bSelected ? FLinearColor(0.640f, 0.545f, 0.345f, 1.f) : GetMapRoomTypeColor(Room.mType));
		RoomPanel->SetPadding(FMargin(10.f, 8.f));
		RoomButton->SetContent(RoomPanel);

		UTextBlock* RoomText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), NAME_None);
		RoomText->SetJustification(ETextJustify::Center);
		RoomText->SetText(FText::Format(
			NSLOCTEXT("TitleMenuWidget", "MapNodeLabel", "{0}\n{1}-{2}"),
			Room.mTitle,
			FText::AsNumber(Room.mRow + 1),
			FText::AsNumber(Room.mColumn + 1)));
		FSlateFontInfo Font = RoomText->GetFont();
		Font.Size = 14;
		RoomText->SetFont(Font);
		RoomPanel->SetContent(RoomText);

		if (UCanvasPanelSlot* CanvasSlot = MapGraphCanvas->AddChildToCanvas(RoomButton))
		{
			const FVector2D Center = GetMapRoomNodeCenter(mMapRoomViews, Room);
			CanvasSlot->SetPosition(FVector2D(Center.X - MapNodeWidth * 0.5f, Center.Y - MapNodeHeight * 0.5f));
			CanvasSlot->SetSize(FVector2D(MapNodeWidth, MapNodeHeight));
		}
	}
}

void UTitleMenuWidget::RefreshMapRoomList()
{
	if (MapRoomList == nullptr)
	{
		return;
	}

	MapRoomList->ClearChildren();

	if (!bHasActiveRun || WidgetTree == nullptr)
	{
		return;
	}

	for (const FTitleMapRoomView& Room : mMapRoomViews)
	{
		UTextBlock* RoomText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), NAME_None);
		RoomText->SetText(FText::Format(
			NSLOCTEXT("TitleMenuWidget", "MapRoomListLabel", "{0}  {1}-{2}"),
			Room.mTitle,
			FText::AsNumber(Room.mRow + 1),
			FText::AsNumber(Room.mColumn + 1)));

		if (UVerticalBoxSlot* RoomSlot = MapRoomList->AddChildToVerticalBox(RoomText))
		{
			RoomSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}
	}
}

void UTitleMenuWidget::SetLanguage(bool bInUseKoreanLanguage)
{
	bUseKoreanLanguage = bInUseKoreanLanguage;
	SyncText();
}

void UTitleMenuWidget::SelectCharacter(int32 CharacterIndex)
{
	mSelectedCharacterIndex = CharacterIndex;
	RefreshCharacterCards();
	RefreshSelectedCharacterDetails();
	OnCharacterSelected.Broadcast(CharacterIndex);
}

void UTitleMenuWidget::SetWidgetVisible(UWidget* Widget, bool bVisible) const
{
	if (Widget != nullptr)
	{
		Widget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UTitleMenuWidget::SetText(UTextBlock* TextBlock, const FText& Text) const
{
	if (TextBlock != nullptr)
	{
		TextBlock->SetText(Text);
	}
}

FText UTitleMenuWidget::LocalizedText(const TCHAR* English, const TCHAR* Korean) const
{
	return FText::FromString(bUseKoreanLanguage ? Korean : English);
}

void UTitleMenuWidget::HandleStartButtonClicked()
{
	OnStartRequested.Broadcast();
	OpenCharacterSelectFromTitle();
}

void UTitleMenuWidget::HandleContinueButtonClicked()
{
	OnContinueRequested.Broadcast();
	OpenMapFromTopBar();
}

void UTitleMenuWidget::HandleSettingsButtonClicked()
{
	OpenSettingsFromTopBar();
}

void UTitleMenuWidget::HandleBackToMainButtonClicked()
{
	SwitchScreen(mCurrentScreen == ETitleScreen::Settings ? mPreviousScreenBeforeSettings : ETitleScreen::Main);
}

void UTitleMenuWidget::HandleConfirmButtonClicked()
{
	OnConfirmRequested.Broadcast();
}

void UTitleMenuWidget::HandleCharacterButton0Clicked()
{
	SelectCharacter(0);
}

void UTitleMenuWidget::HandleCharacterButton1Clicked()
{
	SelectCharacter(1);
}

void UTitleMenuWidget::HandleCharacterButton2Clicked()
{
	SelectCharacter(2);
}

void UTitleMenuWidget::HandleBackToCharacterButtonClicked()
{
	OpenCharacterSelectFromTitle();
}

void UTitleMenuWidget::HandleEnterRoomButtonClicked()
{
	OnEnterRoomRequested.Broadcast();
}

void UTitleMenuWidget::HandleSaveRunButtonClicked()
{
	OnSaveRunRequested.Broadcast();
}

void UTitleMenuWidget::HandleAbandonRunButtonClicked()
{
	OnAbandonRunRequested.Broadcast();
}

void UTitleMenuWidget::HandleEnglishLanguageButtonClicked()
{
	SetLanguage(false);
}

void UTitleMenuWidget::HandleKoreanLanguageButtonClicked()
{
	SetLanguage(true);
}
