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
	const FName DiceStaggerSkillAssetName(TEXT("DA_NomalDefense_Common"));
	const FName DiceSwapSkillAssetName(TEXT("DA_NomalHeal_Common"));

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
    // 조준/세부 행동 선택은 자유롭게 바꿀 수 있지만, 실제 스킬이 성공하면 행동권 하나를 사용한다.
    mConsumesTurn = true;
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
	mDiceDisplacementIsStagger = false;
	mDiceDisplacementIsSwap = false;
    mDiceDisplacementWasReported = false;
    mDiceDisplacementCollisionReported = false;
	mDiceDisplacementStarted = false;
	mDiceDisplacementFinished = false;
	mSkillPresentationFinished = false;
    mIsFixedIntentCast = false;
	mIsEnemySignatureDisplacement = false;
	mEnemySignatureSkillName = FText::GetEmpty();
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
				TryStartSkillDisplacement(Context, SkillData);
			}
			FinishSkillAction();
            });

		FOnTriggerSkillMotionUI TriggerCallback;
		TriggerCallback.BindWeakLambda(this, [this](const FActiveSkillContext& Context, const UStaticSkillData* SkillData)
		{
			if (mDiceDisplacementStarted == false)
			{
				TryStartSkillDisplacement(Context, SkillData);
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

bool USRPGSkillAction::TryStartSkillDisplacement(
	const FActiveSkillContext& Context,
	const UStaticSkillData* SkillData)
{
	return TryStartDiceDisplacement(Context, SkillData)
		|| TryStartEnemySignatureDisplacement(Context);
}

bool USRPGSkillAction::TryStartEnemySignatureDisplacement(const FActiveSkillContext& Context)
{
	UEnemyUnitModel* Enemy = Cast<UEnemyUnitModel>(mInstigator.Get());
	if (mIsFixedIntentCast == false || Enemy == nullptr)
	{
		return false;
	}

	const ESRPGEnemyMovementRole Role = Enemy->GetMovementRole();
	if (Role != ESRPGEnemyMovementRole::Anchor
		&& Role != ESRPGEnemyMovementRole::Flanker)
	{
		return false;
	}

	// 실제로 맞은 유닛만 움직인다. 플레이어를 우선해 광역 오사 상황에서도 주 공격의
	// 물리 결과가 안정적으로 읽히고, 플레이어가 없으면 첫 번째 아군 오사 대상을 사용한다.
	UUnitModel* DisplacementTarget = nullptr;
	for (IBoardCombatTarget* CombatTarget : Context.mResolvedCombatTargets)
	{
		UUnitModel* Candidate = Cast<UUnitModel>(CombatTarget);
		if (Candidate == nullptr || Candidate == Enemy || Candidate->IsTargetable() == false)
		{
			continue;
		}
		if (DisplacementTarget == nullptr || Candidate->IsPlayerUnitModel())
		{
			DisplacementTarget = Candidate;
		}
		if (Candidate->IsPlayerUnitModel())
		{
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

	const FTileIndex EnemyTile = Enemy->GetTileTransform().mIndex;
	const FTileIndex TargetTile = DisplacementTarget->GetTileTransform().mIndex;
	mDiceDisplacementTarget = DisplacementTarget;
	mDiceDisplacementPath = { TargetTile };
	mDiceDisplacementBlocker = nullptr;
	mDiceDisplacementDiceValue = 0;
	mDiceDisplacementIsPull = Role == ESRPGEnemyMovementRole::Flanker;
	mDiceDisplacementIsThrow = false;
	mDiceDisplacementIsStagger = false;
	mDiceDisplacementIsSwap = false;
	mDiceDisplacementWasReported = false;
	mDiceDisplacementCollisionReported = false;
	mDiceDisplacementStarted = true;
	mDiceDisplacementFinished = false;
	mIsEnemySignatureDisplacement = true;
	mEnemySignatureSkillName = Role == ESRPGEnemyMovementRole::Flanker
		? NSLOCTEXT("EnemySkill", "SpiderWebPull", "거미줄 견인")
		: NSLOCTEXT("EnemySkill", "MushroomSporePush", "포자 밀치기");

	FTileIndex Step;
	if (Role == ESRPGEnemyMovementRole::Flanker)
	{
		const int32 DeltaX = EnemyTile.mX - TargetTile.mX;
		const int32 DeltaY = EnemyTile.mY - TargetTile.mY;
		const int32 AbsX = FMath::Abs(DeltaX);
		const int32 AbsY = FMath::Abs(DeltaY);
		Step = FTileIndex(
			AbsX >= AbsY ? FMath::Sign(DeltaX) : 0,
			AbsY >= AbsX ? FMath::Sign(DeltaY) : 0);
		// 거미 몸 안으로 겹치지 않고 인접 칸에서 멈춘다.
		if (FMath::Max(AbsX, AbsY) <= 1)
		{
			mDiceDisplacementFinished = true;
			FinishSkillAction();
			return true;
		}
	}
	else
	{
		Step = FTileIndex(
			FMath::Sign(TargetTile.mX - EnemyTile.mX),
			FMath::Sign(TargetTile.mY - EnemyTile.mY));
	}

	const FTileIndex Destination(TargetTile.mX + Step.mX, TargetTile.mY + Step.mY);
	if (TileMap->IsValidIndex(Destination)
		&& TileMap->CanPlace(Destination, DisplacementTarget))
	{
		mDiceDisplacementPath.Add(Destination);
	}
	else
	{
		for (UBoardActorModel* Actor : TileMap->GetActorsOnTile(Destination, ETileLayerFlag::All))
		{
			if (Actor != nullptr
				&& Actor != DisplacementTarget
				&& Actor != Enemy
				&& EnumHasAnyFlags(Actor->GetBlockLayerFlags(), DisplacementTarget->GetTileLayerFlags()))
			{
				mDiceDisplacementBlocker = Actor;
				break;
			}
		}
	}

	if (mDiceDisplacementPath.Num() < 2)
	{
		ReportDiceDisplacementIfMoved();
		mDiceDisplacementFinished = true;
		FinishSkillAction();
		return true;
	}

	BroadcastDiceDisplacementPath(Role == ESRPGEnemyMovementRole::Flanker
		? EForcedMovePresentationType::Pull
		: EForcedMovePresentationType::Push);
	StartDiceDisplacementStep(1);
	return true;
}

bool USRPGSkillAction::TryStartDiceDisplacement(const FActiveSkillContext& Context, const UStaticSkillData* SkillData)
{
    if (SkillData == nullptr
        || (SkillData->GetFName() != DicePushSkillAssetName
            && SkillData->GetFName() != DicePullSkillAssetName
            && SkillData->GetFName() != DiceStaggerSkillAssetName
            && SkillData->GetFName() != DiceSwapSkillAssetName)
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
	const bool bIsThrow = SkillData->GetFName() == DicePushSkillAssetName;
	const bool bIsStagger = SkillData->GetFName() == DiceStaggerSkillAssetName;
	const bool bIsSwap = SkillData->GetFName() == DiceSwapSkillAssetName;
    const FTileIndex InstigatorTile = mInstigator->GetTileTransform().mIndex;
    const FTileIndex TargetTile = DisplacementTarget->GetTileTransform().mIndex;
    const int32 RequestedDistance = FMath::Max(Context.mDiceSum, 1);
	const int32 DistanceToInstigator = FMath::Max(
		FMath::Abs(TargetTile.mX - InstigatorTile.mX),
		FMath::Abs(TargetTile.mY - InstigatorTile.mY));
	int32 PullDistance = 0;
	if (bIsPull)
	{
		const int32 DestinationToInstigator = mDiceDisplacementDestination == FTileIndex::Invalid
			? MAX_int32
			: FMath::Max(
				FMath::Abs(mDiceDisplacementDestination.mX - InstigatorTile.mX),
				FMath::Abs(mDiceDisplacementDestination.mY - InstigatorTile.mY));
		if (DestinationToInstigator != 1)
		{
			return false;
		}
		PullDistance = FMath::Max(
			FMath::Abs(mDiceDisplacementDestination.mX - TargetTile.mX),
			FMath::Abs(mDiceDisplacementDestination.mY - TargetTile.mY));
	}
	int32 ThrowDistance = 0;
	FTileIndex ThrowStep;
	if (bIsThrow)
	{
		if (mDiceDisplacementDestination == FTileIndex::Invalid
			|| DistanceToInstigator != 1)
		{
			return false;
		}
		const int32 DestinationDeltaX = mDiceDisplacementDestination.mX - TargetTile.mX;
		const int32 DestinationDeltaY = mDiceDisplacementDestination.mY - TargetTile.mY;
		ThrowStep = FTileIndex(FMath::Sign(DestinationDeltaX), FMath::Sign(DestinationDeltaY));
		const int32 SelectedDistance = FMath::Max(FMath::Abs(DestinationDeltaX), FMath::Abs(DestinationDeltaY));
		int32 WeightValue = StaticCast<int32>(ESRPGDisplacementWeight::Medium);
		if (const UEnemyUnitModel* EnemyTarget = Cast<UEnemyUnitModel>(DisplacementTarget))
		{
			WeightValue = StaticCast<int32>(EnemyTarget->GetDisplacementWeight());
		}
		ThrowDistance = FMath::Min(
			SelectedDistance,
			FMath::Clamp(RequestedDistance + 1 - WeightValue, 1, 4));
	}
	const int32 DesiredDistance = bIsPull
		? PullDistance
		: ThrowDistance;

    mDiceDisplacementTarget = DisplacementTarget;
	mDiceDisplacementBlocker = nullptr;
    mDiceDisplacementDiceValue = Context.mDiceSum;
    mDiceDisplacementIsPull = bIsPull;
	mDiceDisplacementIsThrow = bIsThrow;
	mDiceDisplacementIsStagger = bIsStagger;
	mDiceDisplacementIsSwap = bIsSwap;
    mDiceDisplacementWasReported = false;
    mDiceDisplacementCollisionReported = false;
	mDiceDisplacementStarted = true;
	mDiceDisplacementFinished = false;
	if (bIsStagger)
	{
		mDiceDisplacementPath = { TargetTile };
		if (USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
		{
			CombatModel->ReportPlayerStagger(DisplacementTarget, Context.mDiceSum);
		}
		mDiceDisplacementFinished = true;
		FinishSkillAction();
		return true;
	}
	if (bIsSwap)
	{
		if (DistanceToInstigator != 1)
		{
			mDiceDisplacementFinished = true;
			FinishSkillAction();
			return true;
		}
		mDiceDisplacementPath = { TargetTile, InstigatorTile };
		TSharedPtr<FPresentationBarrier> SwapBarrier = FPresentationBarrier::Make(
			FOnFinishPresentation::CreateWeakLambda(this, [this, TargetTile, InstigatorTile]()
			{
				if (UTileMapModel* ActiveTileMap = GetTileMap())
				{
					ActiveTileMap->CompleteActorMovement(mInstigator.Get());
					ActiveTileMap->CompleteActorMovement(mDiceDisplacementTarget);
				}
				if (USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
				{
					CombatModel->ReportPlayerDisplacement(
						mDiceDisplacementTarget,
						TargetTile,
						InstigatorTile,
						mDiceDisplacementDiceValue,
						ESRPGPlayerDisplacementType::Swap);
				}
				mDiceDisplacementWasReported = true;
				mDiceDisplacementFinished = true;
				FinishSkillAction();
			}));
		if (TileMap->StartActorSwap(mInstigator.Get(), DisplacementTarget, SwapBarrier) == false)
		{
			mDiceDisplacementFinished = true;
			FinishSkillAction();
		}
		return true;
	}
	if (bIsPull)
	{
		// 갈고리는 플레이어 주변에서 직접 고른 빈칸까지 적을 끌어온다.
		mDiceDisplacementPath = TileMap->GetPullPathToDestination(
			TargetTile,
			mDiceDisplacementDestination,
			DesiredDistance);
	}
	else if (bIsThrow)
	{
		mDiceDisplacementPath.Reset();
		mDiceDisplacementPath.Add(TargetTile);
		for (int32 Distance = 1; Distance <= ThrowDistance; ++Distance)
		{
			const FTileIndex Candidate(
				TargetTile.mX + ThrowStep.mX * Distance,
				TargetTile.mY + ThrowStep.mY * Distance);
			if (TileMap->IsValidIndex(Candidate) == false
				|| TileMap->CanPlace(Candidate, DisplacementTarget) == false)
			{
				break;
			}
			mDiceDisplacementPath.Add(Candidate);
		}
	}
	else
	{
		return false;
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
			const int32 DeltaX = mDiceDisplacementDestination.mX - Destination.mX;
			const int32 DeltaY = mDiceDisplacementDestination.mY - Destination.mY;
			const int32 AbsX = FMath::Abs(DeltaX);
			const int32 AbsY = FMath::Abs(DeltaY);
			CollisionStep = FTileIndex(
				AbsX >= AbsY ? FMath::Sign(DeltaX) : 0,
				AbsY >= AbsX ? FMath::Sign(DeltaY) : 0);
		}
		else if (bIsThrow)
		{
			CollisionStep = ThrowStep;
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
		mDiceDisplacementFinished = true;
		FinishSkillAction();
		return true;
	}

	BroadcastDiceDisplacementPath(bIsPull
		? EForcedMovePresentationType::Pull
		: EForcedMovePresentationType::Throw);

    StartDiceDisplacementStep(1);
    return true;
}

void USRPGSkillAction::StartDiceDisplacementStep(int32 StepIndex)
{
    if (mDiceDisplacementTarget == nullptr || mDiceDisplacementPath.IsValidIndex(StepIndex) == false)
    {
        ReportDiceDisplacementIfMoved();
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
		if (mIsEnemySignatureDisplacement)
		{
			UEnemyUnitModel* Enemy = Cast<UEnemyUnitModel>(mInstigator.Get());
			const FTileIndex CurrentTile = mDiceDisplacementTarget->GetTileTransform().mIndex;
			if (Enemy != nullptr
				&& mDiceDisplacementWasReported == false
				&& CurrentTile != mDiceDisplacementPath[0])
			{
				CombatModel->ReportEnemySkillDisplacement(
					Enemy,
					mDiceDisplacementTarget,
					mDiceDisplacementPath[0],
					CurrentTile,
					mEnemySignatureSkillName,
					false);
				mDiceDisplacementWasReported = true;
			}
			if (Enemy != nullptr
				&& mDiceDisplacementCollisionReported == false
				&& mDiceDisplacementBlocker != nullptr)
			{
				CombatModel->ReportEnemySkillCollision(
					Enemy,
					mDiceDisplacementTarget,
					mDiceDisplacementBlocker,
					mEnemySignatureSkillName);
				mDiceDisplacementCollisionReported = true;
			}
			return;
		}

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

