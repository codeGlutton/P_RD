#include "SRPGFramework/SRPGSkillAction.h"

#include "Pawn/UnitModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "TAS/Passive/TacticalPassive.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

namespace
{
    const FName DicePushSkillAssetName(TEXT("DA_SwordNormalSmash_Common"));

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

    mDicePushTarget = nullptr;
    mDicePushPath.Reset();
    mDicePushStepIndex = 0;
    mDicePushDiceValue = 0;
    mDicePushWasReported = false;
	mDicePushStarted = false;
	mDicePushFinished = false;
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
			if (mDicePushStarted == false)
			{
				TryStartDicePush(Context, SkillData);
			}
			FinishSkillAction();
            });

		FOnTriggerSkillMotionUI TriggerCallback;
		TriggerCallback.BindWeakLambda(this, [this](const FActiveSkillContext& Context, const UStaticSkillData* SkillData)
		{
			if (mDicePushStarted == false)
			{
				TryStartDicePush(Context, SkillData);
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

bool USRPGSkillAction::TryStartDicePush(const FActiveSkillContext& Context, const UStaticSkillData* SkillData)
{
    if (SkillData == nullptr
        || SkillData->GetFName() != DicePushSkillAssetName
        || mInstigator.IsValid() == false
        || mInstigator->IsPlayerUnitModel() == false)
    {
        return false;
    }

    UUnitModel* PushTarget = nullptr;
    for (IBoardCombatTarget* CombatTarget : Context.mResolvedCombatTargets)
    {
        UUnitModel* Candidate = Cast<UUnitModel>(CombatTarget);
        if (Candidate != nullptr && Candidate != mInstigator.Get() && Candidate->IsTargetable())
        {
            PushTarget = Candidate;
            break;
        }
    }

    if (PushTarget == nullptr)
    {
        return false;
    }

    UTileMapModel* TileMap = GetTileMap();
    if (TileMap == nullptr)
    {
        return false;
    }

    const FTileIndex PusherTile = mInstigator->GetTileTransform().mIndex;
    const FTileIndex PushedTile = PushTarget->GetTileTransform().mIndex;
    const int32 PushDistance = FMath::Max(Context.mDiceSum, 1);
    const FTileIndex Destination = TileMap->GetPushDestination(PusherTile, PushedTile, PushDistance);
    if (Destination == PushedTile)
    {
        return false;
    }

    const FTileIndex Step(
        FMath::Sign(Destination.mX - PushedTile.mX),
        FMath::Sign(Destination.mY - PushedTile.mY));

    mDicePushTarget = PushTarget;
    mDicePushDiceValue = Context.mDiceSum;
    mDicePushWasReported = false;
	mDicePushStarted = true;
	mDicePushFinished = false;
    mDicePushPath.Reset();
    mDicePushPath.Add(PushedTile);

    FTileIndex Current = PushedTile;
    while (Current != Destination)
    {
        Current = FTileIndex(Current.mX + Step.mX, Current.mY + Step.mY);
        mDicePushPath.Add(Current);
    }

    TArray<FVector> PathWorldLocations;
    PathWorldLocations.Reserve(mDicePushPath.Num());
    for (const FTileIndex& TileIndex : mDicePushPath)
    {
        PathWorldLocations.Add(TileMap->TileToWorldLocation(TileIndex));
    }
    mDicePushTarget->OnStartForcedMovePath.Broadcast(PathWorldLocations);

    StartDicePushStep(1);
    return true;
}

void USRPGSkillAction::StartDicePushStep(int32 StepIndex)
{
    if (mDicePushTarget == nullptr || mDicePushPath.IsValidIndex(StepIndex) == false)
    {
        ReportDicePushIfMoved();
		mDicePushFinished = true;
        FinishSkillAction();
        return;
    }

    UTileMapModel* TileMap = GetTileMap();
    bool bBlocked = false;
    if (TileMap != nullptr)
    {
        for (UBoardActorModel* Actor : TileMap->GetActorsOnTile(mDicePushPath[StepIndex], ETileLayerFlag::All))
        {
            if (Actor != nullptr
                && Actor != mDicePushTarget
                && EnumHasAnyFlags(Actor->GetBlockLayerFlags(), mDicePushTarget->GetTileLayerFlags()))
            {
                bBlocked = true;
                break;
            }
        }
    }
    if (TileMap == nullptr
        || mDicePushTarget->GetTileTransform().mIndex != mDicePushPath[StepIndex - 1]
        || TileMap->CanPlace(mDicePushPath[StepIndex], mDicePushTarget) == false
        || bBlocked)
    {
        ReportDicePushIfMoved();
		mDicePushFinished = true;
        FinishSkillAction();
        return;
    }

    mDicePushStepIndex = StepIndex;
    // 강제 이동은 보행이 아니므로 이동 방향으로 몸을 돌리지 않고 기존 facing을 유지한다.
    const FTileTransform NextTransform(
        mDicePushPath[StepIndex],
        mDicePushTarget->GetTileTransform().mDirection);
    TileMap->StartActorMovement(NextTransform, mDicePushTarget);

    TSharedPtr<FPresentationBarrier> Barrier = FPresentationBarrier::Make(
        FOnFinishPresentation::CreateWeakLambda(this, [this]() {
            OnDicePushStepFinished();
            }));

    float RemainingPathDistance = 0.0f;
    for (int32 Index = StepIndex; Index < mDicePushPath.Num() - 1; ++Index)
    {
        RemainingPathDistance += FVector::Dist(
            TileMap->TileToWorldLocation(mDicePushPath[Index]),
            TileMap->TileToWorldLocation(mDicePushPath[Index + 1]));
    }

    mDicePushTarget->OnStartMoveStep.Broadcast(
        NextTransform,
        TileMap->TileToWorldTransform(NextTransform),
        Barrier,
        RemainingPathDistance);
}

void USRPGSkillAction::OnDicePushStepFinished()
{
    if (mActionPhase != ESRPGActionPhase::ActionPlay || mDicePushTarget == nullptr)
    {
        return;
    }

    if (UTileMapModel* TileMap = GetTileMap())
    {
        TileMap->CompleteActorMovement(mDicePushTarget);
    }

    if (mDicePushStepIndex >= mDicePushPath.Num() - 1)
    {
        ReportDicePushIfMoved();
		mDicePushFinished = true;
        FinishSkillAction();
    }
    else
    {
        StartDicePushStep(mDicePushStepIndex + 1);
    }
}

void USRPGSkillAction::ReportDicePushIfMoved()
{
    if (mDicePushWasReported || mDicePushTarget == nullptr || mDicePushPath.IsEmpty())
    {
        return;
    }

    const FTileIndex CurrentTile = mDicePushTarget->GetTileTransform().mIndex;
    if (CurrentTile == mDicePushPath[0])
    {
        return;
    }

    if (USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
    {
        CombatModel->ReportPlayerDisplacement(
            mDicePushTarget,
            mDicePushPath[0],
            CurrentTile,
            mDicePushDiceValue);
        mDicePushWasReported = true;
    }
}

void USRPGSkillAction::FinishSkillAction()
{
	if (mSkillPresentationFinished == false
		|| (mDicePushStarted && mDicePushFinished == false))
	{
		return;
	}
    MarkActionCompleted(ESRPGActionResult::Succeeded);
}

