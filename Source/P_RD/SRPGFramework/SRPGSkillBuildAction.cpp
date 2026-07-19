#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "SRPGFramework/SRPGSkillAction.h"

#include "RDCollision.h"

#include "Component/SkillComponent/SkillComponentModel.h"
#include "Dice/DicePoolModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "Actor/ActorView.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Pawn/UnitModel.h"
#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "Actor/TileMap/TileMapModel.h"

#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

namespace
{
	const FName DicePushBuildSkillAssetName(TEXT("DA_SwordNormalSmash_Common"));
	const FName DicePullBuildSkillAssetName(TEXT("DA_SwordBlade_Rare"));

	bool IsDiceDisplacementBuildSkill(const UStaticSkillData* SkillData)
	{
		return SkillData != nullptr
			&& (SkillData->GetFName() == DicePushBuildSkillAssetName
				|| SkillData->GetFName() == DicePullBuildSkillAssetName);
	}

	bool IsDicePullBuildSkill(const UStaticSkillData* SkillData)
	{
		return SkillData != nullptr && SkillData->GetFName() == DicePullBuildSkillAssetName;
	}

	UUnitModel* FindDisplacementTarget(
		UTileMapModel* TileMap,
		const FTileIndex& TargetIndex,
		const UUnitModel* Instigator)
	{
		if (TileMap == nullptr)
		{
			return nullptr;
		}

		for (UBoardActorModel* Actor : TileMap->GetActorsOnTile(TargetIndex, ETileLayerFlag::Unit))
		{
			UUnitModel* Unit = Cast<UUnitModel>(Actor);
			if (Unit != nullptr && Unit != Instigator && Unit->IsTargetable())
			{
				return Unit;
			}
		}
		return nullptr;
	}

	int32 GetPullThrowDistance(int32 DiceValue, const UUnitModel* TargetUnit)
	{
		int32 WeightValue = StaticCast<int32>(ESRPGDisplacementWeight::Medium);
		if (const UEnemyUnitModel* EnemyTarget = Cast<UEnemyUnitModel>(TargetUnit))
		{
			WeightValue = StaticCast<int32>(EnemyTarget->GetDisplacementWeight());
		}
		return FMath::Clamp(FMath::Max(DiceValue, 1) + 1 - WeightValue, 1, 4);
	}

	bool HasBlockingOccupantExcept(
		UTileMapModel* TileMap,
		const FTileIndex& TileIndex,
		const UBoardActorModel* IgnoredActor)
	{
		if (TileMap == nullptr)
		{
			return false;
		}
		for (const UBoardActorModel* Actor : TileMap->GetActorsOnTile(
			TileIndex,
			ETileLayerFlag::Obstacle | ETileLayerFlag::Unit))
		{
			if (Actor != nullptr && Actor != IgnoredActor)
			{
				return true;
			}
		}
		return false;
	}

