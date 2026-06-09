#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"

#include "ToggleableWidgetInterface.generated.h"

class UUserWidget;

DECLARE_DELEGATE_OneParam(FOnEndUIOpenAnimation, UUserWidget*)
DECLARE_DELEGATE_OneParam(FOnEndUICloseAnimation, UUserWidget*)

UINTERFACE()
class P_RD_API UToggleableWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

class P_RD_API IToggleableWidgetInterface
{
	GENERATED_BODY()

public:
	virtual void OpenUI(FOnEndUIOpenAnimation Callback = FOnEndUIOpenAnimation()) = 0;
	virtual void CloseUI(FOnEndUICloseAnimation Callback = FOnEndUICloseAnimation()) = 0;
	virtual bool IsOpened() const = 0;
};
