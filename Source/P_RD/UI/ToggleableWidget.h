#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"

#include "ToggleableWidget.generated.h"

DECLARE_DELEGATE_OneParam(FOnEndUIOpenAnimation, UUserWidget*)
DECLARE_DELEGATE_OneParam(FOnEndUICloseAnimation, UUserWidget*)

UCLASS(BlueprintType, Blueprintable)
class P_RD_API UToggleableWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UToggleableWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OpenUI(FOnEndUIOpenAnimation Callback = FOnEndUIOpenAnimation());
	virtual void CloseUI(FOnEndUICloseAnimation Callback = FOnEndUICloseAnimation());

	UFUNCTION(BlueprintPure, Category = "UI|Toggleable")
	virtual bool IsOpened() const;

	UFUNCTION(BlueprintCallable, Category = "UI|Toggleable")
	void FinishOpenUI();

	UFUNCTION(BlueprintCallable, Category = "UI|Toggleable")
	void FinishCloseUI();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "UI|Toggleable")
	void PlayOpenUIAnimation();
	virtual void PlayOpenUIAnimation_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "UI|Toggleable")
	void PlayCloseUIAnimation();
	virtual void PlayCloseUIAnimation_Implementation();

	virtual void ApplyOpenUI();
	virtual void ApplyCloseUI();
	virtual int32 GetViewportZOrder() const;
	virtual bool ShouldRemoveFromParentOnClose() const;

protected:
	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "ViewportZOrder"))
	int32 mViewportZOrder = 0;

	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "RemoveFromParentOnClose"))
	bool bRemoveFromParentOnClose = false;

private:
	enum class EToggleableWidgetLifecycleState : uint8
	{
		Closed,
		Opening,
		Open,
		Closing,
	};

	FOnEndUIOpenAnimation OnEndUIOpenAnimation;
	FOnEndUICloseAnimation OnEndUICloseAnimation;
	EToggleableWidgetLifecycleState LifecycleState = EToggleableWidgetLifecycleState::Closed;
};
