#pragma once
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Tutorial/FirstPlayProgress.h"
#include "FirstPlayTutorialWidget.generated.h"

UCLASS()
class P_RD_API UFirstPlayTutorialWidget : public UUserWidget
{
	GENERATED_BODY()
  public:
	UFirstPlayTutorialWidget(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());
	FSimpleDelegate OnPrimary;
	FSimpleDelegate OnSkip;
	void Present(EFirstPlayStep Step, bool TitleContext, bool ReplayOnly = false);
	static FText Heading(EFirstPlayStep Step);
	static FText Instructions(EFirstPlayStep Step);

  protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void ReleaseSlateResources(bool Children) override;

  private:
	UPROPERTY() TObjectPtr<class UFont> GuideFontAsset;
	EFirstPlayStep CurrentStep = EFirstPlayStep::Welcome;
	bool bTitle = false;
	bool bReplay = false;
	bool bCompact = false;
	TSharedPtr<class SBox> Host;
	void RefreshCard();
};
