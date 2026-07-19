#include "SRPGFramework/SRPGMoveAction.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "Pawn/UnitModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Pawn/Enemy/EnemyUnitModel.h"
#include "SRPGFramework/SRPGEnemyIntent.h"

FSRPGMoveCommand::FSRPGMoveCommand()
{
    mCommandType = ESRPGCommandType::MoveCast;
    mRequestedAction = USRPGMoveAction::StaticClass();
}

USRPGMoveAction::USRPGMoveAction()
{
    mActionType = ESRPGActionType::InPlayAction;
    mConsumesTurn = false;
}

ESRPGCommandResult USRPGMoveAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = Super::HandleCommand(Command);
    if (Result == ESRPGCommandResult::Handled)
    {
        return Result;
    }

    /* 생성 시 예약된 이동 명령에서 확정 경로를 수신 (빌드 액션이 실어 보낸 경로) */
    if (Command.Get().GetCommandType() == ESRPGCommandType::MoveCast)
    {
        const FSRPGMoveCommand& MoveCommand = Command.Get<FSRPGMoveCommand>();
        mPathTileIndexes = MoveCommand.mPathTileIndexes;
        mUseFixedIntent = MoveCommand.mUseFixedIntent;
		mIsWarriorCharge = MoveCommand.mIsWarriorCharge;
		mDicePower = MoveCommand.mDicePower;
		mConsumeMovementPoints = MoveCommand.mConsumeMovementPoints;
        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }

    return ESRPGCommandResult::Ignored;
}

void USRPGMoveAction::OnBeginAction()
{
    Super::OnBeginAction();

    // 경로가 없거나 시작 타일만 있으면 이동 없이 즉시 종료
    if (mPathTileIndexes.Num() < 2)
    {
        MarkActionCompleted(ESRPGActionResult::Succeeded);
        return;
    }

    // 코너링에 진입/진출 타일 정보가 필요하므로 전체 경로를 전달
    {
        UTileMapModel* TileMap = GetTileMap();
        TArray<FVector> PathWorldLocations;
        PathWorldLocations.Reserve(mPathTileIndexes.Num());
        for (const FTileIndex& TileIndex : mPathTileIndexes)
        {
            PathWorldLocations.Add(TileMap->TileToWorldLocation(TileIndex));
        }
        mInstigator->OnStartMovePath.Broadcast(PathWorldLocations);
    }

    // 1번 타일로 이동하는 것부터 시작 (0번은 현재 타일).
    // 해당 타일로 이동이 완료되면, 베리어가 끝나고 다시 다음 스텝으로 가는 걸 마지막 타일까지 반복.
    // 시뮬레이션모드에서는 베리어가 없으므로 즉각 다음 스텝으로 진행하므로 문제 없음
    StartStep(1);
}

void USRPGMoveAction::OnEndAction()
{
    Super::OnEndAction();

    /* 이동을 정상 완료한 경우, 사용한 이동 포인트를 이동 유닛에게 차감 통지한다 */

    if (mConsumeMovementPoints && mActionResult == ESRPGActionResult::Succeeded && mPathTileIndexes.Num() >= 2)
    {
        // 소모 이동 포인트 = 밟은 칸 수 (경로 칸 수 - 시작 타일)
        const int32 SpentPoint = mPathTileIndexes.Num() - 1;

        // 이동력 차감
        if (UAttributeSetComponentModel* AttrComp = mInstigator->GetAttributeComponentModel())
        {
            AttrComp->ApplyModToAttribute(UCombatTargetAttributeSet::GetMovementAttribute(), ETacticalModOp::Additive, -static_cast<float>(SpentPoint));
        }
    }
}

