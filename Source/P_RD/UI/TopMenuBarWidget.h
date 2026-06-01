/*****************************************************************//**
 * @file   TopMenuBarWidget.h
 * @brief  상단 메뉴바 위젯 정의 헤더
 * @author Codex
 * @date   2026-06-01
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/WorldWidgetLifecycle.h"

#include "TopMenuBarWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * @brief  대부분의 룸에서 공용으로 사용하는 기본 상단 메뉴바
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UTopMenuBarWidget : public UUserWidget, public IWorldWidgetLifecycle
{
	GENERATED_BODY()

public:
	UTopMenuBarWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(Category = UI, BlueprintCallable)
	void SetTitle(const FText& InText);

	UFUNCTION(Category = UI, BlueprintCallable)
	void SetSummary(const FText& InText);

	UFUNCTION(Category = UI, BlueprintCallable)
	void SetDiceLabel(const FText& InText);

	UFUNCTION(Category = UI, BlueprintCallable)
	void SetSkillLabel(const FText& InText);

	/* IWorldWidgetLifecycle 상속 */
public:
	void OpenUI_Implementation() override;
	void CloseUI_Implementation() override;

	/* UUserWidget 상속 */
protected:
	void NativeConstruct() override;
	void NativeDestruct() override;

private:
	void SyncText();
	void OpenMap() const;
	void OpenSettings() const;

	UFUNCTION()
	void HandleMapButtonClicked();

	UFUNCTION()
	void HandleSettingsButtonClicked();

protected:
	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TitleText"))
	FText mTitleText;

	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "MenuText"))
	FText mMenuText;

	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "MapText"))
	FText mMapText;

	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "DiceText"))
	FText mDiceText;

	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SkillText"))
	FText mSkillText;

	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SettingsText"))
	FText mSettingsText;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DiceTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkillTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MapButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton;
};
