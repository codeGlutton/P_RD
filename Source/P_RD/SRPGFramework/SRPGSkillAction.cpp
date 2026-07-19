#include "SRPGFramework/SRPGSkillAction.h"

#include "Pawn/UnitModel.h"
#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "TAS/Passive/TacticalPassive.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

namespace
{
    const FName DicePushSkillAssetName(TEXT("DA_SwordNormalSmash_Common"));
    const FName DicePullSkillAssetName(TEXT("DA_SwordBlade_Rare"));

    bool HasFixedEffectBlocker(
        const UTileMapModel* TileMap,
        const FTileIndex& TileIndex,
        const UUnitModel* Instigator)
    {
        if (TileMap == nullptr)
        {
            return false;
        }

        for (UBoardActorModel* Actor : TileMap->GetActorsOnTile(
            TileIndex,
            ETileLayerFlag::Unit | ETileLayerFlag::Obstacle))
        {
            if (Actor != nullptr && Actor != Instigator)
            {
                return true;
            }
        }
        return false;
    }

    TArray<FTileIndex> BuildFixedExecutionEffectTiles(
        const UTileMapModel* TileMap,
        const FSRPGSkillCastCommand& Command,
        const UStaticSkillData* SkillData,
        const UUnitModel* Instigator)
    {
        if (SkillData == nullptr
            || SkillData->mIsPenetration
            || SkillData->mEffectPattern == EEffectPattern::Single
            || SkillData->mEffectPattern == EEffectPattern::Square)
        {
            return Command.mFixedEffectTileIndexes;
        }

        TArray<FTileIndex> Result;
        if (SkillData->mEffectPattern == EEffectPattern::Beam)
        {
            // 계획 당시의 빔 타일 순서는 그대로 두되, 실행 시 새로 생긴 첫 점유 칸에서 실제 투사체가 멈춘다.
            for (const FTileIndex& TileIndex : Command.mFixedEffectTileIndexes)
            {
                Result.Add(TileIndex);
                if (HasFixedEffectBlocker(TileMap, TileIndex, Instigator))
                {
                    break;
                }
            }
            return Result;
        }

        // Cross/Star는 고정된 각 광선 방향을 독립적으로 처리한다. 한 방향의 첫 점유 칸은 맞지만 그 뒤 칸은 가려진다.
        TSet<FTileIndex> BlockedDirections;
        for (const FTileIndex& TileIndex : Command.mFixedEffectTileIndexes)
        {
            if (TileIndex == Command.mTargetIndex)
            {
                Result.AddUnique(TileIndex);
                continue;
            }

            const FTileIndex Direction(
                FMath::Sign(TileIndex.mX - Command.mTargetIndex.mX),
                FMath::Sign(TileIndex.mY - Command.mTargetIndex.mY));
            if (BlockedDirections.Contains(Direction))
            {
                continue;
            }

            Result.AddUnique(TileIndex);
            if (HasFixedEffectBlocker(TileMap, TileIndex, Instigator))
            {
                BlockedDirections.Add(Direction);
            }
        }
        return Result;
    }
}

FSRPGSkillCastCommand::FSRPGSkillCastCommand()
{
    mCommandType = ESRPGCommandType::SkillCast;
    mRequestedAction = USRPGSkillAction::StaticClass();
}

USRPGSkillAction::USRPGSkillAction()
{
    mActionType = ESRPGActionType::InPlayAction;
    mConsumesTurn = false;
}

void USRPGSkillAction::OnBeginAction()
{
    Super::OnBeginAction();
}

void USRPGSkillAction::OnTickAction(float DeltaTime)
{
    Super::OnTickAction(DeltaTime);
}

void USRPGSkillAction::OnEndAction()
{
    Super::OnEndAction();

    mDiceDisplacementTarget = nullptr;
    mDiceDisplacementPath.Reset();
    mDiceDisplacementBlocker = nullptr;
    mDiceDisplacementStepIndex = 0;
    mDiceDisplacementDiceValue = 0;
	mDiceDisplacementDestination = FTileIndex::Invalid;
    mDiceDisplacementIsPull = false;
	mDiceDisplacementIsThrow = false;
    mDiceDisplacementWasReported = false;
    mDiceDisplacementCollisionReported = false;
	mDiceDisplacementStarted = false;
	mDiceDisplacementFinished = false;
	mSkillPresentationFinished = false;
    mIsFixedIntentCast = false;
}

