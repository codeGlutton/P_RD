#include "UI/TitleMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
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
		const TCHAR* Role;
		const TCHAR* Stat;
		const TCHAR* Description;
	};

	const FDefaultCharacterView DefaultCharacters[] = {
		{ TEXT("Warrior"), TEXT("Frontline"), TEXT("HP 30 | Dice 3 | Gold 50"), TEXT("Balanced starter hero.") },
		{ TEXT("Archer"), TEXT("Ranged"), TEXT("HP 22 | Dice 4 | Gold 40"), TEXT("Preview slot for ranged play.") },
		{ TEXT("Mage"), TEXT("Caster"), TEXT("HP 18 | Dice 5 | Gold 35"), TEXT("Preview slot for spell play.") },
	};

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
	, mReadyStatusText(NSLOCTEXT("TitleMenuWidget", "ReadyStatusText", "Ready"))
	, mSettingsHintText(NSLOCTEXT("TitleMenuWidget", "SettingsHintText", "Language and run controls are UI-only in this pass."))
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
	SetText(TitleText, mTitleText);
	SetText(StartButtonText, mStartText);
	SetText(ContinueButtonText, mContinueText);
	SetText(SettingsButtonText, mSettingsText);
	SetText(StatusText, mReadyStatusText);
	SetText(CharacterTitleText, mCharacterSelectText);
	SetText(CharacterStatusText, mReadyStatusText);
	SetText(BackToMainButtonText, mBackText);
	SetText(ConfirmButtonText, mConfirmText);
	SetText(SettingsBackButtonText, mBackText);
	SetText(SaveRunButtonText, mSaveRunText);
	SetText(AbandonRunButtonText, mAbandonRunText);
	SetText(BackToCharacterButtonText, mBackText);
	SetText(EnterButtonText, mEnterText);

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
		SetText(RoleBlocks[Index], FText::FromString(DefaultCharacters[Index].Role));
		SetText(StatBlocks[Index], FText::FromString(DefaultCharacters[Index].Stat));
		SetText(DescriptionBlocks[Index], FText::FromString(DefaultCharacters[Index].Description));
		SetText(StateBlocks[Index], Index == mSelectedCharacterIndex
			? NSLOCTEXT("TitleMenuWidget", "CharacterSelected", "SELECTED")
			: NSLOCTEXT("TitleMenuWidget", "CharacterReady", "READY"));
		SetText(FallbackBlocks[Index], FText::FromString(DefaultCharacters[Index].Name));
	}
}

void UTitleMenuWidget::RefreshSelectedCharacterDetails()
{
	const int32 CharacterIndex = mSelectedCharacterIndex == INDEX_NONE ? 0 : mSelectedCharacterIndex;
	const FDefaultCharacterView& Character = DefaultCharacters[FMath::Clamp(CharacterIndex, 0, UE_ARRAY_COUNT(DefaultCharacters) - 1)];

	SetText(SelectedCharacterNameText, FText::FromString(Character.Name));
	SetText(SelectedCharacterRoleText, FText::FromString(Character.Role));
	SetText(SelectedCharacterStatText, FText::FromString(Character.Stat));
	SetText(SelectedCharacterDescriptionText, FText::FromString(Character.Description));
	SetText(SelectedCharacterPortraitFallbackText, FText::FromString(Character.Name));
}

void UTitleMenuWidget::RefreshSettingsText()
{
	SetText(SettingsTitleText, mRunControlText);
	SetText(SettingsBodyText, mSettingsHintText);
	SetText(SettingsStateText, mReadyStatusText);
}

void UTitleMenuWidget::RefreshMapText()
{
	SetText(MapTitleText, mMapText);
	SetText(MapStatusText, mMapReadyStatusText);
	SetText(MapPreviewTitleText, NSLOCTEXT("TitleMenuWidget", "MapPreviewTitle", "No room selected"));
	SetText(MapPreviewDescriptionText, NSLOCTEXT("TitleMenuWidget", "MapPreviewDescription", "Game flow will provide room data in the next pass."));
	SetText(MapPreviewStateText, mReadyStatusText);
}

void UTitleMenuWidget::RefreshLanguageText()
{
	SetText(LanguageTitleText, mLanguageText);
	SetText(EnglishLanguageButtonText, mEnglishText);
	SetText(KoreanLanguageButtonText, mKoreanText);
	SetText(LanguageStateText, bUseKoreanLanguage
		? NSLOCTEXT("TitleMenuWidget", "KoreanLanguageState", "Korean preview")
		: NSLOCTEXT("TitleMenuWidget", "EnglishLanguageState", "English preview"));
}

void UTitleMenuWidget::SelectCharacter(int32 CharacterIndex)
{
	mSelectedCharacterIndex = CharacterIndex;
	RefreshCharacterCards();
	RefreshSelectedCharacterDetails();
	SetText(CharacterStatusText, NSLOCTEXT("TitleMenuWidget", "CharacterSelectedStatus", "Hero selected"));
	OnCharacterSelected.Broadcast(CharacterIndex);
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
	OpenMapFromTopBar();
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
	SetText(MapStatusText, NSLOCTEXT("TitleMenuWidget", "EnterRoomRequested", "Enter room requested"));
}

void UTitleMenuWidget::HandleSaveRunButtonClicked()
{
	OnSaveRunRequested.Broadcast();
	SetText(SettingsStateText, NSLOCTEXT("TitleMenuWidget", "SaveRunRequested", "Save requested"));
}

void UTitleMenuWidget::HandleAbandonRunButtonClicked()
{
	OnAbandonRunRequested.Broadcast();
	SetText(SettingsStateText, NSLOCTEXT("TitleMenuWidget", "AbandonRunRequested", "Abandon requested"));
}

void UTitleMenuWidget::HandleEnglishLanguageButtonClicked()
{
	bUseKoreanLanguage = false;
	RefreshLanguageText();
	SetText(SettingsStateText, mLanguageAppliedText);
}

void UTitleMenuWidget::HandleKoreanLanguageButtonClicked()
{
	bUseKoreanLanguage = true;
	RefreshLanguageText();
	SetText(SettingsStateText, mLanguageAppliedText);
}
