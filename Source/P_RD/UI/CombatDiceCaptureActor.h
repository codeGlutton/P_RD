#pragma once

#include "RDMinimal.h"
#include "UI/CombatDicePreviewActor.h"

#include "CombatDiceCaptureActor.generated.h"

class USceneCaptureComponent2D;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextureRenderTarget2D;

/**
 * @brief 3D 주사위를 투명 RenderTarget으로 캡처하는 프리뷰 액터
 *
 * @details
 * UMG UViewport는 모바일에서 배경 알파가 보존되지 않아 검은 사각형이 남을 수 있다.
 * 이 액터는 기존 3D 주사위 구성은 그대로 쓰되, SceneCapture2D가 전용 RenderTarget에 주사위만 그리게 한다.
 * UI는 이 RenderTarget을 UImage로 표시하므로 주사위 뒤의 배경은 실제 HUD/패널 배경이 그대로 보인다.
 */
UCLASS()
class P_RD_API ACombatDiceCaptureActor : public ACombatDicePreviewActor
{
	GENERATED_BODY()

public:
	ACombatDiceCaptureActor();

	/** @brief UI Image에 연결할 투명 RenderTarget을 만들고 SceneCapture2D에 연결한다. */
	void InitializeCapture(UObject* RenderTargetOuter, int32 RenderTargetSize);

	/** @brief 현재 주사위 상태를 RenderTarget에 즉시 다시 캡처한다. */
	void CaptureDice() const;

	/** @brief UI Image brush에 연결할 RenderTarget을 반환한다. */
	UTextureRenderTarget2D* GetRenderTarget() const;

	/** @brief RenderTarget 알파를 UI 투명도로 해석하는 머티리얼 인스턴스를 반환한다. */
	UMaterialInstanceDynamic* GetCaptureMaterial() const;

private:
	/** @brief 메인 카메라에는 보이지 않고 SceneCapture에만 보이도록 주사위 Primitive 표시 상태를 맞춘다. */
	void SetDicePrimitivesSceneCaptureOnly();

private:
	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<USceneCaptureComponent2D> mSceneCaptureComponent;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> mRenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> mCaptureMaterialTemplate;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> mCaptureMaterial;
};
