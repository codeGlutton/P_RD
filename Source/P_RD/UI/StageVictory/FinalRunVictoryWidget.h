#pragma once
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "FinalRunVictoryWidget.generated.h"

/** Final campaign acknowledgement, shown only after all boss rewards are settled. */
UCLASS()
class P_RD_API UFinalRunVictoryWidget : public UUserWidget
{
    GENERATED_BODY()
  public:
    void SetOnContinue(FSimpleDelegate Callback)
    {
        Continue = MoveTemp(Callback);
    }
    UFUNCTION() void Confirm();

  protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

  private:
    FSimpleDelegate Continue;
    UPROPERTY(Transient) TObjectPtr<class UButton> ContinueButton;
};
