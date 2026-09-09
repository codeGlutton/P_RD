#include "UI/StageVictory/BossEntranceWidget.h"
#include "UI/UITextureLoader.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"

bool UBossEntranceWidget::ShouldPlay(bool bBossRoom, bool bCleared, int32 Stage)
{
    return bBossRoom && !bCleared && Stage >= 1 && Stage <= 3;
}

FString UBossEntranceWidget::VideoPath(int32 Stage)
{
    if (Stage < 1 || Stage > 3) return FString();
    return RDUITexture::ResolveContentFilePath(FString::Printf(
        TEXT("SVN/OutSideAsset/AICreation/UI/StageVictory/BossEntrance/BossEntrance%d.mp4"), Stage));
}

UBossEntranceWidget* UBossEntranceWidget::Show(APlayerController* Player, int32 Stage, FSimpleDelegate Finished)
{
    const FString Path = VideoPath(Stage);
    if (!Player || Path.IsEmpty() || !FPaths::FileExists(Path))
    {
        UE_LOG(LogTemp, Warning, TEXT("RD_BOSS_ENTRANCE missing stage=%d; continuing combat"), Stage);
        return nullptr;
    }
    auto* Widget = CreateWidget<UBossEntranceWidget>(Player);
    if (!Widget) return nullptr;
    Widget->SetCinematicVideoPath(Path);
    Widget->SetCinematicAudioEnabled(true);
    Widget->SetCinematicViewportZOrder(12000);
    Widget->SetHoldLastFrameOnFinish(true);
    Widget->SetIsFocusable(true);
    Widget->OpenUI();
    Widget->SetVisibility(ESlateVisibility::Visible);
    FInputModeUIOnly Input;
    Input.SetWidgetToFocus(Widget->TakeWidget());
    Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Player->SetInputMode(Input);
    Player->SetShowMouseCursor(false);
    Widget->PlayCinematic(FOnEndCinematicAnimation::CreateLambda(
        [Finished = MoveTemp(Finished), Stage](UCinematicWidget* Film)
        {
            UE_LOG(LogTemp, Display, TEXT("RD_BOSS_ENTRANCE finished stage=%d"), Stage);
            Film->CancelCinematic();
            Finished.ExecuteIfBound();
        }));
    UE_LOG(LogTemp, Display, TEXT("RD_BOSS_ENTRANCE start stage=%d file=%s"), Stage, *Path);
    return Widget;
}
