#pragma once
#include "Components/ActorComponent.h"
#include "Component/TimeScaleComponent/TimeScaleComponent.h"
#include "CombatPlaybackComponent.generated.h"

class UCombatUIModel;

// Combat lifetime owns playback; the camera's mixer still owns all cinematic requests.
UCLASS()
class P_RD_API UCombatPlaybackComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCombatPlaybackComponent();
    void StartPlayback(UCombatUIModel* Model);
    void StopPlayback();
    void CycleSpeed();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void ApplySpeed();
    TWeakObjectPtr<UCombatUIModel> ViewModel;
    TWeakObjectPtr<UTimeScaleComponent> Mixer;
    FTimeScaleHandle Handle;
    int32 Speed = 1;
    bool Active = false;
};