void USRPGMoveAction::StartStep(int32 StepIndex)
{
    checkf(mPathTileIndexes.IsValidIndex(StepIndex) == true, TEXT("이동 경로 인덱스 오류"));

    if (mUseFixedIntent && ValidateFixedIntentStep(StepIndex) == false)
    {
        return;
    }

    mCurrentStepIndex = StepIndex;

    UTileMapModel* TileMap = GetTileMap();
	if (mIsWarriorCharge && TryStartWarriorChargeImpact(StepIndex))
	{
		return;
	}
	if (TileMap->CanPlace(mPathTileIndexes[StepIndex], mInstigator.Get()) == false)
	{
		MarkActionCompleted(mIsWarriorCharge
			? ESRPGActionResult::Succeeded : ESRPGActionResult::Cancelled);
		return;
	}

    // 직전 타일에서 이번 타일을 바라볼때의 방향 계산
    // 직전->현재와 현재->다음 방향을 보간해서 자연스럽게 코너링 할 계획
    const ETileActorDirection Direction = UTileMapModel::TileDeltaToDirection(
        mPathTileIndexes[StepIndex - 1],
        mPathTileIndexes[StepIndex],
        mInstigator->GetTileTransform().mDirection);

    // 다음 타일로 이동 (모델의 논리적 위치 변경)
    // 점유는 즉시 하고, 도착 오버랩 통지는 CompleteStep가 함
    const FTileTransform NextTransform(mPathTileIndexes[StepIndex], Direction);
    TileMap->StartActorMovement(NextTransform, mInstigator.Get());

    // 이동 후 받을 베리어 생성
    // 액션이 먼저 파괴될 수 있으므로 WeakLambda로 보호.
    TSharedPtr<FPresentationBarrier> Barrier = FPresentationBarrier::Make(
        FOnFinishPresentation::CreateWeakLambda(this, [this]() {
            OnStepPresentationFinished();
            }));

    // 이번 스텝 도착 후 최종 목적지까지 남은 경로 거리 계산
    // -> 제동거리에 들어가면 감속할 때 사용
    float RemainingPathDistance = 0.0f;
    for (int32 i = StepIndex; i < mPathTileIndexes.Num() - 1; ++i)
    {
        RemainingPathDistance += FVector::Dist(
            TileMap->TileToWorldLocation(mPathTileIndexes[i]),
            TileMap->TileToWorldLocation(mPathTileIndexes[i + 1]));
    }

    // OnStartMoveStep을 구독하고 있던 뷰가 이동 시작 (뷰의 물리적 위치 변경)
    mInstigator->OnStartMoveStep.Broadcast(NextTransform, TileMap->TileToWorldTransform(NextTransform), Barrier, RemainingPathDistance);
}

bool USRPGMoveAction::ValidateFixedIntentStep(int32 StepIndex)
{
    checkf(mPathTileIndexes.IsValidIndex(StepIndex), TEXT("고정 이동 경로 인덱스 오류"));

    USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
    checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

    const FTileIndex CurrentTile = mInstigator->GetTileTransform().mIndex;
    const FTileIndex PlannedPreviousTile = mPathTileIndexes[StepIndex - 1];
    if (CurrentTile != PlannedPreviousTile)
    {
        CombatModel->ReportFixedIntentPathDisrupted(
            mInstigator.Get(),
            FText::Format(
                NSLOCTEXT("EnemyIntent", "PathOriginChanged", "출발 위치가 ({0},{1})로 바뀌어 예정 경로 취소"),
                FText::AsNumber(CurrentTile.mX),
                FText::AsNumber(CurrentTile.mY)));
        MarkActionCompleted(ESRPGActionResult::Cancelled);
        return false;
    }

    UTileMapModel* TileMap = GetTileMap();
    const FTileIndex NextTile = mPathTileIndexes[StepIndex];
    UBoardActorModel* Blocker = nullptr;
    for (UBoardActorModel* Actor : TileMap->GetActorsOnTile(NextTile, ETileLayerFlag::All))
    {
        if (Actor != nullptr
            && Actor != mInstigator.Get()
            && EnumHasAnyFlags(Actor->GetBlockLayerFlags(), mInstigator->GetTileLayerFlags()))
        {
            Blocker = Actor;
            break;
        }
    }

    if (TileMap->CanPlace(NextTile, mInstigator.Get()) == false || Blocker != nullptr)
    {
        if (Blocker != nullptr)
        {
            CombatModel->ResolveFixedIntentCollision(mInstigator.Get(), Blocker);
        }
        else
        {
            CombatModel->ReportFixedIntentPathDisrupted(
                mInstigator.Get(),
                NSLOCTEXT("EnemyIntent", "InvalidFixedPath", "예정 경로가 막혀 이동 취소"));
        }
        MarkActionCompleted(ESRPGActionResult::Cancelled);
        return false;
    }

    return true;
}

void USRPGMoveAction::CompleteStep()
{
    // 현재(도착) 타일의 오버랩 통지 — 함정/장판 등 타일 효과가 여기서 발동
    GetTileMap()->CompleteActorMovement(mInstigator.Get());
}

void USRPGMoveAction::OnStepPresentationFinished()
{
    // ActionPlay 상태가 아니면 중단됐다고 보고 바로 리턴
    if (mActionPhase != ESRPGActionPhase::ActionPlay)
    {
        return;
    }

    // 현재 타일 도착 처리 -> 함정/장판 등 오버랩 관련된 처리
    CompleteStep();

	if (mWarriorChargeStopAfterPlayerStep)
	{
		mWarriorChargeStopAfterPlayerStep = false;
		MarkActionCompleted(ESRPGActionResult::Succeeded);
		return;
	}

    // 1) 마지막 타일이면 이동 완료.
    if (mCurrentStepIndex >= mPathTileIndexes.Num() - 1)
    {
        MarkActionCompleted(ESRPGActionResult::Succeeded);
    }
    // 2) 마지막 타일이 아니면 다음 타일로 이동
    else
    {
        StartStep(mCurrentStepIndex + 1);
    }
}