	TArray<FTileIndex> BuildPullDestinationIndexes(
		UTileMapModel* TileMap,
		const UUnitModel* Instigator,
		UUnitModel* TargetUnit,
		int32 DiceValue)
	{
		TArray<FTileIndex> Result;
		if (TileMap == nullptr || Instigator == nullptr || TargetUnit == nullptr)
		{
			return Result;
		}

		const FTileIndex PlayerTile = Instigator->GetTileTransform().mIndex;
		const FTileIndex TargetTile = TargetUnit->GetTileTransform().mIndex;
		const TArray<FTileIndex> PullPath = TileMap->GetPullPath(PlayerTile, TargetTile, 64);
		if (PullPath.IsEmpty())
		{
			return Result;
		}

		// 발앞 칸은 "당기기만" 하는 선택지다. 나머지는 플레이어 중심 8방향의 직접 투척 후보다.
		const FTileIndex PullEnd = PullPath.Last();
		Result.Add(PullEnd);
		const bool bReachedPlayer = FMath::Max(
			FMath::Abs(PullEnd.mX - PlayerTile.mX),
			FMath::Abs(PullEnd.mY - PlayerTile.mY)) == 1;
		if (bReachedPlayer == false)
		{
			// 장애물에 걸려 발앞까지 오지 못하면 던질 수 없고, 막힌 지점까지 당기는 선택만 남는다.
			return Result;
		}
		const int32 ThrowDistance = GetPullThrowDistance(DiceValue, TargetUnit);
		for (int32 StepX = -1; StepX <= 1; ++StepX)
		{
			for (int32 StepY = -1; StepY <= 1; ++StepY)
			{
				if (StepX == 0 && StepY == 0)
				{
					continue;
				}

				FTileIndex DirectionDestination = FTileIndex::Invalid;
				for (int32 Distance = 1; Distance <= ThrowDistance; ++Distance)
				{
					const FTileIndex Candidate(
						PlayerTile.mX + StepX * Distance,
						PlayerTile.mY + StepY * Distance);
					if (TileMap->IsValidIndex(Candidate) == false)
					{
						break;
					}
					if (Candidate == PullEnd)
					{
						continue;
					}

					// 선택 당시 대상의 원래 칸은 당기면서 비워지므로 장애물로 보지 않는다.
					const bool bBlocked = Candidate == TargetTile
						? HasBlockingOccupantExcept(TileMap, Candidate, TargetUnit)
						: TileMap->CanPlace(Candidate, TargetUnit) == false;
					DirectionDestination = Candidate;
					if (bBlocked)
					{
						// 첫 막힌 칸 자체가 이 방향의 충돌 목표다.
						break;
					}
				}
				if (DirectionDestination != FTileIndex::Invalid)
				{
					// 중간 칸 전체가 아니라 방향별 최종 칸 하나만 보여 선택지를 8개 이하로 유지한다.
					Result.AddUnique(DirectionDestination);
				}
			}
		}
		return Result;
	}
}

FSRPGSkillSelectCommand::FSRPGSkillSelectCommand()
{
    mCommandType = ESRPGCommandType::SkillSelect;
    mRequestedAction = USRPGSkillBuildAction::StaticClass();
}

FSRPGDiceSelectCommand::FSRPGDiceSelectCommand()
{
    mCommandType = ESRPGCommandType::DiceSelect;
}

USRPGSkillBuildAction::USRPGSkillBuildAction()
{
    mActionType = ESRPGActionType::BuildAction;
    mConsumesTurn = false;
}

void USRPGSkillBuildAction::OnBeginAction()
{
    Super::OnBeginAction();
}

void USRPGSkillBuildAction::OnEndAction()
{
    ClearAllTileHighlights();
    ResetTargetTile();
    ResetDice();
    ResetSkill();

    Super::OnEndAction();
}

