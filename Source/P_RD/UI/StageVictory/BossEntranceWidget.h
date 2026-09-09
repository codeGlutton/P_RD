#pragma once

#include "UI/CinematicWidget.h"
#include "BossEntranceWidget.generated.h"

/** Plays the approved entrance edit before the first combat turn. */
UCLASS()
class P_RD_API UBossEntranceWidget : public UCinematicWidget
{
    GENERATED_BODY()
public:
    static bool ShouldPlay(bool bBossRoom, bool bCleared, int32 Stage);
    static FString VideoPath(int32 Stage);
    static UBossEntranceWidget* Show(APlayerController* Player, int32 Stage, FSimpleDelegate Finished);
protected:
    virtual FReply NativeOnKeyDown(const FGeometry&, const FKeyEvent&) override { return FReply::Handled(); }
    virtual FReply NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
    virtual FReply NativeOnTouchStarted(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
    virtual bool HandleBackNavigation() override { return true; }
};
