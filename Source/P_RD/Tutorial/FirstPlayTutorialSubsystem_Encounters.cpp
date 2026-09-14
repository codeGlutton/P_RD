#include "Tutorial/FirstPlayTutorialSubsystem.h"
#include "Actor/BoardActor/Obstacle/Gimmick/GimmickModel.h"
#include "DataAsset/ObstacleSpawnData/StaticGimmickSpawnData.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Tutorial/GuidedTutorialWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

namespace
{
bool ProjectTrap(UFirstPlayTutorialSubsystem* Context, const UGimmickModel* Trap,
                 UCombatLayoutHUDWidget* HUD, TArray<FVector2D>& Corners)
{
    Corners.Reset();
    const AActor* View = Trap ? Trap->GetView<AActor>() : nullptr;
    auto* PC = Context->GetWorld()->GetFirstPlayerController();
    if (!View || View->IsHidden() || !PC || Trap->IsDead()) return false;
    FVector Center, Extent;
    View->GetActorBounds(false, Center, Extent);
    if (Extent.IsNearlyZero()) return false;
    FVector2D Min(DBL_MAX, DBL_MAX), Max(-DBL_MAX, -DBL_MAX);
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    for (int32 I = 0; I < 8; ++I)
    {
        FVector2D Pixel;
        const FVector Point = Center + Extent * FVector(I & 1 ? 1 : -1, I & 2 ? 1 : -1, I & 4 ? 1 : -1);
        if (!PC->ProjectWorldLocationToScreen(Point, Pixel)) return false;
        Min.X = FMath::Min(Min.X, Pixel.X); Min.Y = FMath::Min(Min.Y, Pixel.Y);
        Max.X = FMath::Max(Max.X, Pixel.X); Max.Y = FMath::Max(Max.Y, Pixel.Y);
    }
    // Never block controls for a target outside the visible playfield.
    if (Min.X < 0 || Min.Y < 0 || Max.X > Width || Max.Y > Height) return false;
    FVector2D AbsoluteCenter;
    USlateBlueprintLibrary::ScreenToWidgetAbsolute(Context, (Min + Max) * .5, AbsoluteCenter);
    if (!HUD->IsGuidedBoardInputAt(AbsoluteCenter)) return false;
    for (const auto Pixel : {Min, FVector2D(Max.X, Min.Y), Max, FVector2D(Min.X, Max.Y)})
    {
        FVector2D Absolute;
        USlateBlueprintLibrary::ScreenToWidgetAbsolute(Context, Pixel, Absolute);
        Corners.Add(Absolute);
    }
    return true;
}
}

bool UFirstPlayTutorialSubsystem::IsEncounterHintVisible() const
{
    return EncounterWidget && EncounterWidget->IsInViewport() && EncounterWidget->IsVisible();
}

void UFirstPlayTutorialSubsystem::HideEncounterHint()
{
    if (EncounterWidget) EncounterWidget->RemoveFromParent();
    EncounterWidget = nullptr;
    EncounterTarget.Reset();
    EncounterReadyAt = FPlatformTime::Seconds() + .6;
}

void UFirstPlayTutorialSubsystem::EncounterConfirmed()
{
    if (!IsEncounterHintVisible() || !EncounterTarget.IsValid()) return;
    const FPrimaryAssetId Type = EncounterTarget->GetStaticSpawnDataId();
    if (Type.IsValid())
    {
        GetUserMutableData()->SeenTrapHints.Add(Type);
        Save();
        UE_LOG(LogTemp, Display, TEXT("Trap hint acknowledged: %s"), *Type.ToString());
    }
    HideEncounterHint();
}

bool UFirstPlayTutorialSubsystem::UpdateEncounterHint(UCombatLayoutHUDWidget* HUD)
{
    const auto* Data = GetUserMutableData();
    const auto& Flow = Data->GuidedTutorial;
    const bool BasicGuideActive = Flow.Enrolled && !Data->TutorialProgress.Skipped &&
        (Flow.CanGuideCombat() || Flow.SystemsActive || Flow.Lesson != EGuidedLesson::None);
    if (!HUD || !bInCombat || !bPlayerTurn || bExecutingAction || BasicGuideActive ||
        !HUD->CanShowEncounterHint())
    {
        HideEncounterHint();
        return false;
    }
    TArray<FVector2D> Corners;
    if (EncounterWidget)
    {
        if (!ProjectTrap(this, EncounterTarget.Get(), HUD, Corners))
        {
            HideEncounterHint();
            return false;
        }
        EncounterWidget->SetWorldFocus(Corners);
        return true;
    }
    if (FPlatformTime::Seconds() < EncounterReadyAt) return false;
    const auto* Combat = GetWorldSubsystemModel<USRPGCombatModel>(GetWorld());
    if (!Combat) return false;
    for (const auto& Obstacle : Combat->GetObstacles())
    {
        auto* Trap = Cast<UGimmickModel>(Obstacle);
        if (!Trap || Data->SeenTrapHints.Contains(Trap->GetStaticSpawnDataId())) continue;
        const auto* Spawn = Cast<UStaticGimmickSpawnData>(Trap->GetStaticSpawnData());
        // An unconfigured future gimmick must not show an empty, blocking notice.
        if (!Spawn || Spawn->mDisplayName.IsEmpty() || Spawn->mDescription.IsEmpty() ||
            !ProjectTrap(this, Trap, HUD, Corners)) continue;
        EncounterWidget = CreateWidget<UGuidedTutorialWidget>(GetGameInstance());
        if (!EncounterWidget) return false;
        EncounterTarget = Trap;
        EncounterWidget->PresentEncounter(Spawn->mDisplayName, Spawn->mDescription);
        EncounterWidget->SetWorldFocus(Corners);
        EncounterWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        EncounterWidget->AddToViewport(75);
        EncounterWidget->GetContinueButton()->OnClicked.AddUniqueDynamic(this, &UFirstPlayTutorialSubsystem::EncounterConfirmed);
        UE_LOG(LogTemp, Display, TEXT("Trap hint shown: %s"), *Trap->GetStaticSpawnDataId().ToString());
        return true;
    }
    return false;
}
