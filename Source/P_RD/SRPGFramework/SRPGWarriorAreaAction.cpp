#include "SRPGFramework/SRPGWarriorAreaAction.h"

#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/TileMap/TileMapModel.h"
#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Pawn/UnitModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"

FSRPGWarriorAreaCommand::FSRPGWarriorAreaCommand()
{
	mCommandType = ESRPGCommandType::WarriorAreaCast;
	mRequestedAction = USRPGWarriorAreaAction::StaticClass();
}

USRPGWarriorAreaAction::USRPGWarriorAreaAction()
{
	mActionType = ESRPGActionType::InPlayAction;
	mConsumesTurn = true;
}

ESRPGCommandResult USRPGWarriorAreaAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
	const ESRPGCommandResult ParentResult = Super::HandleCommand(Command);
	if (ParentResult == ESRPGCommandResult::Handled)
	{
		return ParentResult;
	}
	if (Command.Get().GetCommandType() != ESRPGCommandType::WarriorAreaCast)
	{
		return ESRPGCommandResult::Ignored;
	}
	const FSRPGWarriorAreaCommand& AreaCommand = Command.Get<FSRPGWarriorAreaCommand>();
	mAreaActionType = AreaCommand.mAreaActionType;
	mRadius = FMath::Max(AreaCommand.mRadius, 1);
	mDamage = FMath::Max(AreaCommand.mDamage, 0);
	return CombineSRPGCommandResult(ESRPGCommandResult::Handled, ParentResult);
}

void USRPGWarriorAreaAction::OnBeginAction()
{
	Super::OnBeginAction();
	USRPGTurnContext* TurnContext = GetParent().Get();
	USRPGCombatModel* CombatModel = TurnContext != nullptr ? TurnContext->GetParent() : nullptr;
	if (CombatModel == nullptr || mInstigator == nullptr || CombatModel->GetTileMap() == nullptr)
	{
		MarkActionCompleted(ESRPGActionResult::Cancelled);
		return;
	}
	if (TryStartAreaApproach() == false)
	{
		BeginAreaImpact();
	}
}

bool USRPGWarriorAreaAction::TryStartAreaApproach()
{
	USRPGTurnContext* TurnContext = GetParent().Get();
	USRPGCombatModel* CombatModel = TurnContext != nullptr ? TurnContext->GetParent() : nullptr;
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	if (CombatModel == nullptr || TileMap == nullptr || mInstigator == nullptr)
	{
		return false;
	}

	const FTileIndex Origin = mInstigator->GetTileTransform().mIndex;
	const int32 ApproachRange = mAreaActionType == ESRPGWarriorAreaActionType::Shockwave ? 3 : 2;
	FTileIndex BestTile = FTileIndex::Invalid;
	int32 BestScore = 0;
	for (int32 DeltaY = -ApproachRange; DeltaY <= ApproachRange; ++DeltaY)
	{
		for (int32 DeltaX = -ApproachRange; DeltaX <= ApproachRange; ++DeltaX)
		{
			const FTileIndex Candidate(Origin.mX + DeltaX, Origin.mY + DeltaY);
			if (Candidate == Origin || TileMap->IsValidIndex(Candidate) == false
				|| TileMap->CanPlace(Candidate, mInstigator.Get()) == false)
			{
				continue;
			}
			int32 Score = 0;
			for (const TObjectPtr<UUnitModel>& Unit : CombatModel->GetUnits())
			{
				const UEnemyUnitModel* Enemy = Cast<UEnemyUnitModel>(Unit);
				if (Enemy == nullptr || Enemy->IsDead()
					|| Enemy->GetTeamAttitudeTowards(*mInstigator) != ETeamAttitude::Hostile)
				{
					continue;
				}
				const FTileIndex EnemyTile = Enemy->GetTileTransform().mIndex;
				if (FMath::Max(FMath::Abs(EnemyTile.mX - Candidate.mX), FMath::Abs(EnemyTile.mY - Candidate.mY)) <= mRadius)
				{
					++Score;
				}
			}
			if (Score > BestScore)
			{
				BestScore = Score;
				BestTile = Candidate;
			}
		}
	}
	if (BestTile == FTileIndex::Invalid)
	{
		return false;
	}

	mApproachFrom = Origin;
	mApproachTo = BestTile;
	const FTileTransform DestinationTransform(
		BestTile,
		UTileMapModel::TileDeltaToDirection(Origin, BestTile, mInstigator->GetTileTransform().mDirection));
	TileMap->StartActorMovement(DestinationTransform, mInstigator.Get());
	mInstigator->OnStartForcedMovePath.Broadcast(
		{TileMap->TileToWorldLocation(Origin), TileMap->TileToWorldLocation(BestTile)},
		mAreaActionType == ESRPGWarriorAreaActionType::Shockwave
			? EForcedMovePresentationType::Leap
			: EForcedMovePresentationType::BlinkStrike);
	TSharedPtr<FPresentationBarrier> Barrier = FPresentationBarrier::Make(
		FOnFinishPresentation::CreateWeakLambda(this, [this]() { OnAreaApproachFinished(); }));
	mInstigator->OnStartMoveStep.Broadcast(
		DestinationTransform,
		TileMap->TileToWorldTransform(DestinationTransform),
		Barrier,
		0.0f);
	return true;
}

