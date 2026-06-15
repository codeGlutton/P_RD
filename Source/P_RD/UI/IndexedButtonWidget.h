#pragma once

#include "RDMinimal.h"
#include "Components/Button.h"

#include "IndexedButtonWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIndexedButtonWidgetEvent, int32, ButtonIndex);

/**
 * @brief 런타임으로 생성하는 버튼이 자신의 index를 함께 전달하도록 하는 작은 UButton 래퍼.
 */
UCLASS()
class P_RD_API UIndexedButtonWidget : public UButton
{
	GENERATED_BODY()

public:
	UIndexedButtonWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "UI|Input")
	void SetButtonIndex(int32 InButtonIndex);

	UFUNCTION(BlueprintPure, Category = "UI|Input")
	int32 GetButtonIndex() const;

	UPROPERTY(BlueprintAssignable, Category = "UI|Input")
	FIndexedButtonWidgetEvent OnIndexedClicked;

	UPROPERTY(BlueprintAssignable, Category = "UI|Input")
	FIndexedButtonWidgetEvent OnIndexedPressed;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandlePressed();

	UPROPERTY(Category = "UI|Input", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	int32 mButtonIndex = INDEX_NONE;
};
