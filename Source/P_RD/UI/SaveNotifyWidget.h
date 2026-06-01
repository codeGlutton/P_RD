/*****************************************************************//**
 * @file   SaveNotifyWidget.h
 * @brief  저장 상태 알림 위젯 정의 헤더
 * @author Codex
 * @date   2026-06-01
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/WorldWidgetLifecycle.h"

#include "SaveNotifyWidget.generated.h"

class UTextBlock;

/**
 * @brief  저장 시작/완료 상태를 표시하는 기본 월드 공용 위젯
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API USaveNotifyWidget : public UUserWidget, public IWorldWidgetLifecycle
{
	GENERATED_BODY()

public:
	USaveNotifyWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(Category = UI, BlueprintCallable)
	void SetMessage(const FText& Message);

	/* IWorldWidgetLifecycle 상속 */
public:
	void OpenUI_Implementation() override;
	void CloseUI_Implementation() override;

	/* UUserWidget 상속 */
protected:
	void NativeConstruct() override;

protected:
	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SavingText"))
	FText mSavingText;

	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SavedText"))
	FText mSavedText;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageTextBlock;
};
