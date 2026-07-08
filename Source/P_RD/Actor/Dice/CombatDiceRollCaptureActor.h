#pragma once

#include "RDMinimal.h"
#include "GameFramework/Actor.h"

#include "CombatDiceRollCaptureActor.generated.h"

class ACombatDicePreviewActor;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UProceduralMeshComponent;
class USceneCaptureComponent2D;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture;
class UTextureRenderTarget2D;

struct FCombatDiceRollPhysicsSpec
{
	int32 mFaceCount = 6;
	TArray<int32> mFaceValues;
	TArray<TObjectPtr<UTexture>> mFaceTextures;
	FLinearColor mDiceColor = FLinearColor::White;
};

/**
 * @brief 여러 주사위를 하나의 숨겨진 물리 테이블에서 굴리고 RenderTarget으로 캡처하는 액터.
 *
 * @details
 * ProceduralMeshComponent는 충돌/물리 전용으로 숨겨 두고, 실제 화면에 보이는 주사위는
 * ACombatDicePreviewActor가 담당한다. Tick마다 물리 바디 Transform을 시각 주사위에 복사한 뒤
 * SceneCapture2D로 한 장의 투명 RenderTarget에 캡처한다.
 */
UCLASS()
class P_RD_API ACombatDiceRollCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	ACombatDiceRollCaptureActor();

	void InitializeCapture(UObject* RenderTargetOuter, int32 RenderTargetSize);
	void InitializeCapture(UObject* RenderTargetOuter, int32 RenderTargetWidth, int32 RenderTargetHeight);
	void ConfigureDice(const TArray<FCombatDiceRollPhysicsSpec>& DiceSpecs);
	void StartRoll(int32 RollSeed);
	void CaptureDice() const;

	/** @brief 굴림이 끝난 주사위들을 한 줄로(결과면이 위로 오도록) 부드럽게 정렬하는 애니메이션을 시작한다. */
	void StartAlign();

	bool IsRollActive() const { return mRollActive; }
	bool IsRollComplete() const { return mRollComplete; }
	bool IsAlignActive() const { return mAligning; }
	bool IsAlignComplete() const { return mAlignComplete; }
	void GetSettledFaceOrdinals(TArray<int32>& OutFaceOrdinals) const;

	UTextureRenderTarget2D* GetRenderTarget() const;
	UMaterialInstanceDynamic* GetCaptureMaterial() const;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InitializeSceneComponents();
	void InitializeCaptureMaterial();
	void ClearDice();
	void BuildTableCollision();
	UProceduralMeshComponent* CreatePhysicsBody(int32 DiceIndex, int32 FaceCount);
	void ResetDiceBodyPose(UProceduralMeshComponent* PhysicsBody, int32 DiceIndex, int32 DiceCount, FRandomStream& Stream) const;
	void ConstrainDiceToTable(UProceduralMeshComponent* PhysicsBody) const;
	void ApplyRollRattleKicks(float PreviousRollElapsed);
	void SyncVisualDiceToPhysics();
	int32 GetSettledFaceOrdinal(int32 DiceIndex) const;
	int32 GetSettledFaceValue(int32 DiceIndex) const;
	void UpdateAlign(float DeltaSeconds);
	FQuat ComputeAlignedFaceUpQuat(int32 DiceIndex) const;

private:
	UPROPERTY(VisibleAnywhere, Category = "DiceRoll")
	TObjectPtr<USceneComponent> mSceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "DiceRoll")
	TObjectPtr<USceneCaptureComponent2D> mSceneCaptureComponent;

	UPROPERTY(VisibleAnywhere, Category = "DiceRoll")
	TObjectPtr<UStaticMeshComponent> mFloorCollision;

	UPROPERTY(VisibleAnywhere, Category = "DiceRoll")
	TObjectPtr<UPointLightComponent> mKeyLight;

	UPROPERTY(VisibleAnywhere, Category = "DiceRoll")
	TObjectPtr<UPointLightComponent> mFillLight;

	UPROPERTY(VisibleAnywhere, Category = "DiceRoll")
	TArray<TObjectPtr<UStaticMeshComponent>> mWallCollisions;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACombatDicePreviewActor>> mVisualDiceActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> mPhysicsDiceBodies;

	UPROPERTY(Transient)
	TArray<int32> mPhysicsFaceCounts;

	/** @brief 주사위별 면값(물리 면 index → 표시 숫자). 크기순 정렬에 쓴다. */
	TArray<TArray<int32>> mDiceFaceValues;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> mRenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> mCaptureMaterialTemplate;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> mCaptureMaterial;

	float mRollElapsed = 0.0f;
	int32 mRollSeed = 0;
	bool mRollActive = false;
	bool mRollComplete = false;

	bool mAligning = false;
	bool mAlignComplete = false;
	float mAlignElapsed = 0.0f;
	TArray<FTransform> mAlignStartTransforms;
	TArray<FTransform> mAlignTargetTransforms;

	static constexpr float PhysicsDiceRadius = 44.0f;
	// 파바바박 손맛: sleep 조기 종료 없이 이 시간까지 물리를 유지한 뒤 결과를 확정한다.
	static constexpr float RollForceCompleteSeconds = 2.30f;
	static constexpr float AlignDurationSeconds = 0.50f;
	static constexpr float AlignRowSpacing = 66.0f;
};