ESRPGCommandResult USRPGSkillBuildAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = Super::HandleCommand(Command);
    if (Result == ESRPGCommandResult::Handled)
    {
        return Result;
    }

    switch (Command.Get().GetCommandType())
    {
    case ESRPGCommandType::SkillSelect:
    {
        /* 새롭게 스킬 선택 시 제거 */

        const FSRPGSkillSelectCommand& SkillSelectCommand = Command.Get<FSRPGSkillSelectCommand>();

        OnSelectSkill = SkillSelectCommand.OnSelectSkill;
        OnChangeSkillBuildPhase = SkillSelectCommand.OnChangeSkillBuildPhase;
        OnPostSimulateSkillAction = SkillSelectCommand.OnPostSimulateSkillAction;
        OnCancelSimulateSkillAction = SkillSelectCommand.OnCancelSimulateSkillAction;
        if (SkillSelectCommand.mSkillIndex != mSelectedSkillIndex)
        {
            /* 다르면 변경 */

            // 이전 스킬의 조준/효과 하이라이트를 먼저 지운다. ResetTargetTile은 데이터(mEffectTileIndexes)만
            // 비우고 화면 하이라이트는 안 지워서, 안 지우면 이전 스킬의 효과 범위가 남아 "취소가 안 된 것"처럼 보인다.
            ClearAllTileHighlights();
            ResetTargetTile();
            ResetDice();
            ResetSkill();
            /*
             * 조준 중 다른 스킬을 고르면 처음부터 다시 시작한다.
             * 페이즈도 None으로 돌려야 다음 SetSkill이 안전하다.
             */
            SetBuildPhase(ESRPGSkillBuildPhase::None);
            SetSkill(SkillSelectCommand.mSkillIndex);
            RefreshAimableTileHighlights();
            SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
        }
        else
        {
            /* 같으면 취소 */

            MarkActionCompleted(ESRPGActionResult::Cancelled);
            SetBuildPhase(ESRPGSkillBuildPhase::None);
        }
        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    case ESRPGCommandType::MoveSelect:
    case ESRPGCommandType::TurnEnd:
    {
        /* 다른 동작 요구 시 취소 */

        ClearAllTileHighlights();
        ResetTargetTile();
        ResetDice();
        ResetSkill();
        MarkActionCompleted(ESRPGActionResult::Cancelled);
        SetBuildPhase(ESRPGSkillBuildPhase::None);
        return ESRPGCommandResult::Ignored;
    }
    case ESRPGCommandType::DiceSelect:
    {
        /* 주사위 변경 시 타겟부터 재설정 */

        const FSRPGDiceSelectCommand& DiceSelectCommand = Command.Get<FSRPGDiceSelectCommand>();

        /*
         * 주사위 변경은 스킬을 고른 뒤(조준/프리뷰 단계)에만 의미가 있다.
         * - 스킬 미선택(None)/시전 완료(Build) 상태의 주사위 클릭은 무시한다(억지로 진행하면
         *   선택 스킬이 없어 ChangeDices에서 nullptr 참조로 죽는다).
         * - 프리뷰에서 주사위를 바꾸면 주사위 합이 달라져 조준이 무효가 되므로, ChangeDices 전에
         *   조준 단계로 되돌린다. ChangeDices는 AimSelection 전제(checkf)라 안 되돌리면 어설션 크래시.
         *   (스킬 조준 -> 주사위 재선택 크래시 수정)
         */
        const bool IsSkillSelectedForDice = mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection
            || mSkillBuildPhase == ESRPGSkillBuildPhase::ThrowDestinationSelection
            || mSkillBuildPhase == ESRPGSkillBuildPhase::Preview;
        if (DiceSelectCommand.mDiceIndex != INDEX_NONE && IsSkillSelectedForDice == true)
        {
            ResetTargetTile();
            SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
            ChangeDices(DiceSelectCommand.mDiceIndex);
            RefreshAimableTileHighlights();
        }
        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    case ESRPGCommandType::WorldTrace:
    {
        /* 월드 공간 터치 시 선택 위치에 따라서 결정 */

        return CombineSRPGCommandResult(HandleWorldTraceCommand(Command), Result);
    }
    }

    return ESRPGCommandResult::Ignored;
}

ESRPGCommandResult USRPGSkillBuildAction::HandleWorldTraceCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = ESRPGCommandResult::Ignored;

    const FSRPGWorldTraceCommand& WorldTraceCommand = Command.Get<FSRPGWorldTraceCommand>();
    if (WorldTraceCommand.mIsLongPress == true)
    {
        return Result;
    }

    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    AActor* TargetActor = nullptr;
    FTileIndex TargetTileIndex = FTileIndex::Invalid;
    GetTileActorUnderCursor(GetWorld(), RDTraceChannels::TileOnlyTrace, WorldTraceCommand.mScreenPosition, OUT TargetActor, OUT TargetTileIndex);

    IActorView* ActorView = Cast<IActorView>(TargetActor);
    const bool IsContactedTileMap = ActorView != nullptr && ActorView->GetModel() == TileMap;
    const bool IsContactedBoard = IsContactedTileMap == true || TargetTileIndex != FTileIndex::Invalid;
    // 보드를 아예 벗어난 탭(IsContactedBoard=false)도 무효 타일과 똑같이 "타일 밖" 취소로 취급한다
    // (기획: 스킬 조준 중 타일 밖을 누르면 취소). TargetTileIndex가 유효하면 정상 처리로 간다.
    if (IsContactedBoard == true || TargetTileIndex == FTileIndex::Invalid)
    {
        if (TargetTileIndex == FTileIndex::Invalid)
        {
            /* 한단계 취소작업 (무효 타일 또는 보드 밖 탭) */

            switch (mSkillBuildPhase)
            {
            case ESRPGSkillBuildPhase::Preview:
            {
                /* 투척 프리뷰 취소는 적 선택을 유지하고 착지 선택으로 한 단계만 돌아간다. */
				if (IsDicePullBuildSkill(mSelectedSkill) && mTargetIndex != FTileIndex::Invalid)
				{
					mDisplacementDestination = FTileIndex::Invalid;
					SetBuildPhase(ESRPGSkillBuildPhase::ThrowDestinationSelection);
					RefreshPullDestinationHighlights();
				}
				else
				{
					ResetTargetTile();
					SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
					RefreshAimableTileHighlights();
				}

                Result = ESRPGCommandResult::Handled;
                break;
            }
			case ESRPGSkillBuildPhase::ThrowDestinationSelection:
			{
				/* 착지 선택 취소는 고정한 적을 풀고 처음의 적 선택으로 돌아간다. */
				ResetTargetTile();
				SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
				RefreshAimableTileHighlights();
				Result = ESRPGCommandResult::Handled;
				break;
			}
            case ESRPGSkillBuildPhase::AimSelection:
            {
                /* 조준 대상 설정 단계에서 한단계 취소 시, 빌드 자체 종료 */

                MarkActionCompleted(ESRPGActionResult::Cancelled);
                SetBuildPhase(ESRPGSkillBuildPhase::None);

                Result = ESRPGCommandResult::Handled;
                break;
            }
            }
        }
        else
        {
            /* 한단계 처리작업 */

            switch (mSkillBuildPhase)
            {
            case ESRPGSkillBuildPhase::Preview:
            {
                /* 프리뷰에서 같은 확정 칸을 다시 클릭하면 스킬 캐스팅 */

				const FTileIndex ConfirmationIndex = IsDicePullBuildSkill(mSelectedSkill)
					? mDisplacementDestination
					: mTargetIndex;
                if (ConfirmationIndex == TargetTileIndex)
                {
                    BuildSkill();
                    SetBuildPhase(ESRPGSkillBuildPhase::Build);
                    MarkActionCompleted(ESRPGActionResult::Succeeded);

                    Result = ESRPGCommandResult::Handled;
                    break;
                }

				// 끌어당기기는 적을 다시 고르지 않고 다른 착지 후보를 곧바로 바꿀 수 있다.
				if (IsDicePullBuildSkill(mSelectedSkill)
					&& CanSelectPullDestinationTile(TargetTileIndex))
				{
					SetBuildPhase(ESRPGSkillBuildPhase::ThrowDestinationSelection);
					SetPullDestinationTile(TargetTileIndex);
					RefreshEffectTileHighlights();
					SetBuildPhase(ESRPGSkillBuildPhase::Preview);
					Result = ESRPGCommandResult::Handled;
					break;
				}
				[[fallthrough]];
            }
			case ESRPGSkillBuildPhase::ThrowDestinationSelection:
			{
				if (CanSelectPullDestinationTile(TargetTileIndex))
				{
					SetPullDestinationTile(TargetTileIndex);
					RefreshEffectTileHighlights();
					SetBuildPhase(ESRPGSkillBuildPhase::Preview);
					Result = ESRPGCommandResult::Handled;
					break;
				}
				if (mSkillBuildPhase == ESRPGSkillBuildPhase::ThrowDestinationSelection)
				{
					break;
				}
				[[fallthrough]];
			}
            case ESRPGSkillBuildPhase::AimSelection:
            {
                /* 끌어당기기는 적을 먼저 고정하고, 다음 입력에서 착지 방향을 고른다. */

                if (CanSelectTargetTile(TargetTileIndex) == true)
                {
                    ResetTargetTile();
                    SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
					if (IsDicePullBuildSkill(mSelectedSkill))
					{
						LockPullTarget(TargetTileIndex);
						if (mPullDestinationIndexes.IsEmpty())
						{
							ResetTargetTile();
							RefreshAimableTileHighlights();
							break;
						}
						RefreshPullDestinationHighlights();
						SetBuildPhase(ESRPGSkillBuildPhase::ThrowDestinationSelection);
					}
					else
					{
						SetTargetTile(TargetTileIndex);
						RefreshEffectTileHighlights();
						SetBuildPhase(ESRPGSkillBuildPhase::Preview);
					}

                    Result = ESRPGCommandResult::Handled;
                    break;
                }
                break;
            }
            }
        }
    }
    return Result;
}