UBoardActorModel* USRPGMoveAction::FindBlockingActor(
	const FTileIndex& TileIndex,
	const UBoardActorModel* MovingActor) const
{
	if (MovingActor == nullptr)
	{
		return nullptr;
	}
	for (UBoardActorModel* Actor : GetTileMap()->GetActorsOnTile(TileIndex, ETileLayerFlag::All))
	{
		if (Actor != nullptr && Actor != MovingActor
			&& EnumHasAnyFlags(Actor->GetBlockLayerFlags(), MovingActor->GetTileLayerFlags()))
		{
			return Actor;
		}
	}
	return nullptr;
}

bool USRPGMoveAction::TryStartWarriorChargeImpact(int32 StepIndex)
{
	UTileMapModel* TileMap = GetTileMap();
	const FTileIndex ImpactTile = mPathTileIndexes[StepIndex];
	UBoardActorModel* ImpactActor = FindBlockingActor(ImpactTile, mInstigator.Get());
	if (ImpactActor == nullptr)
	{
		return false;
	}

	UEnemyUnitModel* EnemyTarget = Cast<UEnemyUnitModel>(ImpactActor);
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	if (EnemyTarget == nullptr)
	{
		// 돌진을 벽/장애물로 끝내면 실제 충돌 피해를 받고 현재 칸에서 정지한다.
		if (CombatModel != nullptr)
		{
			CombatModel->ReportPlayerDisplacementCollision(
				mInstigator.Get(), ImpactActor, ESRPGPlayerDisplacementType::Push);
		}
		MarkActionCompleted(ESRPGActionResult::Succeeded);
		return true;
	}

	mWarriorChargeTarget = EnemyTarget;
	mWarriorChargeBlocker = nullptr;
	mWarriorChargeFromTile = EnemyTarget->GetTileTransform().mIndex;
	mWarriorChargeResumePlayerStep = StepIndex;
	mWarriorChargePushPath = { mWarriorChargeFromTile };

	const ESRPGDisplacementWeight Weight = EnemyTarget->GetDisplacementWeight();
	const int32 PushDistance = Weight == ESRPGDisplacementWeight::Light
		? FMath::Clamp(1 + mDicePower / 4, 1, 2)
		: (Weight == ESRPGDisplacementWeight::Medium && mDicePower >= 4 ? 1 : 0);
	mWarriorChargeContinueAfterImpact = Weight == ESRPGDisplacementWeight::Light;
	mWarriorChargeStopAfterPlayerStep = Weight == ESRPGDisplacementWeight::Medium && PushDistance > 0;

	const FTileIndex PreviousTile = mPathTileIndexes[StepIndex - 1];
	const FTileIndex PushStep(
		FMath::Sign(ImpactTile.mX - PreviousTile.mX),
		FMath::Sign(ImpactTile.mY - PreviousTile.mY));
	FTileIndex Cursor = ImpactTile;
	for (int32 Distance = 0; Distance < PushDistance; ++Distance)
	{
		const FTileIndex Candidate(Cursor.mX + PushStep.mX, Cursor.mY + PushStep.mY);
		if (TileMap->IsValidIndex(Candidate) == false
			|| TileMap->CanPlace(Candidate, EnemyTarget) == false)
		{
			mWarriorChargeBlocker = FindBlockingActor(Candidate, EnemyTarget);
			break;
		}
		mWarriorChargePushPath.Add(Candidate);
		Cursor = Candidate;
	}

	if (mWarriorChargePushPath.Num() < 2)
	{
		// 초중량, 위력이 모자란 중량, 뒤가 막힌 적은 기사와 정면 충돌한다.
		if (CombatModel != nullptr)
		{
			CombatModel->ReportPlayerDisplacementCollision(
				EnemyTarget,
				mWarriorChargeBlocker != nullptr ? mWarriorChargeBlocker.Get() : mInstigator.Get(),
				ESRPGPlayerDisplacementType::Push);
			CombatModel->ReportPlayerStagger(EnemyTarget, mDicePower);
		}
		MarkActionCompleted(ESRPGActionResult::Succeeded);
		return true;
	}

	TArray<FVector> PushWorldPath;
	PushWorldPath.Reserve(mWarriorChargePushPath.Num());
	for (const FTileIndex& TileIndex : mWarriorChargePushPath)
	{
		PushWorldPath.Add(TileMap->TileToWorldLocation(TileIndex));
	}
	EnemyTarget->OnStartForcedMovePath.Broadcast(PushWorldPath, EForcedMovePresentationType::Push);
	StartWarriorChargePushStep(1);
	return true;
}

