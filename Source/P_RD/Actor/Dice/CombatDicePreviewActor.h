#pragma once

#include "RDMinimal.h"
#include "GameFramework/Actor.h"

#include "CombatDicePreviewActor.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextRenderComponent;
class UPointLightComponent;
class UFont;

/**
 * @brief 전투 HUD의 주사위 굴림 연출에만 쓰는 3D 미리보기 액터
 *
 * @details
 * 실제 전투 월드에 놓는 액터가 아니라 UMG Viewport가 가진 별도 미리보기 월드에 생성된다.
 * 그래서 타일맵 위를 굴러다니지 않고, 화면 정면의 고정된 연출 영역 안에서만 회전한다.
 *
 * 주사위는 StaticMesh 큐브와 6개의 TextRender 숫자로 직접 구성한다.
 * 아직 실제 주사위 결과 API가 없으므로, 최종 정지 면은 HUD 연출 쪽에서 정한 임시 값으로 맞춘다.
 */
UCLASS()
class P_RD_API ACombatDicePreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ACombatDicePreviewActor();

	/** @brief 지정한 눈이 정면에 보이는 정지 회전값을 반환한다. */
	static FRotator GetSettledFaceRotation(int32 FaceValue);

	/** @brief 주사위 연출 중 회전값을 적용한다. */
	void SetDiceRotation(const FRotator& NewRotation);

	/**
	 * @brief 6개 면에 표시할 숫자를 갱신한다.
	 *
	 * @details
	 * 주사위 면 배경은 공용 텍스처를 쓰고, 숫자는 TextRender로 따로 올린다.
	 * 그래서 주사위 데이터 API가 각 면 값을 다르게 내려줘도 텍스처를 다시 만들 필요 없이 숫자만 바꾸면 된다.
	 *
	 * @param NewFaceValues 1번 면부터 6번 면까지 표시할 값. 부족한 면은 기존 값을 유지한다.
	 */
	void SetFaceValues(const TArray<int32>& NewFaceValues);

	/**
	 * @brief 지정한 눈이 정면에 보이도록 정지 자세를 맞춘다.
	 *
	 * @param FaceValue 정면에 보여줄 주사위 눈. 1~6 범위를 벗어나면 1~6으로 보정한다.
	 */
	void SettleToFace(int32 FaceValue);

	/**
	 * @brief 주사위 종류를 구분할 색을 적용한다.
	 *
	 * @param NewColor 주사위 몸체에 적용할 색
	 */
	void SetDiceColor(const FLinearColor& NewColor);

	/** @brief Viewport 배경판 표시 여부를 바꾼다. */
	void SetBackdropVisible(bool bVisible);

private:
	/** @brief 주사위/배경의 기본 SceneComponent 계층을 만든다. */
	void InitializeSceneComponents();

	/** @brief UI 캡처에 맞는 고정 조명을 만든다. */
	void InitializeLighting();

	/** @brief 기본 큐브 메시를 주사위와 배경판에 적용한다. */
	void ApplyCubeMesh(UStaticMesh* CubeMesh);

	/** @brief 숫자 TextRender에 사용할 기본 폰트를 로드한다. */
	void LoadDiceNumberFont();

	/** @brief 주사위/배경/모서리 머티리얼을 준비한다. */
	void InitializeMaterials();

	/** @brief 정면 실루엣 보강용 모서리 메시를 만든다. */
	void InitializeEdgeMeshes(UStaticMesh* CubeMesh);

	/** @brief 6개 면의 숫자 TextRender를 만든다. */
	void InitializeFaceTexts();

	/**
	 * @brief 주사위 한 면에 숫자 TextRender를 붙인다.
	 *
	 * @param Name 컴포넌트 이름
	 * @param FaceText 표시할 숫자
	 * @param RelativeLocation 큐브 중심 기준 면 위치
	 * @param RelativeRotation 숫자가 면 바깥을 바라보게 하는 회전
	 */
	void AddFaceText(int32 FaceValue, const TCHAR* Name, const FVector& RelativeLocation, const FRotator& RelativeRotation);

	/** @brief 지정한 면의 TextRender 숫자를 바꾼다. */
	void SetFaceText(int32 FaceValue, const FText& FaceText);

	/**
	 * @brief 주사위 정면 윤곽에 얇은 선을 붙여 큐브 실루엣을 보강한다.
	 *
	 * @details
	 * 기본 Cube 메시만 쓰면 모바일 작은 Viewport에서 면 경계가 흐려져 납작한 판처럼 보일 수 있다.
	 * 다만 모든 모서리를 다 그리면 뒤쪽 빈 공간까지 면처럼 보일 수 있으므로, 정면 윤곽만 보강한다.
	 */
	void AddEdgeMesh(const TCHAR* Name, UStaticMesh* CubeMesh, const FVector& RelativeLocation, const FVector& RelativeScale);

private:
	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<USceneComponent> mSceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<USceneComponent> mDiceRoot;

	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<UStaticMeshComponent> mBackdropMesh;

	UPROPERTY(VisibleAnywhere, Category = "Dice")
	TObjectPtr<UStaticMeshComponent> mDiceMesh;

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
	TArray<TObjectPtr<UTextRenderComponent>> mFaceTexts;

	UPROPERTY(Transient)
	TObjectPtr<UFont> mDiceNumberFont;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> mEdgeMeshes;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> mEdgeMaterial;
};
