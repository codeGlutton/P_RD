#include "UI/StageVictory/BossEntranceWidget.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

#if !UE_BUILD_SHIPPING
namespace BossEntrancePreview
{
    TWeakObjectPtr<UBossEntranceWidget> Preview;
    void PlayPreview(UWorld* World, int32 Stage, bool All, bool Verify)
    {
        auto* Player=World?World->GetFirstPlayerController():nullptr;
        if(!Player)return;
        if(Preview.IsValid())Preview->CancelCinematic();
        TWeakObjectPtr<UWorld> WeakWorld=World;
        Preview=UBossEntranceWidget::Show(Player,Stage,FSimpleDelegate::CreateWeakLambda(Player,[WeakWorld,Stage,All,Verify]()
        {
            Preview.Reset();
            if(!WeakWorld.IsValid())return;
            if(All && Stage<3)
            {
                FTimerHandle Next;
                WeakWorld->GetTimerManager().SetTimer(Next,FTimerDelegate::CreateLambda([WeakWorld,Stage,Verify]()
                {
                    if(WeakWorld.IsValid())PlayPreview(WeakWorld.Get(),Stage+1,true,Verify);
                }),.3f,false);
                return;
            }
            if(auto* PC=WeakWorld->GetFirstPlayerController())
            {
                PC->SetInputMode(FInputModeGameAndUI());PC->SetShowMouseCursor(true);
            }
            UE_LOG(LogTemp,Display,TEXT("RD_BOSS_ENTRANCE preview complete all=%d"),All);
            if(Verify)FPlatformMisc::RequestExit(false);
        }));
        if(!Preview.IsValid())
        {
            UE_LOG(LogTemp,Error,TEXT("RD_BOSS_ENTRANCE preview failed stage=%d"),Stage);
            if(Verify)FPlatformMisc::RequestExit(false);
            return;
        }
        if(Verify)
        {
            const float Duration = Stage == 1 ? 3.f : Stage == 2 ? 10.3f : 14.f;
            for(int32 Beat=0;Beat<2;++Beat)
            {
                FTimerHandle Capture;
                World->GetTimerManager().SetTimer(Capture,FTimerDelegate::CreateLambda([Stage,Beat]()
                {
                    FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots"),FString::Printf(TEXT("BossEntrance_Stage%d_Beat%d.png"),Stage,Beat)),true,false);
                }),Beat==0?.85f:Duration-.35f,false);
            }
        }
    }
    FAutoConsoleCommandWithWorldAndArgs Command(TEXT("RD.BossEntrance.Preview"),
        TEXT("Approved boss entrance movies only; no rewards or saves. Stage 1-3, or 0 for all. Optional verify captures and exits."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args,UWorld* World)
        {
            if(!World || !World->IsGameWorld())return;
            const int32 Stage=Args.IsEmpty()?1:FMath::Clamp(FCString::Atoi(*Args[0]),0,3);
            const bool Verify=Args.Num()>1 && Args[1]==TEXT("verify");
            TWeakObjectPtr<UWorld> WeakWorld=World;
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakWorld,Stage,Verify](float)
            {
                if(WeakWorld.IsValid())PlayPreview(WeakWorld.Get(),Stage==0?1:Stage,Stage==0,Verify);
                return false;
            }),3.f);
        }));
}
#endif