void USRPGMoveAction::StartWarriorChargePushStep(int32 StepIndex)
{
	if (mWarriorChargeTarget == nullptr || mWarriorChargePushPath.IsValidIndex(StepIndex) == false)
	{
		FinishWarriorChargeImpact();
		return;
	}
	UTileMapModel* TileMap = GetTileMap();
	const FTileIndex NextTile = mWarriorChargePushPath[StepIndex];
	if (TileMap->CanPlace(NextTile, mWarriorChargeTarget) == false)
	{
		mWarriorChargeBlocker = FindBlockingActor(NextTile, mWarriorChargeTarget);
		FinishWarriorChargeImpact();
		return;
	}

	mWarriorChargePushStepIndex = StepIndex;
	const FTileTransform NextTransform(NextTile, mWarriorChargeTarget->GetTileTransform().mDirection);
	TileMap->StartActorMovement(NextTransform, mWarriorChargeTarget);
	TSharedPtr<FPresentationBarrier> Barrier = FPresentationBarrier::Make(
		FOnFinishPresentation::CreateWeakLambda(this, [this]()
		{
			OnWarriorChargePushStepFinished();
		}));
	float RemainingPathDistance = 0.0f;
	for (int32 Index = StepIndex; Index < mWarriorChargePushPath.Num() - 1; ++Index)
	{
		RemainingPathDistance += FVector::Dist(
			TileMap->TileToWorldLocation(mWarriorChargePushPath[Index]),
			TileMap->TileToWorldLocation(mWarriorChargePushPath[Index + 1]));
	}
	mWarriorChargeTarget->OnStartMoveStep.Broadcast(
		NextTransform,
		TileMap->TileToWorldTransform(NextTransform),
		Barrier,
		RemainingPathDistance);
}

void USRPGMoveAction::OnWarriorChargePushStepFinished()
{
	if (mActionPhase != ESRPGActionPhase::ActionPlay || mWarriorChargeTarget == nullptr)
	{
		return;
	}
	GetTileMap()->CompleteActorMovement(mWarriorChargeTarget);
	if (mWarriorChargePushStepIndex >= mWarriorChargePushPath.Num() - 1)
	{
		FinishWarriorChargeImpact();
	}
	else
	{
		StartWarriorChargePushStep(mWarriorChargePushStepIndex + 1);
	}
}

void USRPGMoveAction::FinishWarriorChargeImpact()
{
	if (mWarriorChargeTarget == nullptr)
	{
		MarkActionCompleted(ESRPGActionResult::Succeeded);
		return;
	}
	if (USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
	{
		const FTileIndex CurrentTile = mWarriorChargeTarget->GetTileTransform().mIndex;
		if (CurrentTile != mWarriorChargeFromTile)
		{
			CombatModel->ReportPlayerDisplacement(
				mWarriorChargeTarget,
				mWarriorChargeFromTile,
				CurrentTile,
				mDicePower,
				ESRPGPlayerDisplacementType::Push);
		}
		if (mWarriorChargeBlocker != nullptr)
		{
			CombatModel->ReportPlayerDisplacementCollision(
				mWarriorChargeTarget,
				mWarriorChargeBlocker,
				ESRPGPlayerDisplacementType::Push);
		}
		if (mWarriorChargeStopAfterPlayerStep)
		{
			CombatModel->ReportPlayerStagger(mWarriorChargeTarget, mDicePower);
		}
	}

	mWarriorChargeTarget = nullptr;
	mWarriorChargeBlocker = nullptr;
	mWarriorChargePushPath.Reset();
	if (mWarriorChargeContinueAfterImpact || mWarriorChargeStopAfterPlayerStep)
	{
		mWarriorChargeContinueAfterImpact = false;
		StartStep(mWarriorChargeResumePlayerStep);
	}
	else
	{
		MarkActionCompleted(ESRPGActionResult::Succeeded);
	}
}

UTileMapModel* USRPGMoveAction::GetTileMap() const
{
    // 전투 모델을 월드 서브시스템에서 바로 받아 타일 맵을 꺼낸다 (턴 컨텍스트 체인 의존 제거)
    USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
    checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

    UTileMapModel* TileMap = CombatModel->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));
    return TileMap;
}