void USRPGSkillBuildAction::SetSkill(int32 SkillIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::None, TEXT("스킬 빌드 순서 오류"));

    USkillComponentModel* SkillCompModel = mInstigator->GetSkillComponentModel();
    checkf(SkillCompModel != nullptr, TEXT("스킬 컴포넌트 모델 nullptr"));

    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"))

    /* 스킬 등록 */

    {
        TSoftObjectPtr<UStaticSkillData> StaticSkillDataSoftObj = nullptr;
        StaticSkillDataSoftObj = SkillCompModel->GetSkill(SkillIndex)->mData;
        if (StaticSkillDataSoftObj == nullptr)
        {
            UE_LOG(LogSRPGCombat, Warning, TEXT("스킬 시전 시 비정상적 스킬 선택"));
            return;
        }

        mSelectedSkillIndex = SkillIndex;
        mSelectedSkill = StaticSkillDataSoftObj.Get();
    }

    OnSelectSkill.Broadcast(mSelectedSkillIndex);
}

void USRPGSkillBuildAction::ChangeDices(int32 RequestedDiceIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection, TEXT("스킬 빌드 순서 오류"));

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    if (DicePoolModel->IsSelectedDice(RequestedDiceIndex) == true)
    {
        // 이전 주사위 제거
        DicePoolModel->MarkDiceUnselected(RequestedDiceIndex);
    }
    else if (DicePoolModel->GetSelectedDiceNum() < mSelectedSkill->mRequiredDiceCount)
    {
        // 새로운 주사위 추가 할당
        DicePoolModel->MarkDiceSelected(RequestedDiceIndex);
    }
}

