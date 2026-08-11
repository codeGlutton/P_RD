#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "UI/Combat/CombatUIModel.h"

#include "FunctionLibrary/CameraFunctionLibrary.h"

#include "Pawn/Camera/CombatCameraPawn.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "SRPGFramework/SRPGMoveAction.h"
#include "Pawn/UnitModel.h"

void USRPGCombatSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TStatId USRPGCombatSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USRPGCombatSubsystem, STATGROUP_Tickables);
}

void USRPGCombatSubsystem::BindModel(UObjectModel* Model)
{
	mCombatModel = Cast<USRPGCombatModel>(Model);

	if (mCombatModel != nullptr)
	{
		mCombatModel->OnBeginAnyTurnUI.AddUObject(this, &USRPGCombatSubsystem::MoveCameraOnBeginTurn);
		mCombatModel->OnBeginAnyTurnActionUI.AddUObject(this, &USRPGCombatSubsystem::FixCameraOnBeginMoveAction);
		mCombatModel->OnEndAnyTurnActionUI.AddUObject(this, &USRPGCombatSubsystem::ReleaseCameraOnEndMoveAction);
	}
}

void USRPGCombatSubsystem::UnbindModel(UObjectModel* Model)
{
	if (mCombatModel != nullptr)
	{
		mCombatModel->OnBeginAnyTurnUI.RemoveAll(this);
		mCombatModel->OnBeginAnyTurnActionUI.RemoveAll(this);
		mCombatModel->OnEndAnyTurnActionUI.RemoveAll(this);
	}

	mCombatModel.Reset();
}

UObjectModel* USRPGCombatSubsystem::GetModel_Internal() const
{
	return mCombatModel.Get();
}

void USRPGCombatSubsystem::MoveCameraOnBeginTurn(TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext)
{
	AActor* UnitActor = GetTurnOwnerView(TurnContext);
	if (UnitActor == nullptr)
	{
		return;
	}

	ACombatCameraPawn* CameraPawn = UCameraFunctionLibrary::GetMainCameraPawn(this);
	if (CameraPawn == nullptr)
	{
		return;
	}

	UCameraMovementComponent* CameraMovement = CameraPawn->GetCameraMovementComponent();
	if (CameraMovement == nullptr)
	{
		return;
	}

	// 카메라를 단순 유닛 위치로 이동 (고정하지 않음)
	CameraMovement->MoveToWorldPosition(UnitActor->GetActorLocation(), false);
}

void USRPGCombatSubsystem::FixCameraOnBeginMoveAction(TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action)
{
	/* 이동 액션일 때만 동작하도록 검사 */

	if (Action == nullptr || Action->IsA<USRPGMoveAction>() == false)
	{
		return;
	}

	AActor* UnitActor = GetTurnOwnerView(TurnContext);
	if (UnitActor == nullptr)
	{
		return;
	}

	ACombatCameraPawn* CameraPawn = UCameraFunctionLibrary::GetMainCameraPawn(this);
	if (CameraPawn == nullptr)
	{
		return;
	}

	UCameraMovementComponent* CameraMovement = CameraPawn->GetCameraMovementComponent();
	if (CameraMovement == nullptr)
	{
		return;
	}

	// 카메라를 이동 유닛 액터에 고정
	CameraMovement->StartFollowingActor(UnitActor);
}

void USRPGCombatSubsystem::ReleaseCameraOnEndMoveAction(TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action, ESRPGActionResult Result)
{
	/* 이동 액션일 때만 동작하도록 검사 */

	if (Action == nullptr || Action->IsA<USRPGMoveAction>() == false)
	{
		return;
	}

	ACombatCameraPawn* CameraPawn = UCameraFunctionLibrary::GetMainCameraPawn(this);
	if (CameraPawn == nullptr)
	{
		return;
	}

	UCameraMovementComponent* CameraMovement = CameraPawn->GetCameraMovementComponent();
	if (CameraMovement == nullptr)
	{
		return;
	}

	// 유닛 고정 해제
	CameraMovement->EndFollowingActor();
}

AActor* USRPGCombatSubsystem::GetTurnOwnerView(const USRPGTurnContext* TurnContext) const
{
	if (TurnContext == nullptr)
	{
		return nullptr;
	}

	UUnitModel* TurnOwnerUnit = TurnContext->GetOwner();
	if (TurnOwnerUnit == nullptr)
	{
		return nullptr;
	}

	return TurnOwnerUnit->GetView<AActor>();
}


