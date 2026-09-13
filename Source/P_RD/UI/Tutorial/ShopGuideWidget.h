#pragma once
#include "UI/RDUserWidget.h"
#include "ShopGuideWidget.generated.h"
DECLARE_DELEGATE_RetVal_OneParam(UWidget*, FShopGuideTarget, int32);
/** Explains actual shop controls; advancing never executes a transaction. */
UCLASS()
class P_RD_API UShopGuideWidget : public URDUserWidget
{
    GENERATED_BODY()
public:
    UShopGuideWidget(const FObjectInitializer& Initializer);
    virtual bool Initialize() override;
    virtual bool UsesMobileSafeArea() const override { return true; }
    virtual bool HandleBackNavigation() override;
    FSimpleDelegate OnDismissed;
    FShopGuideTarget OnStepChanged;
    static constexpr int32 PageCount = 12;
    int32 GetPage() const { return Page; }
    class UGuidedTutorialWidget* GetGuide() const { return Guide; }
    void BeginReading();
    UFUNCTION() void Next();
    UFUNCTION() void Previous();
    UFUNCTION() void Dismiss();
protected:
    virtual void ApplyOpenUI() override;
private:
    void RefreshPage();
    int32 Page = 0;
    bool bDismissed = false;
    UPROPERTY() TObjectPtr<class UGuidedTutorialWidget> Guide;
};
