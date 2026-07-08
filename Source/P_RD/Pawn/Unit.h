/*****************************************************************//**
 * @file   Unit.h
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스 정의 파일
 * @author 모호재
 * @date   2026-04-25
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/ActorView.h"
#include "Actor/BoardActor/BoardSelectionTarget.h"
#include "GameFramework/Pawn.h"

#include "Unit.generated.h"

class UUnitModel;
struct FTileTransform;
struct FPresentationBarrier;

class USkeletalMeshComponent;
class UFloatingPawnMovement;
class UCapsuleComponent;
class UArrowComponent;

struct FApplyNiagaraSpawnData;

/**
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스
 */
UCLASS(abstract)
class P_RD_API AUnit : public APawn, public IActorView, public IBoardSelectionTarget
{
	GENERATED_BODY()

public:
	AUnit();

public:
	// @brief 이동 연출
	// @note 플레이어 및 몹 모두 이동해야 하니까 베이스 클래스에서 구현
	void Tick(float DeltaSeconds) override;

	// @brief 이동 연출할 때 현재속도
	// @note
	// 현재는 이동을 무브먼트컴포넌트 없이 직접 하므로 자체 값을 반환
	// TODO: 나중에 무브먼트컴포넌트 사용하면 이 코드는 수정 요망
	FVector GetVelocity() const override;

	/* IActorView 상속 */
public:
	// @brief 이동 델리게이트 구독
	void BindModel(UObjectModel* Model) override;
	// @brief 이동 델리게이트 구독 해제
	void UnbindModel(UObjectModel* Model) override;

protected:
	UObjectModel* GetModel_Internal() const override;

protected:
	// @brief 이동 시작 요청을 수신해서 이동 시작
	virtual void OnStartMoveStep(
		const FTileTransform& NextTileTransform,
		const FTransform& TargetWorldTransform,
		TSharedPtr<FPresentationBarrier> Barrier,
		float RemainingPathDistance);

	// @brief 방향 전환 요청을 수신해서 제자리 회전 시작
	virtual void OnRotate(
		const FRotator& TargetWorldRotation,
		TSharedPtr<FPresentationBarrier> Barrier);

protected:
	void SpawnHitVFX(const FApplyNiagaraSpawnData& NiagaraSpawnData, ETileActorDirection LocalDirection) const;

public:
	UCapsuleComponent* GetCapsuleComponent() const;
	USkeletalMeshComponent* GetMesh() const;
	UFloatingPawnMovement* GetCharacterMovement() const;

#if WITH_EDITORONLY_DATA
	UArrowComponent* GetArrowComponent() const;
#endif

protected:
	// @brief 최대 이동 속도 (cm/초)
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MaxMoveSpeed"))
	float mMaxMoveSpeed = 300.0f;

	// @brief 가속도 (cm/초^2). 출발할 때 속도를 올리는 데 사용
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Acceleration", ClampMin = "0.0"))
	float mAcceleration = 600.0f;

	// @brief 감속도 (cm/초^2). 도착할 때 속도를 내리는 데 사용. 낮을수록 제동거리가 길어져 부드럽게 멈춤
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Deceleration", ClampMin = "0.0"))
	float mDeceleration = 300.0f;

	// @brief 코너에서 바라보는 방향이 바뀌는 회전 속도 (도/초)
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RotationSpeed"))
	float mRotationSpeed = 360.0f;

private:
	// @brief 이번 이동의 목표 타일의 월드트랜스폼 (각 스텝마다 다음 타일이 목표 타일이 됨)
	FTransform mMoveTargetTransform = FTransform::Identity;
	// @brief 진행 중인 이동스텝의 연출 배리어
	TSharedPtr<FPresentationBarrier> mMoveBarrier;
	// @brief 현재 이동 속도 (cm/초).
	// @details
	// 가속/감속 계산에 쓰이는 스칼라 값.
	// 계산이 끝나면 mCurrentMoveVelocity에 벡터 값을 넣어줌
	float mCurrentMoveSpeed = 0.0f;
	// @brief 현재 이동 속도 벡터 (cm/초). GetVelocity()로 사용
	FVector mCurrentMoveVelocity = FVector::ZeroVector;
	// @brief 이번 스텝 목표 도착 후 최종 목적지까지 남은 경로 거리 (cm). 0이면 이번 목표가 최종 목적지
	float mRemainingPathDistance = 0.0f;

private:
	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CapsuleComp", AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> mCapsuleComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MeshComp", AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> mMeshComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MovementComp", AllowPrivateAccess = "true"))
	TObjectPtr<UFloatingPawnMovement> mMovementComp;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ArrowComp", AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> mArrowComp;
#endif

protected:
	TWeakObjectPtr<UUnitModel> mUnitModel;
};
