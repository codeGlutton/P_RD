/*****************************************************************//**
 * @file   Unit.h
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스 정의 파일
 * @author 모호재, 이문환
 * @date   2026-04-25
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/ActorView.h"
#include "Actor/BoardActor/BoardCombatTargetView.h"
#include "Actor/BoardActor/BoardSelectionTargetView.h"
#include "GameFramework/Pawn.h"

#include "Unit.generated.h"

class UUnitModel;
struct FTileTransform;
struct FPresentationBarrier;

class USkeletalMeshComponent;
class UCapsuleComponent;
class UArrowComponent;
class USkeletonSkillAnimationComponent;
class UBoardMovementPresentationComponent;

/**
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스
 */
UCLASS(abstract)
class P_RD_API AUnit : public APawn, public IActorView, public IBoardCombatTargetView, public IBoardSelectionTargetView
{
	GENERATED_BODY()

public:
	AUnit();

	/* APawn 상속 */
public:
	// @brief 이동 연출할 때 현재속도
	// @note 이동 연출 컴포넌트가 계산한 속도를 반환 (애니메이션이 사용)
	FVector GetVelocity() const override;

protected:
	// @brief 이동 연출 컴포넌트에 캡슐 반높이를 바닥 오프셋으로 전달
	void BeginPlay() override;

	/* IActorView 상속 */
public:
	// @brief 이동 델리게이트 구독
	void BindModel(UObjectModel* Model) override;
	// @brief 이동 델리게이트 구독 해제
	void UnbindModel(UObjectModel* Model) override;

protected:
	UObjectModel* GetModel_Internal() const override;

	/* IBoardCombatTargetView 상속 */
public:
	USkillAnimationComponent* GetSkillAnimationComponent() const override;
	UCombatTargetVFXTimelineComponent* GetCombatTargetVFXTimelineComponent() const override;
	UPrimitiveComponent* GetTargetMeshComponent() const override;

public:
	UCapsuleComponent* GetCapsuleComponent() const;
	USkeletalMeshComponent* GetMesh() const;
	UArrowComponent* GetArrowComponent() const;
	UBoardMovementPresentationComponent* GetBoardMovementPresentationComponent() const;

private:
	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CapsuleComp", AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> mCapsuleComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MeshComp", AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> mMeshComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MovementPresentationComp", AllowPrivateAccess = "true"))
	TObjectPtr<UBoardMovementPresentationComponent> mMovementPresentationComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ArrowComp", AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> mArrowComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "SkillAnimationComp", AllowPrivateAccess = "true"))
	TObjectPtr<USkeletonSkillAnimationComponent> mSkillAnimationComp;
	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatTargetVFXTimelineComp", AllowPrivateAccess = "true"))
	TObjectPtr<UCombatTargetVFXTimelineComponent> mCombatTargetVFXTimelineComp;

protected:
	TWeakObjectPtr<UUnitModel> mUnitModel;
};