void USRPGSkillBuildAction::SetTargetTile(const FTileIndex& TargetIndex)
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection, TEXT("스킬 빌드 순서 오류"));

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    mTargetIndex = TargetIndex;

    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    TArray<TObjectPtr<UBoardActorModel>> AllEffectActors;
    for (const FTileIndex& EffectTileIndex : mEffectTileIndexes)
    {
        TArray<UBoardActorModel*> EffectActors = TileMap->GetActorsOnTile(EffectTileIndex);
        AllEffectActors.Append(EffectActors);
    }

    USimulationSubsystem* SimulationSubsystem = GetWorld()->GetSubsystem<USimulationSubsystem>();
    checkf(SimulationSubsystem != nullptr, TEXT("시뮬레이션 서브시스템 모델 nullptr"));

    TInstancedStruct<FSRPGCommand> SkillCastCommand;
    SkillCastCommand.InitializeAs<FSRPGSkillCastCommand>();
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mSkillIndex = mSelectedSkillIndex;
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mTargetIndex = mTargetIndex;
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mDisplacementDestination = mDisplacementDestination;
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mDiceSum = DicePoolModel->GetSelectedDiceSum();

    TArray<FSRPGTurnEventLog> TurnEventLogs = SimulationSubsystem->SimulateUntilNextAction(MoveTemp(SkillCastCommand));
    OnPostSimulateSkillAction.Broadcast(TurnEventLogs);
}