void USRPGWarriorAreaAction::OnAreaApproachFinished()
{
	USRPGTurnContext* TurnContext = GetParent().Get();
	USRPGCombatModel* CombatModel = TurnContext != nullptr ? TurnContext->GetParent() : nullptr;
	if (CombatModel != nullptr && CombatModel->GetTileMap() != nullptr && mInstigator != nullptr)
	{
		CombatModel->GetTileMap()->CompleteActorMovement(mInstigator.Get());
		CombatModel->NotifyWarriorSkillMovement(
			mApproachFrom,
			mApproachTo,
			mAreaActionType == ESRPGWarriorAreaActionType::Shockwave
				? NSLOCTEXT("WarriorArea", "ShockLanding", "충격 착지")
				: NSLOCTEXT("WarriorArea", "WhirlwindEntry", "회전 돌입"));
	}
	BeginAreaImpact();
}

void USRPGWarriorAreaAction::BeginAreaImpact()
{
	USRPGTurnContext* TurnContext = GetParent().Get();
	USRPGCombatModel* CombatModel = TurnContext != nullptr ? TurnContext->GetParent() : nullptr;
	if (CombatModel == nullptr || mInstigator == nullptr || CombatModel->GetTileMap() == nullptr)
	{
		MarkActionCompleted(ESRPGActionResult::Cancelled);
		return;
	}

	const FTileIndex Origin = mInstigator->GetTileTransform().mIndex;
	TArray<TObjectPtr<UEnemyUnitModel>> Targets;
	for (const TObjectPtr<UUnitModel>& Unit : CombatModel->GetUnits())
	{
		UEnemyUnitModel* Enemy = Cast<UEnemyUnitModel>(Unit);
		if (Enemy == nullptr || Enemy->IsDead()
			|| Enemy->GetTeamAttitudeTowards(*mInstigator) != ETeamAttitude::Hostile)
		{
			continue;
		}
		const FTileIndex Tile = Enemy->GetTileTransform().mIndex;
		const int32 Distance = FMath::Max(FMath::Abs(Tile.mX - Origin.mX), FMath::Abs(Tile.mY - Origin.mY));
		if (Distance <= mRadius)
		{
			Targets.Add(Enemy);
		}
	}

	if (Targets.IsEmpty())
	{
		MarkActionCompleted(ESRPGActionResult::Cancelled);
		return;
	}
	mResolvedAnyTarget = true;
	for (UEnemyUnitModel* Target : Targets)
	{
		ApplyHit(Target);
	}
	if (mAreaActionType == ESRPGWarriorAreaActionType::Shockwave)
	{
		mSchedulingPushes = true;
		for (UEnemyUnitModel* Target : Targets)
		{
			TryPushTarget(Target);
		}
		mSchedulingPushes = false;
	}
	FinishIfReady();
}

void USRPGWarriorAreaAction::ApplyHit(UEnemyUnitModel* Target)
{
	if (Target == nullptr)
	{
		return;
	}
	if (UAttributeSetComponentModel* Attributes = Target->GetAttributeComponentModel())
	{
		Attributes->ApplyModToAttribute(
			UCombatTargetAttributeSet::GetHPAttribute(), ETacticalModOp::Additive, -StaticCast<float>(mDamage));
	}
	USRPGTurnContext* TurnContext = GetParent().Get();
	USRPGCombatModel* CombatModel = TurnContext != nullptr ? TurnContext->GetParent() : nullptr;
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	if (TileMap != nullptr)
	{
		FVector Direction = TileMap->TileToWorldLocation(Target->GetTileTransform().mIndex)
			- TileMap->TileToWorldLocation(mInstigator->GetTileTransform().mIndex);
		Direction.Z = 0.0f;
		Direction = Direction.GetSafeNormal();
		mInstigator->OnPlayImpactPresentation.Broadcast(Direction, 0.9f, EImpactPresentationType::Source);
		Target->OnPlayImpactPresentation.Broadcast(Direction, 1.05f, EImpactPresentationType::Receiver);
	}
}