ESRPGCommandResult USRPGSkillAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = Super::HandleCommand(Command);
    if (Result == ESRPGCommandResult::Handled)
    {
        return Result;
    }

    switch (Command.Get().GetCommandType())
    {
    case ESRPGCommandType::SkillCast:
    {
        /* 스킬 실행 */

        const FSRPGSkillCastCommand& SkillCastCommand = Command.Get<FSRPGSkillCastCommand>();

        USkillComponentModel* SkillCompModel = mInstigator->GetSkillComponentModel();
        checkf(SkillCompModel != nullptr, TEXT("스킬 컴포넌트 모델 nullptr"));

        UTileMapModel* TileMap = GetTileMap();
        checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

        if (SkillCompModel->IsAnySkillActivated() == true)
        {
            // 이미 스킬 실행 중 무시
            return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
        }

        mIsFixedIntentCast = SkillCastCommand.mUseFixedIntent;
		mDiceDisplacementDestination = SkillCastCommand.mDisplacementDestination;

        FOnEndSkillUI Callback;
        Callback.AddWeakLambda(this, [this](const FActiveSkillContext& Context, const UStaticSkillData* SkillData) {
            if (mIsFixedIntentCast)
            {
                if (USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
                {
                    CombatModel->ResolveFixedIntentAttack(mInstigator.Get(), Context.mResolvedCombatTargets);
                }
            }

			mSkillPresentationFinished = true;
			// Hit 노티가 없는 비정상 몽타주/무연출 데이터만 종료 시점 폴백으로 처리한다.
			if (mDiceDisplacementStarted == false)
			{
				TryStartDiceDisplacement(Context, SkillData);
			}
			FinishSkillAction();
            });

		FOnTriggerSkillMotionUI TriggerCallback;
		TriggerCallback.BindWeakLambda(this, [this](const FActiveSkillContext& Context, const UStaticSkillData* SkillData)
		{
			if (mDiceDisplacementStarted == false)
			{
				TryStartDiceDisplacement(Context, SkillData);
			}
		});

        const FSkillEntry* SkillEntry = SkillCompModel->GetSkill(SkillCastCommand.mSkillIndex);
        const UStaticSkillData* StaticSkillData = SkillEntry != nullptr ? SkillEntry->mData.Get() : nullptr;
        TArray<FTileIndex> FixedExecutionEffectTiles;
        const TArray<FTileIndex>* FixedEffectTiles = nullptr;
        if (SkillCastCommand.mUseFixedIntent)
        {
            FixedExecutionEffectTiles = BuildFixedExecutionEffectTiles(
                TileMap,
                SkillCastCommand,
                StaticSkillData,
                mInstigator.Get());
            FixedEffectTiles = &FixedExecutionEffectTiles;
        }
        SkillCompModel->ActivateSkill(
            TileMap,
            SkillCastCommand.mSkillIndex,
            SkillCastCommand.mTargetIndex,
            SkillCastCommand.mDiceSum,
            MoveTemp(Callback),
            FixedEffectTiles,
            SkillCastCommand.mAllowFriendlyFire,
			MoveTemp(TriggerCallback));

        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    }

    return ESRPGCommandResult::Ignored;
}

UTileMapModel* USRPGSkillAction::GetTileMap() const
{
    USRPGTurnContext* TurnContext = mParent.Get();
    if (TurnContext != nullptr)
    {
        USRPGCombatModel* CombatModel = TurnContext->GetParent();
        if (CombatModel != nullptr)
        {
            UTileMapModel* TileMap = CombatModel->GetTileMap();
            return TileMap;
        }
    }
    return nullptr;
}

bool USRPGSkillAction::TryStartDiceDisplacement(const FActiveSkillContext& Context, const UStaticSkillData* SkillData)
{
    if (SkillData == nullptr
        || (SkillData->GetFName() != DicePushSkillAssetName
            && SkillData->GetFName() != DicePullSkillAssetName)
        || mInstigator.IsValid() == false
        || mInstigator->IsPlayerUnitModel() == false)
    {
        return false;
    }

    UUnitModel* DisplacementTarget = nullptr;
    for (IBoardCombatTarget* CombatTarget : Context.mResolvedCombatTargets)
    {
        UUnitModel* Candidate = Cast<UUnitModel>(CombatTarget);
        if (Candidate != nullptr
            && Candidate != mInstigator.Get()
            && Candidate->IsTargetable()
            && Candidate->GetTileTransform().mIndex == Context.mTargetTileIndex)
        {
            DisplacementTarget = Candidate;
            break;
        }
    }

    if (DisplacementTarget == nullptr)
    {
        return false;
    }

    UTileMapModel* TileMap = GetTileMap();
    if (TileMap == nullptr)
    {
        return false;
    }

    const bool bIsPull = SkillData->GetFName() == DicePullSkillAssetName;
    const FTileIndex InstigatorTile = mInstigator->GetTileTransform().mIndex;
    const FTileIndex TargetTile = DisplacementTarget->GetTileTransform().mIndex;
    const int32 RequestedDistance = FMath::Max(Context.mDiceSum, 1);
    const int32 DistanceToInstigator = FMath::Max(
        FMath::Abs(TargetTile.mX - InstigatorTile.mX),
        FMath::Abs(TargetTile.mY - InstigatorTile.mY));
    const int32 DesiredDistance = bIsPull
		? FMath::Max(DistanceToInstigator - 1, 0)
		: RequestedDistance;

    mDiceDisplacementTarget = DisplacementTarget;
	mDiceDisplacementBlocker = nullptr;
    mDiceDisplacementDiceValue = Context.mDiceSum;
    mDiceDisplacementIsPull = bIsPull;
	mDiceDisplacementIsThrow = false;
    mDiceDisplacementWasReported = false;
    mDiceDisplacementCollisionReported = false;
	mDiceDisplacementStarted = true;
	mDiceDisplacementFinished = false;
	if (bIsPull)
	{
		// 갈고리는 사거리 안의 적을 발앞까지 끌어온다. 주사위 눈은 후속 던지기의 거리로 쓰여
		// 작은 눈도 위치 개입은 성립하고, 큰 눈은 더 강한 충돌을 만들게 한다.
		mDiceDisplacementPath = TileMap->GetPullPath(InstigatorTile, TargetTile, DesiredDistance);
	}
	else
	{
		mDiceDisplacementPath.Reset();
		mDiceDisplacementPath.Add(TargetTile);
		const FTileIndex Destination = TileMap->GetPushDestination(InstigatorTile, TargetTile, RequestedDistance);
		const FTileIndex PushStep(
			FMath::Sign(TargetTile.mX - InstigatorTile.mX),
			FMath::Sign(TargetTile.mY - InstigatorTile.mY));
		FTileIndex Current = TargetTile;
		while (Current != Destination)
		{
			Current = FTileIndex(Current.mX + PushStep.mX, Current.mY + PushStep.mY);
			mDiceDisplacementPath.Add(Current);
		}
	}
	if (mDiceDisplacementPath.IsEmpty())
	{
		return false;
	}
	const FTileIndex Destination = mDiceDisplacementPath.Last();

	// 실제 이동 가능 거리보다 짧게 멈췄다면 다음 칸의 충돌 대상을 기억한다. 당기는 주체의
	// 바로 앞에서 정상 정지한 경우(DesiredDistance 달성)는 충돌로 취급하지 않는다.
	const int32 MovedDistance = mDiceDisplacementPath.Num() - 1;
	if (MovedDistance < DesiredDistance)
	{
		FTileIndex CollisionStep;
		if (bIsPull)
		{
			const int32 DeltaX = InstigatorTile.mX - Destination.mX;
			const int32 DeltaY = InstigatorTile.mY - Destination.mY;
			const int32 AbsX = FMath::Abs(DeltaX);
			const int32 AbsY = FMath::Abs(DeltaY);
			CollisionStep = FTileIndex(
				AbsX >= AbsY ? FMath::Sign(DeltaX) : 0,
				AbsY >= AbsX ? FMath::Sign(DeltaY) : 0);
		}
		else
		{
			CollisionStep = FTileIndex(
				FMath::Sign(TargetTile.mX - InstigatorTile.mX),
				FMath::Sign(TargetTile.mY - InstigatorTile.mY));
		}
		const FTileIndex BlockedTile(
			Destination.mX + CollisionStep.mX,
			Destination.mY + CollisionStep.mY);
		for (UBoardActorModel* Actor : TileMap->GetActorsOnTile(BlockedTile, ETileLayerFlag::All))
		{
			if (Actor != nullptr
				&& Actor != mDiceDisplacementTarget
				&& Actor != mInstigator.Get()
				&& EnumHasAnyFlags(Actor->GetBlockLayerFlags(), mDiceDisplacementTarget->GetTileLayerFlags()))
			{
				mDiceDisplacementBlocker = Actor;
				break;
			}
		}
	}

	if (Destination == TargetTile)
	{
		ReportDiceDisplacementIfMoved();
		if (TryStartDiceFollowUpThrow())
		{
			return true;
		}
		mDiceDisplacementFinished = true;
		FinishSkillAction();
		return true;
	}

	BroadcastDiceDisplacementPath(bIsPull
		? EForcedMovePresentationType::Pull
		: EForcedMovePresentationType::Push);

    StartDiceDisplacementStep(1);
    return true;
}

void USRPGSkillAction::StartDiceDisplacementStep(int32 StepIndex)
{
    if (mDiceDisplacementTarget == nullptr || mDiceDisplacementPath.IsValidIndex(StepIndex) == false)
    {
        ReportDiceDisplacementIfMoved();
		if (TryStartDiceFollowUpThrow())
		{
			return;
		}
		mDiceDisplacementFinished = true;
        FinishSkillAction();
        return;
    }

    UTileMapModel* TileMap = GetTileMap();
    bool bBlocked = false;
    if (TileMap != nullptr)
    {
        for (UBoardActorModel* Actor : TileMap->GetActorsOnTile(mDiceDisplacementPath[StepIndex], ETileLayerFlag::All))
        {
            if (Actor != nullptr
                && Actor != mDiceDisplacementTarget
                && EnumHasAnyFlags(Actor->GetBlockLayerFlags(), mDiceDisplacementTarget->GetTileLayerFlags()))
            {
                bBlocked = true;
				mDiceDisplacementBlocker = Actor;
                break;
            }
        }
    }
    if (TileMap == nullptr
        || mDiceDisplacementTarget->GetTileTransform().mIndex != mDiceDisplacementPath[StepIndex - 1]
        || TileMap->CanPlace(mDiceDisplacementPath[StepIndex], mDiceDisplacementTarget) == false
        || bBlocked)
    {
        ReportDiceDisplacementIfMoved();
		if (TryStartDiceFollowUpThrow())
		{
			return;
		}
		mDiceDisplacementFinished = true;
        FinishSkillAction();
        return;
    }

    mDiceDisplacementStepIndex = StepIndex;
    // 강제 이동은 보행이 아니므로 이동 방향으로 몸을 돌리지 않고 기존 facing을 유지한다.
    const FTileTransform NextTransform(
        mDiceDisplacementPath[StepIndex],
        mDiceDisplacementTarget->GetTileTransform().mDirection);
    TileMap->StartActorMovement(NextTransform, mDiceDisplacementTarget);

    TSharedPtr<FPresentationBarrier> Barrier = FPresentationBarrier::Make(
        FOnFinishPresentation::CreateWeakLambda(this, [this]() {
            OnDiceDisplacementStepFinished();
            }));

    float RemainingPathDistance = 0.0f;
    for (int32 Index = StepIndex; Index < mDiceDisplacementPath.Num() - 1; ++Index)
    {
        RemainingPathDistance += FVector::Dist(
            TileMap->TileToWorldLocation(mDiceDisplacementPath[Index]),
            TileMap->TileToWorldLocation(mDiceDisplacementPath[Index + 1]));
    }

    mDiceDisplacementTarget->OnStartMoveStep.Broadcast(
        NextTransform,
        TileMap->TileToWorldTransform(NextTransform),
        Barrier,
        RemainingPathDistance);
}

void USRPGSkillAction::OnDiceDisplacementStepFinished()
{
    if (mActionPhase != ESRPGActionPhase::ActionPlay || mDiceDisplacementTarget == nullptr)
    {
        return;
    }

    if (UTileMapModel* TileMap = GetTileMap())
    {
        TileMap->CompleteActorMovement(mDiceDisplacementTarget);
    }

    if (mDiceDisplacementStepIndex >= mDiceDisplacementPath.Num() - 1)
    {
        ReportDiceDisplacementIfMoved();
		if (TryStartDiceFollowUpThrow())
		{
			return;
		}
		mDiceDisplacementFinished = true;
        FinishSkillAction();
    }
    else
    {
        StartDiceDisplacementStep(mDiceDisplacementStepIndex + 1);
    }
}

void USRPGSkillAction::ReportDiceDisplacementIfMoved()
{
    if (mDiceDisplacementTarget == nullptr || mDiceDisplacementPath.IsEmpty())
    {
        return;
    }

    if (USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
    {
		const ESRPGPlayerDisplacementType DisplacementType = mDiceDisplacementIsThrow
			? ESRPGPlayerDisplacementType::Throw
			: (mDiceDisplacementIsPull
				? ESRPGPlayerDisplacementType::Pull
				: ESRPGPlayerDisplacementType::Push);
		const FTileIndex CurrentTile = mDiceDisplacementTarget->GetTileTransform().mIndex;
		if (mDiceDisplacementWasReported == false && CurrentTile != mDiceDisplacementPath[0])
		{
			CombatModel->ReportPlayerDisplacement(
				mDiceDisplacementTarget,
				mDiceDisplacementPath[0],
				CurrentTile,
				mDiceDisplacementDiceValue,
				DisplacementType);
			mDiceDisplacementWasReported = true;
		}
		if (mDiceDisplacementCollisionReported == false && mDiceDisplacementBlocker != nullptr)
		{
			CombatModel->ReportPlayerDisplacementCollision(
				mDiceDisplacementTarget,
				mDiceDisplacementBlocker,
				DisplacementType);
			mDiceDisplacementCollisionReported = true;
		}
    }
}

bool USRPGSkillAction::TryStartDiceFollowUpThrow()
{
	if (mDiceDisplacementIsPull == false
		|| mDiceDisplacementIsThrow
		|| mDiceDisplacementTarget == nullptr
		|| mInstigator.IsValid() == false
		|| mDiceDisplacementDestination == FTileIndex::Invalid)
	{
		return false;
	}

	UTileMapModel* TileMap = GetTileMap();
	if (TileMap == nullptr)
	{
		return false;
	}

	const FTileIndex PlayerTile = mInstigator->GetTileTransform().mIndex;
	const FTileIndex TargetTile = mDiceDisplacementTarget->GetTileTransform().mIndex;
	const int32 PullDeltaX = PlayerTile.mX - TargetTile.mX;
	const int32 PullDeltaY = PlayerTile.mY - TargetTile.mY;
	if (FMath::Max(FMath::Abs(PullDeltaX), FMath::Abs(PullDeltaY)) != 1
		|| mDiceDisplacementDestination == TargetTile)
	{
		// 발앞 칸을 고르면 여기서 끝난다. 이것이 순수한 "끌어당기기" 선택이다.
		return false;
	}

	const int32 DestinationDeltaX = mDiceDisplacementDestination.mX - PlayerTile.mX;
	const int32 DestinationDeltaY = mDiceDisplacementDestination.mY - PlayerTile.mY;
	const FTileIndex ThrowStep(FMath::Sign(DestinationDeltaX), FMath::Sign(DestinationDeltaY));
	const int32 SelectedDistance = FMath::Max(
		FMath::Abs(DestinationDeltaX),
		FMath::Abs(DestinationDeltaY));
	if ((ThrowStep.mX == 0 && ThrowStep.mY == 0) || SelectedDistance <= 0)
	{
		return false;
	}

	int32 WeightValue = StaticCast<int32>(ESRPGDisplacementWeight::Medium);
	if (const UEnemyUnitModel* EnemyTarget = Cast<UEnemyUnitModel>(mDiceDisplacementTarget))
	{
		WeightValue = StaticCast<int32>(EnemyTarget->GetDisplacementWeight());
	}
	const int32 MaxThrowDistance = FMath::Clamp(
		FMath::Max(mDiceDisplacementDiceValue, 1) + 1 - WeightValue,
		1,
		4);
	const int32 ThrowDistance = FMath::Min(SelectedDistance, MaxThrowDistance);

	mDiceDisplacementPath.Reset();
	mDiceDisplacementPath.Add(TargetTile);
	mDiceDisplacementBlocker = nullptr;
	for (int32 Distance = 1; Distance <= ThrowDistance; ++Distance)
	{
		// 자동 반대편이 아니라 플레이어가 고른 8방향으로만 진행한다. 대상이 당겨져 서 있는
		// 발앞 칸과 겹치는 첫 후보는 건너뛰고, 이후 빈 칸 또는 첫 충돌 칸까지 검사한다.
		const FTileIndex Candidate(
			PlayerTile.mX + ThrowStep.mX * Distance,
			PlayerTile.mY + ThrowStep.mY * Distance);
		if (Candidate == TargetTile)
		{
			continue;
		}
		if (TileMap->IsValidIndex(Candidate) == false
			|| TileMap->CanPlace(Candidate, mDiceDisplacementTarget) == false)
		{
			for (UBoardActorModel* Actor : TileMap->GetActorsOnTile(Candidate, ETileLayerFlag::All))
			{
				if (Actor != nullptr
					&& Actor != mDiceDisplacementTarget
					&& Actor != mInstigator.Get()
					&& EnumHasAnyFlags(Actor->GetBlockLayerFlags(), mDiceDisplacementTarget->GetTileLayerFlags()))
				{
					mDiceDisplacementBlocker = Actor;
					break;
				}
			}
			break;
		}
		mDiceDisplacementPath.Add(Candidate);
	}

	mDiceDisplacementIsThrow = true;
	mDiceDisplacementWasReported = false;
	mDiceDisplacementCollisionReported = false;
	mDiceDisplacementStepIndex = 0;
	if (mDiceDisplacementPath.Num() < 2)
	{
		ReportDiceDisplacementIfMoved();
		return false;
	}

	BroadcastDiceDisplacementPath(EForcedMovePresentationType::Throw);
	StartDiceDisplacementStep(1);
	return true;
}

void USRPGSkillAction::BroadcastDiceDisplacementPath(EForcedMovePresentationType PresentationType) const
{
	const UTileMapModel* TileMap = GetTileMap();
	if (TileMap == nullptr || mDiceDisplacementTarget == nullptr || mDiceDisplacementPath.Num() < 2)
	{
		return;
	}

	TArray<FVector> PathWorldLocations;
	PathWorldLocations.Reserve(mDiceDisplacementPath.Num());
	for (const FTileIndex& TileIndex : mDiceDisplacementPath)
	{
		PathWorldLocations.Add(TileMap->TileToWorldLocation(TileIndex));
	}
	mDiceDisplacementTarget->OnStartForcedMovePath.Broadcast(PathWorldLocations, PresentationType);
}

void USRPGSkillAction::FinishSkillAction()
{
	if (mSkillPresentationFinished == false
		|| (mDiceDisplacementStarted && mDiceDisplacementFinished == false))
	{
		return;
	}
    MarkActionCompleted(ESRPGActionResult::Succeeded);
}