void USRPGSkillBuildAction::LockPullTarget(const FTileIndex& TargetIndex)
{
	checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::AimSelection, TEXT("스킬 빌드 순서 오류"));
	UTileMapModel* TileMap = GetTileMap();
	checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));
	UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
	checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));
	UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
	checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

	UUnitModel* TargetUnit = FindDisplacementTarget(TileMap, TargetIndex, mInstigator.Get());
	if (TargetUnit == nullptr)
	{
		return;
	}

	mTargetIndex = TargetIndex;
	mDisplacementDestination = FTileIndex::Invalid;
	mPullDestinationIndexes = BuildPullDestinationIndexes(
		TileMap,
		mInstigator.Get(),
		TargetUnit,
		DicePoolModel->GetSelectedDiceSum());
}

void USRPGSkillBuildAction::SetPullDestinationTile(const FTileIndex& DestinationIndex)
{
	checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::ThrowDestinationSelection, TEXT("스킬 빌드 순서 오류"));
	UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
	checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));
	UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
	checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

	mDisplacementDestination = DestinationIndex;

	USimulationSubsystem* SimulationSubsystem = GetWorld()->GetSubsystem<USimulationSubsystem>();
	checkf(SimulationSubsystem != nullptr, TEXT("시뮬레이션 서브시스템 모델 nullptr"));
	TInstancedStruct<FSRPGCommand> SkillCastCommand;
	SkillCastCommand.InitializeAs<FSRPGSkillCastCommand>();
	SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mSkillIndex = mSelectedSkillIndex;
	SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mTargetIndex = mTargetIndex;
	SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mDisplacementDestination = mDisplacementDestination;
	SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mDiceSum = DicePoolModel->GetSelectedDiceSum();

	TArray<FSRPGTurnEventLog> TurnEventLogs = SimulationSubsystem->SimulateUntilNextAction(MoveTemp(SkillCastCommand));
	OnPostSimulateSkillAction.Broadcast(TurnEventLogs);
}

void USRPGSkillBuildAction::BuildSkill()
{
    checkf(mSkillBuildPhase == ESRPGSkillBuildPhase::Preview, TEXT("스킬 빌드 순서 오류"));

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    // 확정
    DicePoolModel->MarkSelectedDiceAsUsed();

    USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
    checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 서브시스템 모델 nullptr"));

    TInstancedStruct<FSRPGCommand> SkillCastCommand;
    SkillCastCommand.InitializeAs<FSRPGSkillCastCommand>();
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mSkillIndex = mSelectedSkillIndex;
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mTargetIndex = mTargetIndex;
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mDisplacementDestination = mDisplacementDestination;
    SkillCastCommand.GetMutable<FSRPGSkillCastCommand>().mDiceSum = DicePoolModel->GetSelectedDiceSum();

    CommandRouterModel->SummitCommand(SkillCastCommand);
}

void USRPGSkillBuildAction::ResetSkill()
{
    mReachableTileIndexes.Empty();
    mSelectedSkill = nullptr;
    mSelectedSkillIndex = INDEX_NONE;

    OnSelectSkill.Broadcast(mSelectedSkillIndex);
}

void USRPGSkillBuildAction::ResetDice()
{
    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    DicePoolModel->ResetSelected();
}

void USRPGSkillBuildAction::ResetTargetTile()
{
    mEffectTileIndexes.Empty();
    mTargetIndex = FTileIndex::Invalid;
	mDisplacementDestination = FTileIndex::Invalid;
	mPullDestinationIndexes.Empty();
}

void USRPGSkillBuildAction::ClearAllTileHighlights()
{
    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    TileMap->ClearTileHighlight(ETileHighlightFlag::All);
}

