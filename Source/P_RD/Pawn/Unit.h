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

	/* IActorView 상속 */
protected:
	UObjectModel* GetModel_Internal() const override;

public:
	UCapsuleComponent* GetCapsuleComponent() const;
	USkeletalMeshComponent* GetMesh() const;
	UFloatingPawnMovement* GetCharacterMovement() const;

#if WITH_EDITORONLY_DATA
	UArrowComponent* GetArrowComponent() const;
#endif

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
