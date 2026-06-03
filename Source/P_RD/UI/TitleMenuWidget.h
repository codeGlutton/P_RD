/*****************************************************************//**
 * @file   TitleMenuWidget.h
 * @brief  타이틀 메인 화면 위젯 정의 헤더
 * @author Codex
 * @date   2026-06-02
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"

#include "TitleMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UWidgetSwitcher;

/**
 * @brief 타이틀 메인 화면만 표시하는 HUD 위젯
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UTitleMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UTitleMenuWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	void NativeConstruct() override;
	void NativeDestruct() override;

private:
	void ShowMainScreen() const;
	void SyncMainText() const;
	void SetStatusText(const FText& InText) const;

	UFUNCTION()
	void HandleStartButtonClicked();

	UFUNCTION()
	void HandleContinueButtonClicked();

	UFUNCTION()
	void HandleSettingsButtonClicked();

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
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StartButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContinueButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;
};
