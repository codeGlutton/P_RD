#pragma once

#include "RDMinimal.h"

class ACombatDiceCaptureActor;
class UImage;

namespace RDDiceCapturePreview
{
	P_RD_API int32 GetDefaultRenderTargetSize();
	P_RD_API FVector2D GetDefaultBrushSize();
	P_RD_API FVector GetCombatPreviewLocation(int32 GroupIndex, int32 DiceIndex);
	P_RD_API float GetCombatPreviewDiceScale();
	P_RD_API void ApplyCaptureBrush(UImage* DiceImage, ACombatDiceCaptureActor* DiceActor, FVector2D BrushSize);
	P_RD_API ACombatDiceCaptureActor* SpawnCaptureActor(UWorld* World, UObject* RenderTargetOuter, const FVector& PreviewLocation, int32 RenderTargetSize);
	P_RD_API void DestroyCaptureActors(TArray<TObjectPtr<ACombatDiceCaptureActor>>& DiceActors);
}