void USRPGWarriorAreaAction::TryPushTarget(UEnemyUnitModel* Target)
{
	USRPGTurnContext* TurnContext = GetParent().Get();
	USRPGCombatModel* CombatModel = TurnContext != nullptr ? TurnContext->GetParent() : nullptr;
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	if (Target == nullptr || TileMap == nullptr)
	{
		return;
	}
	const FTileIndex Origin = mInstigator->GetTileTransform().mIndex;
	const FTileIndex From = Target->GetTileTransform().mIndex;
	const int32 DeltaX = FMath::Clamp(From.mX - Origin.mX, -1, 1);
	const int32 DeltaY = FMath::Clamp(From.mY - Origin.mY, -1, 1);
	const FTileIndex Destination(From.mX + DeltaX, From.mY + DeltaY);
	if ((DeltaX == 0 && DeltaY == 0) || TileMap->IsValidIndex(Destination) == false
		|| TileMap->CanPlace(Destination, Target) == false)
	{
		if (UBoardActorModel* Blocker = FindBlockingActor(Destination, Target))
		{
			CombatModel->ReportPlayerDisplacementCollision(Target, Blocker, ESRPGPlayerDisplacementType::Push);
		}
		return;
	}

	const FTileTransform DestinationTransform(
		Destination,
		UTileMapModel::TileDeltaToDirection(From, Destination, Target->GetTileTransform().mDirection));
	TileMap->StartActorMovement(DestinationTransform, Target);
	CombatModel->ReportPlayerDisplacement(Target, From, Destination, 4, ESRPGPlayerDisplacementType::Push);
	Target->OnStartForcedMovePath.Broadcast(
		{TileMap->TileToWorldLocation(From), TileMap->TileToWorldLocation(Destination)},
		EForcedMovePresentationType::Push);
	++mPendingPushPresentations;
	TSharedPtr<FPresentationBarrier> Barrier = FPresentationBarrier::Make(
		FOnFinishPresentation::CreateWeakLambda(this, [this, WeakTarget = TWeakObjectPtr<UEnemyUnitModel>(Target)]()
		{
			OnPushPresentationFinished(WeakTarget.Get());
		}));
	Target->OnStartMoveStep.Broadcast(
		DestinationTransform,
		TileMap->TileToWorldTransform(DestinationTransform),
		Barrier,
		0.0f);
}

void USRPGWarriorAreaAction::OnPushPresentationFinished(UEnemyUnitModel* Target)
{
	USRPGTurnContext* TurnContext = GetParent().Get();
	USRPGCombatModel* CombatModel = TurnContext != nullptr ? TurnContext->GetParent() : nullptr;
	if (Target != nullptr && CombatModel != nullptr && CombatModel->GetTileMap() != nullptr)
	{
		CombatModel->GetTileMap()->CompleteActorMovement(Target);
	}
	mPendingPushPresentations = FMath::Max(mPendingPushPresentations - 1, 0);
	FinishIfReady();
}

void USRPGWarriorAreaAction::FinishIfReady()
{
	if (mPendingPushPresentations == 0 && mSchedulingPushes == false)
	{
		MarkActionCompleted(mResolvedAnyTarget ? ESRPGActionResult::Succeeded : ESRPGActionResult::Cancelled);
	}
}

UBoardActorModel* USRPGWarriorAreaAction::FindBlockingActor(
	const FTileIndex& TileIndex,
	const UBoardActorModel* MovingActor) const
{
	USRPGTurnContext* TurnContext = GetParent().Get();
	USRPGCombatModel* CombatModel = TurnContext != nullptr ? TurnContext->GetParent() : nullptr;
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	if (TileMap == nullptr || TileMap->IsValidIndex(TileIndex) == false)
	{
		return nullptr;
	}
	for (UBoardActorModel* Actor : TileMap->GetActorsOnTile(TileIndex, ETileLayerFlag::All))
	{
		if (Actor != nullptr && Actor != MovingActor
			&& EnumHasAnyFlags(Actor->GetBlockLayerFlags(), MovingActor->GetTileLayerFlags()))
		{
			return Actor;
		}
	}
	return nullptr;
}
