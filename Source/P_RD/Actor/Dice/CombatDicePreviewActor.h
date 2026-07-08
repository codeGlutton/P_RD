#pragma once

#include "RDMinimal.h"
#include "GameFramework/Actor.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextRenderComponent;
class UPointLightComponent;
class UFont;
class UTexture;
class UProceduralMeshComponent;

namespace RDDicePolyhedron { struct FDicePolyhedron; }

#include "CombatDicePreviewActor.generated.h"

USTRUCT()
struct FRDCombatDiceFaceTextureSet
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture>> mTextures;
};

/**
 * @brief 전투 HUD의 주사위 굴림 연출에만 쓰는 3D 미리보기 액터
 *
 * @details
 * UMG Viewport가 가진 별도 미리보기 월드에 생성돼, 화면 정면의 고정 연출 영역 안에서만 회전한다.
 *
 * 주사위 몸체는 면 수(d2/d4/d6/d8/d10/d12/d20)에 맞춘 블랭크 스태틱메시를 교체해 쓴다.
 * 숫자는 TextRender로, 면 텍스처는 면 모양 절차 메시 위 머티리얼로 올려 런타임 교체가 가능하다.
 * (주사위 본체는 에셋. DicePolyhedron은 텍스처 커버/숫자 배치/안착 회전을 위한 면 레이아웃 데이터로 쓴다.)
 */
UCLASS()
class P_RD_API ACombatDicePreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ACombatDicePreviewActor();

	/** @brief 주사위 종류(면 수)에 맞는 다면체 메시 + 면 숫자 TextRender를 다시 만든다. */
	void SetDiceType(int32 FaceCount);

protected:
	// 메시/TextRender 생성은 런타임 작업(RegisterComponent)이라 생성자(CDO) 대신 여기서 기본 주사위를 만든다.
	virtual void BeginPlay() override;

	// 메시/숫자를 새로 만든 뒤 호출(서브클래스가 캡처 재등록/재촬영에 사용).
	virtual void OnDiceRebuilt() {}

public:
	/** @brief 지정한 눈이 정면에 보이는 정지 회전값을 반환한다(현재 주사위 종류 기준). */
	FRotator GetSettledFaceRotation(int32 FaceValue) const;

	/** @brief 주사위 연출 중 회전값을 적용한다. */
	void SetDiceRotation(const FRotator& NewRotation);

	/** @brief 각 면에 표시할 숫자를 갱신한다(부족한 면은 기존 값 유지). */
	void SetFaceValues(const TArray<int32>& NewFaceValues);

	/** @brief 각 면에 덮을 텍스처를 갱신한다(nullptr이면 기본 면 텍스처). */
	void SetFaceTextures(const TArray<TObjectPtr<UTexture>>& NewFaceTextures);

	/** @brief 각 면의 숫자/텍스처를 한 번에 갱신한다. */
	void SetFaceData(const TArray<int32>& NewFaceValues, const TArray<TObjectPtr<UTexture>>& NewFaceTextures);

	/** @brief 지정한 눈이 정면에 보이도록 정지 자세를 맞춘다. */
	void SettleToFace(int32 FaceValue);

	/** @brief 물리 바디 등 외부 Transform을 시각 주사위 전체에 적용한다. */
	void SetDiceWorldTransform(const FTransform& NewTransform);

	/** @brief 메인 카메라에는 숨기고 SceneCapture에서만 보이도록 Primitive 표시 상태를 바꾼다. */
	void SetDiceVisibleInSceneCaptureOnly(bool bSceneCaptureOnly);

	/** @brief 여러 주사위를 한 장면에서 캡처할 때 개별 프리뷰 라이트 중첩을 막는다. */
	void SetPreviewLightingEnabled(bool bEnabled);

	/** @brief 주사위 몸체 색(희귀도/상태 구분)을 적용한다. */
	void SetDiceColor(const FLinearColor& NewColor);

	/** @brief Viewport 배경판 표시 여부를 바꾼다. */
	void SetBackdropVisible(bool bVisible);

private:
	void InitializeSceneComponents();
	void InitializeLighting();
	void LoadDiceNumberFont();
	void LoadDiceMeshes();
	void InitializeMaterials();
	void LoadDefaultFaceTextures();

	/** @brief 면 수에 맞는 블랭크 스태틱메시로 교체하고 크기를 맞춘다. */
	void ApplyDiceMesh(int32 FaceCount);

	/** @brief 면 수만큼 TextRender 숫자를 만들어 각 면 중심에 바깥을 보게 배치한다. */
	void RebuildFaceTexts(int32 FaceCount);

	/** @brief 지정 면의 TextRender 숫자를 바꾼다(1-base). */
	void SetFaceText(int32 FaceIndex, const FText& FaceText);

	/** @brief 명시 텍스처가 없으면 현재 면 수에 맞는 기본 커버 텍스처를 반환한다. */
	UTexture* ResolveFaceTexture(int32 FaceIndex, const TArray<TObjectPtr<UTexture>>& NewFaceTextures) const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<USceneComponent> mSceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<USceneComponent> mDiceRoot;

	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<UStaticMeshComponent> mBackdropMesh;

	// 주사위 몸체(블랭크 메시). 면 수가 바뀌면 SetDiceType이 메시를 교체한다.
	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<UStaticMeshComponent> mDiceMesh;

	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<UStaticMeshComponent> mCoinRimMesh;

	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<UPointLightComponent> mFrontFillLight;

	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<UPointLightComponent> mSideFillLight;

	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<UPointLightComponent> mTopFillLight;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> mBackdropMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> mDiceMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> mCoinRimMaterial;

	// 면 수 → 블랭크 스태틱메시.
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UStaticMesh>> mDiceMeshAssets;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> mFaceMaterialTemplate;

	UPROPERTY(Transient)
	TObjectPtr<UTexture> mDefaultFaceTexture;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> mFaceMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> mFaceMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture>> mFaceTextureOverrides;

	UPROPERTY(Transient)
	TMap<int32, FRDCombatDiceFaceTextureSet> mDefaultFaceTexturesByCount;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> mFaceTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> mFaceTextStrokes;

	UPROPERTY(Transient)
	TObjectPtr<UFont> mDiceNumberFont;

	// 현재 만들어진 주사위의 면 수(안착/숫자 계산에 사용).
	int32 mCurrentFaceCount = 6;

	// 메시 바운딩 반지름을 이 값으로 맞춰 주사위 종류별 크기를 통일한다.
	static constexpr float DiceRadius = 96.0f;
};
