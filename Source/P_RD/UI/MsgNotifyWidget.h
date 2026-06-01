/*****************************************************************//**
 * @file   MsgNotifyWidget.h
 * @brief  공용 메시지 알림 위젯 정의 헤더
 * @author Codex
 * @date   2026-06-01
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/WorldWidgetLifecycle.h"

#include "MsgNotifyWidget.generated.h"

class UTextBlock;

/**
 * @brief  소유자 없는 공용 메시지를 표시하는 월드 위젯
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UMsgNotifyWidget : public UUserWidget, public IWorldWidgetLifecycle
{
	GENERATED_BODY()

public:
	UMsgNotifyWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "DefaultMessage"))
	FText mDefaultMessage;

	UPROPERTY(Category = UI, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CurrentMessage"))
	FText mCurrentMessage;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageTextBlock;
};
