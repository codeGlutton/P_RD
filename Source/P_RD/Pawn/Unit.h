/*****************************************************************//**
 * @file   Unit.h
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스 정의 파일
 * @author 모호재
 * @date   2026-04-25
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

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

	/* IActorView 상속 */
public:
	// @brief 이동 델리게이트 구독
	void BindModel(UObjectModel* Model) override;
	// @brief 이동 델리게이트 구독 해제
	void UnbindModel(UObjectModel* Model) override;

protected:
	UObjectModel* GetModel_Internal() const override;

	// @brief 이동 시작 요청을 수신해서 이동 시작
	virtual void OnStartMoveStep(
		const FTileTransform& NextTileTransform,
		const FTransform& TargetWorldTransform,
		TSharedPtr<FPresentationBarrier> Barrier);

public:
	UCapsuleComponent* GetCapsuleComponent() const;
	USkeletalMeshComponent* GetMesh() const;
	UFloatingPawnMovement* GetCharacterMovement() const;

#if WITH_EDITORONLY_DATA
	UArrowComponent* GetArrowComponent() const;
#endif

protected:
	// @brief 이동 속도 (cm/초)
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MoveSpeed"))
	float mMoveSpeed = 300.0f;

	// @brief 코너에서 바라보는 방향이 바뀌는 회전 속도 (도/초)
	UPROPERTY(Category = Move, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RotationSpeed"))
	float mRotationSpeed = 720.0f;

private:
	// @brief 이번 이동의 목표 타일의 월드트랜스폼 (각 스텝마다 다음 타일이 목표 타일이 됨)
	FTransform mMoveTargetTransform = FTransform::Identity;
	// @brief 진행 중인 이동스텝의 연출 배리어
	TSharedPtr<FPresentationBarrier> mMoveBarrier;

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
