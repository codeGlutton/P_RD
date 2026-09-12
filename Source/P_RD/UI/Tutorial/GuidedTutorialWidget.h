#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tutorial/GuidedTutorial.h"
#include "GuidedTutorialWidget.generated.h"
UCLASS()
class P_RD_API UGuidedTutorialWidget : public UUserWidget
{
	GENERATED_BODY()
  public:
	UGuidedTutorialWidget(const FObjectInitializer &Initializer = FObjectInitializer::Get());
	void Present(EGuidedStage Stage, UWidget *Target, bool BoardTarget = false, bool Retry = false);
	class UButton *GetContinueButton() const
	{
		return ContinueButton;
	}
	void PresentEncounter(const FText& Title, const FText& Description);
	static FText Instruction(EGuidedStage Stage);
	void SetNotice(const FText &Text);
	void SetWorldFocus(const TArray<FVector2D> &Corners)
	{
		WorldFocus = Corners;
	}
#if WITH_DEV_AUTOMATION_TESTS
	void UpdateLayoutForTest()
	{
		NativeTick(GetCachedGeometry(), .016f);
	}
#endif
  protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeTick(const FGeometry &Geometry, float DeltaTime) override;
	int32 NativePaint(const FPaintArgs &, const FGeometry &, const FSlateRect &, FSlateWindowElementList &, int32,
					  const FWidgetStyle &, bool) const override;

  private:
	UPROPERTY() TObjectPtr<class UFont> Font;
	UPROPERTY() TObjectPtr<class UTexture2D> Parchment;
	UPROPERTY() TObjectPtr<class UCanvasPanel> Canvas;
	UPROPERTY() TObjectPtr<class USizeBox> Card;
	UPROPERTY() TObjectPtr<class UTextBlock> Counter;
	UPROPERTY() TObjectPtr<class UTextBlock> Label;
	UPROPERTY() TObjectPtr<class UTextBlock> GestureText;
	UPROPERTY() TObjectPtr<class UButton> ContinueButton;
	TWeakObjectPtr<UWidget> TargetWidget;
	EGuidedStage CurrentStage = EGuidedStage::OpenSkills;
	bool bBoard = false, bFocus = false;
	float GestureTime = 0;
	FVector2D FocusA, FocusB, BubblePosition, BubbleSize;
	TArray<FVector2D> WorldFocus;
	FText EncounterTitle, EncounterDescription;
};
