#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "BossCollapseWidget.generated.h"

class UAudioComponent;
class USoundBase;
class UTexture2D;

/** Boss-only victory comic. Completion opens rewards; it never grants rewards itself. */
UCLASS()
class P_RD_API UBossCollapseWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static UBossCollapseWidget* Show(APlayerController* Player, int32 Stage, FSimpleDelegate OnFinished);
    static bool ShouldPlay(bool bWon, bool bBossRoom, int32 Stage);
    void Cancel();
    float GetElapsedTime() const { return Elapsed; }
    float GetDuration() const;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry&, float Delta) override;
    virtual void NativeDestruct() override;
    virtual int32 NativePaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
        FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
    virtual FReply NativeOnKeyDown(const FGeometry&, const FKeyEvent&) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
    virtual FReply NativeOnTouchStarted(const FGeometry&, const FPointerEvent&) override;
private:
    friend class FBossCollapseFlowTest;
    friend class FBossCollapseRenderTest;
    bool LoadStage(int32 InStage);
    void Advance(float Delta);
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Atlas;
    UPROPERTY(Transient) TObjectPtr<USoundBase> Sound;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Audio;
    UPROPERTY(Transient) TArray<FSlateBrush> Panels;
    FSimpleDelegate Finished;
    int32 StageNumber = 0;
    float Elapsed = 0.f;
    bool bFinished = false;
};
