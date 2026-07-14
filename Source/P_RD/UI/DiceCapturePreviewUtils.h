#pragma once

#include "RDMinimal.h"

class ACombatDiceCaptureActor;
class UImage;
class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;

namespace RDDiceCapturePreview
{
	P_RD_API int32 GetDefaultRenderTargetSize();
	P_RD_API FVector2D GetDefaultBrushSize();
	P_RD_API FVector GetCombatPreviewLocation(int32 GroupIndex, int32 DiceIndex);
	P_RD_API float GetCombatPreviewDiceScale();
	P_RD_API void ApplyCaptureBrush(UImage* DiceImage, ACombatDiceCaptureActor* DiceActor, FVector2D BrushSize);
	/** @brief 공유 캡처 액터 구조에서 다이별 캡처 머티리얼을 Image 브러시에 직접 연결한다. */
	P_RD_API void ApplyCaptureMaterialBrush(UImage* DiceImage, UMaterialInstanceDynamic* CaptureMaterial, FVector2D BrushSize);
	/** @brief 다이별 표시용 투명 RT를 만든다(캡처 액터 내부 RT 설정과 동일 포맷). */
	P_RD_API UTextureRenderTarget2D* CreateDiceRenderTarget(UObject* Outer, int32 RenderTargetSize);
	P_RD_API ACombatDiceCaptureActor* SpawnCaptureActor(UWorld* World, UObject* RenderTargetOuter, const FVector& PreviewLocation, int32 RenderTargetSize);
	P_RD_API void DestroyCaptureActors(TArray<TObjectPtr<ACombatDiceCaptureActor>>& DiceActors);
}
