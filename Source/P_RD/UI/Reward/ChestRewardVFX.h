#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChestRewardVFX.generated.h"

// An isolated live particle stage, captured into the reward UI.
UCLASS(Transient, NotBlueprintable)
class AChestRewardVFX : public AActor
{
	GENERATED_BODY()
public:
	AChestRewardVFX();
	static void PreloadAssets();
	class UMaterialInstanceDynamic* StartEffect();
	UPROPERTY() TObjectPtr<class UEffekseerSystemComponent> Particles;
	UPROPERTY() TObjectPtr<class UEffekseerEmitterComponent> Emitter;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class USceneCaptureComponent2D> Capture;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UTextureRenderTarget2D> Target;
	UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> Material;
};
