/*****************************************************************//**
 * @file   TitleMenuWidget.h
 * @brief  Title menu widget shell.
 * @author Codex
 * @date   2026-06-01
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"

#include "TitleMenuWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UVerticalBox;
class UWidget;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTitleMenuEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTitleMenuCharacterEvent, int32, CharacterIndex);

/**
 * @brief Title HUD shell. It owns visual state and exposes events for game flow wiring.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UTitleMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UTitleMenuWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(Category = UI, BlueprintCallable)
	void OpenCharacterSelectFromTitle();

	UFUNCTION(Category = UI, BlueprintCallable)
	void OpenMapFromTopBar();

	UFUNCTION(Category = UI, BlueprintCallable)
	void OpenSettingsFromTopBar();

public:
	UPROPERTY(Category = UI, BlueprintAssignable)
	FTitleMenuEvent OnStartRequested;

	UPROPERTY(Category = UI, BlueprintAssignable)
	FTitleMenuEvent OnContinueRequested;

	UPROPERTY(Category = UI, BlueprintAssignable)
	FTitleMenuEvent OnConfirmRequested;

	UPROPERTY(Category = UI, BlueprintAssignable)
	FTitleMenuEvent OnEnterRoomRequested;

	UPROPERTY(Category = UI, BlueprintAssignable)
	FTitleMenuEvent OnSaveRunRequested;

	UPROPERTY(Category = UI, BlueprintAssignable)
	FTitleMenuEvent OnAbandonRunRequested;

	UPROPERTY(Category = UI, BlueprintAssignable)
	FTitleMenuCharacterEvent OnCharacterSelected;

protected:
	void NativeConstruct() override;
	void NativeDestruct() override;

public:
	enum class ETitleScreen : uint8
	{
		Main,
		CharacterSelect,
		Settings,
		Map
	};

private:
	void SwitchScreen(ETitleScreen Screen);
	void SyncText();
	void RefreshCharacterCards();
	void RefreshSelectedCharacterDetails();
	void RefreshSettingsText();
	void RefreshMapText();
	void RefreshLanguageText();
	void SelectCharacter(int32 CharacterIndex);
	void SetText(UTextBlock* TextBlock, const FText& Text) const;
	FText LocalizedText(const TCHAR* English, const TCHAR* Korean) const;

	UFUNCTION()
	void HandleStartButtonClicked();
	UFUNCTION()
	void HandleContinueButtonClicked();
	UFUNCTION()
	void HandleSettingsButtonClicked();
	UFUNCTION()
	void HandleBackToMainButtonClicked();
	UFUNCTION()
	void HandleConfirmButtonClicked();
	UFUNCTION()
	void HandleCharacterButton0Clicked();
	UFUNCTION()
	void HandleCharacterButton1Clicked();
	UFUNCTION()
	void HandleCharacterButton2Clicked();
	UFUNCTION()
	void HandleBackToCharacterButtonClicked();
	UFUNCTION()
	void HandleEnterRoomButtonClicked();
	UFUNCTION()
	void HandleSaveRunButtonClicked();
	UFUNCTION()
	void HandleAbandonRunButtonClicked();
	UFUNCTION()
	void HandleEnglishLanguageButtonClicked();
	UFUNCTION()
	void HandleKoreanLanguageButtonClicked();

private:
	FText mTitleText;
	FText mStartText;
	FText mContinueText;
	FText mSettingsText;
	FText mCloseText;
	FText mSaveRunText;
	FText mAbandonRunText;
	FText mRunControlText;
	FText mCharacterSelectText;
	FText mConfirmText;
	FText mBackText;
	FText mMapText;
	FText mEnterText;
	FText mReadyStatusText;
	FText mSettingsHintText;
	FText mLanguageText;
	FText mEnglishText;
	FText mKoreanText;
	FText mLanguageAppliedText;
	FText mMapReadyStatusText;

	int32 mSelectedCharacterIndex = INDEX_NONE;
	ETitleScreen mCurrentScreen = ETitleScreen::Main;
	ETitleScreen mPreviousScreenBeforeSettings = ETitleScreen::Main;
	bool bUseKoreanLanguage = false;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> ScreenSwitcher;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackToMainButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsBackButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveRunButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> AbandonRunButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EnglishLanguageButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> KoreanLanguageButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CharacterButton0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CharacterButton1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CharacterButton2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackToCharacterButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EnterRoomButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StartButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContinueButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterTitleText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterStatusText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BackToMainButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ConfirmButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsTitleText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsBodyText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsStateText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LanguageStateText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LanguageTitleText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsBackButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SaveRunButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AbandonRunButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EnglishLanguageButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KoreanLanguageButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedCharacterNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedCharacterRoleText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedCharacterStatText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedCharacterDescriptionText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedCharacterPortraitFallbackText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterIconFallbackText0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterIconFallbackText1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterIconFallbackText2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterNameText0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterNameText1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterNameText2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterRoleText0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterRoleText1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterRoleText2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterStatText0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterStatText1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterStatText2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterDescriptionText0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterDescriptionText1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterDescriptionText2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterStateText0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterStateText1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterStateText2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BackToCharacterButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapStatusText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapTitleText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EnterButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapPreviewTitleText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapPreviewDescriptionText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapPreviewStateText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SelectedCharacterPortraitImage;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CharacterIconImage0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CharacterIconImage1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CharacterIconImage2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CharacterPanel0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CharacterPanel1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CharacterPanel2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MapPreviewSize;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> MapPreviewPanel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> MapRoomList;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> MapGraphCanvas;
};