void USRPGSkillBuildAction::RefreshAimableTileHighlights()
{
    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));
	ClearAllTileHighlights();

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    USkillComponentModel* SkillCompModel = mInstigator->GetSkillComponentModel();
    checkf(SkillCompModel != nullptr, TEXT("스킬 컴포넌트 모델 nullptr"));

	mReachableTileIndexes = SkillCompModel->GetAimableTiles(TileMap, mSelectedSkillIndex, DicePoolModel->GetSelectedDiceSum());
	if (IsDiceDisplacementBuildSkill(mSelectedSkill))
	{
		if (USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
		{
			// 실제 조준 가능 목록은 전장 전체로 유지하되, 회색 Aim으로 전 타일을 덮지 않는다.
			// 공개된 적 경로/공격선과 ★ 추천 타일을 복원해 어떤 계획을 망가뜨리는지 계속 보이게 한다.
			CombatModel->RefreshEnemyIntentHighlights();
			return;
		}
	}

	TileMap->SetTileHighlight(mReachableTileIndexes, ETileHighlightFlag::Aim);
}

void USRPGSkillBuildAction::RefreshPullDestinationHighlights()
{
	UTileMapModel* TileMap = GetTileMap();
	checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));
	UUnitModel* TargetUnit = FindDisplacementTarget(TileMap, mTargetIndex, mInstigator.Get());
	if (TargetUnit == nullptr)
	{
		return;
	}

	ClearAllTileHighlights();
	const FTileIndex PlayerTile = mInstigator->GetTileTransform().mIndex;
	const TArray<FTileIndex> PullPath = TileMap->GetPullPath(PlayerTile, mTargetIndex, 64);
	// 타일맵의 레이어 의존 순서가 Aim → Select → Effect이므로 넓은 후보부터 구체적인 선택/경로 순으로 칠한다.
	TileMap->SetTileHighlight(mPullDestinationIndexes, ETileHighlightFlag::Aim);
	if (PullPath.IsEmpty() == false)
	{
		// 이 칸은 던지지 않고 발앞에 놓는 "당기기만" 선택지다.
		TileMap->SetTileHighlight(TArray<FTileIndex>({ PullPath.Last() }), ETileHighlightFlag::Select);
		TileMap->SetTileHighlight(PullPath, ETileHighlightFlag::Effect);
	}
}

