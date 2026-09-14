#include "Component/TimeScaleComponent/CombatPlaybackComponent.h"
#include "UI/Combat/CombatUIModel.h"
#include "FunctionLibrary/CameraFunctionLibrary.h"
#include "Pawn/Camera/CombatCameraPawn.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Engine/GameInstance.h"

UCombatPlaybackComponent::UCombatPlaybackComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCombatPlaybackComponent::StartPlayback(UCombatUIModel* Model)
{
    StopPlayback();
    ViewModel = Model;
    if (auto* GI = GetWorld()->GetGameInstance())
        if (auto* Data = GI->GetSubsystem<UPersistentDataSubsystem>())
            Speed = Data->GetOptionPersistData()->GetCombatPlaybackSpeed();
    Active = true;
    if (Model) Model->OnCyclePlaybackSpeed.AddUObject(this, &UCombatPlaybackComponent::CycleSpeed);
    SetComponentTickEnabled(true);
    ApplySpeed();
}

void UCombatPlaybackComponent::ApplySpeed()
{
    auto* Camera = UCameraFunctionLibrary::GetMainCameraPawn(this);
    auto* CurrentMixer = Camera ? Camera->GetTimeScaleComponent() : nullptr;
    if (Mixer.Get() != CurrentMixer)
    {
        if (Mixer.IsValid()) Mixer->ReleaseTimeScaleImmediately(Handle);
        Mixer = CurrentMixer;
        Handle.Invalidate();
    }
    if (Mixer.IsValid())
    {
        if (!Handle.IsValid()) Handle = Mixer->RequestTimeScale(this, Speed);
        Mixer->SetTimeScaleImmediately(Handle, Speed);
    }
    if (ViewModel.IsValid()) ViewModel->SetPlaybackSpeed(Speed, Active && Mixer.IsValid());
}

void UCombatPlaybackComponent::CycleSpeed()
{
    if (!Active || !Mixer.IsValid()) return;
    Speed = Speed % 3 + 1;
    ApplySpeed();
    if (auto* GI = GetWorld()->GetGameInstance())
    {
        GI->GetSubsystem<UPersistentDataSubsystem>()->GetOptionPersistData()->SetCombatPlaybackSpeed(Speed);
        // Tiny options save is synchronous: rapid taps cannot race older async writes.
        const bool Saved = GI->GetSubsystem<USaveGameSubsystem>()->SaveOption();
        UE_LOG(LogTemp, Display, TEXT("RD_PLAYBACK speed=%d saved=%d"), Speed, Saved);
    }
}

void UCombatPlaybackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    if (Active) ApplySpeed(); // Reattach after a camera replacement; never accumulate requests.
}

void UCombatPlaybackComponent::StopPlayback()
{
    Active = false;
    SetComponentTickEnabled(false);
    if (Mixer.IsValid()) Mixer->ReleaseTimeScaleImmediately(Handle);
    Mixer.Reset(); Handle.Invalidate();
    if (ViewModel.IsValid())
    {
        ViewModel->OnCyclePlaybackSpeed.RemoveAll(this);
        ViewModel->SetPlaybackSpeed(Speed, false);
    }
    ViewModel.Reset();
}

void UCombatPlaybackComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    StopPlayback();
    Super::EndPlay(Reason);
}
