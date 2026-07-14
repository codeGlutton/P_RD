#pragma once

#include "RDMinimal.h"
#include "Actor/Dice/CombatDicePreviewActor.h"

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

	/**
	 * @brief 현재 주사위 상태를 지정 RenderTarget에 캡처한다(공유 캡처 액터용).
	 * @details 한 액터가 여러 다이를 순서대로 구성해가며 다이별 RT에 찍을 때 쓴다.
	 *          캡처 후 TextureTarget을 내부 RT로 복원해, 내부 캡처 경로(OnDiceRebuilt 등)가
	 *          마지막 대상 RT를 엉뚱하게 덮지 않게 한다.
	 */
	void CaptureDiceInto(UTextureRenderTarget2D* RenderTarget) const;

	/** @brief 지정 RT의 알파를 UI 투명도로 해석하는 캡처 머티리얼 인스턴스를 만든다(CDO 템플릿 사용). */
	static UMaterialInstanceDynamic* CreateCaptureMaterialFor(UObject* Outer, UTextureRenderTarget2D* RenderTarget);

	/** @brief UI Image brush에 연결할 RenderTarget을 반환한다. */
	UTextureRenderTarget2D* GetRenderTarget() const;

	/** @brief RenderTarget 알파를 UI 투명도로 해석하는 머티리얼 인스턴스를 반환한다. */
	UMaterialInstanceDynamic* GetCaptureMaterial() const;

protected:
	// 주사위 메시/숫자를 새로 만들면(타입 변경 등) 새 컴포넌트를 캡처 전용으로 다시 등록하고 재촬영한다.
	virtual void OnDiceRebuilt() override;

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