void USRPGSkillBuildAction::RefreshEffectTileHighlights()
{
    UTileMapModel* TileMap = GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    USkillComponentModel* SkillCompModel = mInstigator->GetSkillComponentModel();
    checkf(SkillCompModel != nullptr, TEXT("스킬 컴포넌트 모델 nullptr"));

    mEffectTileIndexes = SkillCompModel->GetEffectTiles(TileMap, mSelectedSkillIndex, mTargetIndex, DicePoolModel->GetSelectedDiceSum());
    TileMap->SetTileHighlight(mEffectTileIndexes, ETileHighlightFlag::Effect);

	if (IsDiceDisplacementBuildSkill(mSelectedSkill) == false)
	{
		return;
	}

	UUnitModel* TargetUnit = FindDisplacementTarget(TileMap, mTargetIndex, mInstigator.Get());
	if (TargetUnit == nullptr)
	{
		return;
	}

	const FTileIndex InstigatorTile = mInstigator->GetTileTransform().mIndex;
	const FTileIndex TargetTile = TargetUnit->GetTileTransform().mIndex;
	const int32 Distance = FMath::Max(DicePoolModel->GetSelectedDiceSum(), 1);
	const bool bIsPull = mSelectedSkill->GetFName() == DicePullBuildSkillAssetName;
	TArray<FTileIndex> Trajectory = bIsPull
		? TileMap->GetPullPath(InstigatorTile, TargetTile, 64)
		: TArray<FTileIndex>({ TargetTile });
	if (bIsPull == false)
	{
		const FTileIndex Destination = TileMap->GetPushDestination(InstigatorTile, TargetTile, Distance);
		const FTileIndex Step(
			FMath::Sign(Destination.mX - TargetTile.mX),
			FMath::Sign(Destination.mY - TargetTile.mY));
		FTileIndex Current = TargetTile;
		while (Current != Destination && (Step.mX != 0 || Step.mY != 0))
		{
			Current = FTileIndex(Current.mX + Step.mX, Current.mY + Step.mY);
			Trajectory.Add(Current);
		}
	}
	else if (Trajectory.IsEmpty() == false)
	{
		const FTileIndex PullEnd = Trajectory.Last();
		const bool bReachedPlayer = FMath::Max(
			FMath::Abs(PullEnd.mX - InstigatorTile.mX),
			FMath::Abs(PullEnd.mY - InstigatorTile.mY)) == 1;
		if (bReachedPlayer
			&& mDisplacementDestination != FTileIndex::Invalid
			&& mDisplacementDestination != PullEnd)
		{
			const FTileIndex ThrowStep(
				FMath::Sign(mDisplacementDestination.mX - InstigatorTile.mX),
				FMath::Sign(mDisplacementDestination.mY - InstigatorTile.mY));
			const int32 SelectedDistance = FMath::Max(
				FMath::Abs(mDisplacementDestination.mX - InstigatorTile.mX),
				FMath::Abs(mDisplacementDestination.mY - InstigatorTile.mY));
			const int32 ThrowDistance = FMath::Min(SelectedDistance, GetPullThrowDistance(Distance, TargetUnit));
			Trajectory.AddUnique(InstigatorTile);
			for (int32 ThrowIndex = 1; ThrowIndex <= ThrowDistance; ++ThrowIndex)
			{
				const FTileIndex Candidate(
					InstigatorTile.mX + ThrowStep.mX * ThrowIndex,
					InstigatorTile.mY + ThrowStep.mY * ThrowIndex);
				if (TileMap->IsValidIndex(Candidate) == false)
				{
					break;
				}
				if (Candidate == PullEnd)
				{
					continue;
				}
				Trajectory.AddUnique(Candidate);
				const bool bBlocked = Candidate == TargetTile
					? HasBlockingOccupantExcept(TileMap, Candidate, TargetUnit)
					: TileMap->CanPlace(Candidate, TargetUnit) == false;
				if (bBlocked)
				{
					break;
				}
			}
		}
	}
	if (Trajectory.IsEmpty())
	{
		return;
	}
	mEffectTileIndexes = Trajectory;
	const FTileIndex SelectedTile = bIsPull && mDisplacementDestination != FTileIndex::Invalid
		? mDisplacementDestination
		: Trajectory.Last();
	TileMap->SetTileHighlight(TArray<FTileIndex>({ SelectedTile }), ETileHighlightFlag::Select);
	TileMap->SetTileHighlight(Trajectory, ETileHighlightFlag::Effect);
}

bool USRPGSkillBuildAction::CanSelectTargetTile(const FTileIndex& Index) const
{
    UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
    checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));

    UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
    checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트를 들고 있지 않음"));

    const bool bBaseSelectable = mReachableTileIndexes.Contains(Index)
		&& DicePoolModel->GetSelectedDiceNum() == mSelectedSkill->mRequiredDiceCount;
	if (bBaseSelectable == false)
	{
		return false;
	}
	if (IsDicePullBuildSkill(mSelectedSkill))
	{
		return FindDisplacementTarget(GetTileMap(), Index, mInstigator.Get()) != nullptr;
	}
	return true;
}

bool USRPGSkillBuildAction::CanSelectPullDestinationTile(const FTileIndex& Index) const
{
	return IsDicePullBuildSkill(mSelectedSkill)
		&& mTargetIndex != FTileIndex::Invalid
		&& mPullDestinationIndexes.Contains(Index);
}

void USRPGSkillBuildAction::SetBuildPhase(ESRPGSkillBuildPhase BuildPhase)
{
	if (mSkillBuildPhase == BuildPhase)
	{
		return;
	}

    if (BuildPhase != ESRPGSkillBuildPhase::Build && mSkillBuildPhase == ESRPGSkillBuildPhase::Preview)
    {
        OnCancelSimulateSkillAction.Broadcast();
    }

    mSkillBuildPhase = BuildPhase;
    OnChangeSkillBuildPhase.Broadcast(this, BuildPhase);
}

UTileMapModel* USRPGSkillBuildAction::GetTileMap() const
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

