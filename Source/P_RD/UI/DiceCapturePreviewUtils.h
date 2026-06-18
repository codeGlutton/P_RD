#pragma once

#include "RDMinimal.h"

class ACombatDiceCaptureActor;
class UImage;

namespace RDDiceCapturePreview
{
	/** @brief HUD 주사위 프리뷰 RenderTarget 기본 크기. 품질/메모리 절충값이다. */
	P_RD_API int32 GetDefaultRenderTargetSize();
	/** @brief 기본 RenderTarget 크기와 같은 정사각 UImage brush 크기를 반환한다. */
	P_RD_API FVector2D GetDefaultBrushSize();
	/** @brief 캡처 액터를 전투 맵 밖 원거리 좌표에 슬롯별로 배치한다. */
	P_RD_API FVector GetCombatPreviewLocation(int32 GroupIndex, int32 DiceIndex);
	/** @brief 캡처용 주사위 액터 스케일 보정값을 반환한다. */
	P_RD_API float GetCombatPreviewDiceScale();
	/** @brief CaptureActor의 알파 합성 머티리얼을 UImage brush에 연결한다. */
	P_RD_API void ApplyCaptureBrush(UImage* DiceImage, ACombatDiceCaptureActor* DiceActor, FVector2D BrushSize);
	/** @brief 캡처 액터를 스폰하고 RenderTarget 초기화까지 수행한다. */
	P_RD_API ACombatDiceCaptureActor* SpawnCaptureActor(UWorld* World, UObject* RenderTargetOuter, const FVector& PreviewLocation, int32 RenderTargetSize);
	/** @brief 보유 중인 캡처 액터들을 안전하게 Destroy하고 배열을 비운다. */
	P_RD_API void DestroyCaptureActors(TArray<TObjectPtr<ACombatDiceCaptureActor>>& DiceActors);
}
