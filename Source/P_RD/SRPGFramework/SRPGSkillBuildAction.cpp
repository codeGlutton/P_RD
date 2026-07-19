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
	const FName DiceStaggerBuildSkillAssetName(TEXT("DA_NomalDefense_Common"));
	const FName DiceSwapBuildSkillAssetName(TEXT("DA_NomalHeal_Common"));

	bool IsDiceDisplacementBuildSkill(const UStaticSkillData* SkillData)
	{
		return SkillData != nullptr
			&& (SkillData->GetFName() == DicePushBuildSkillAssetName
				|| SkillData->GetFName() == DicePullBuildSkillAssetName
				|| SkillData->GetFName() == DiceStaggerBuildSkillAssetName
				|| SkillData->GetFName() == DiceSwapBuildSkillAssetName);
	}

	bool IsDicePullBuildSkill(const UStaticSkillData* SkillData)
	{
		return SkillData != nullptr && SkillData->GetFName() == DicePullBuildSkillAssetName;
	}

	bool IsDiceThrowBuildSkill(const UStaticSkillData* SkillData)
	{
		return SkillData != nullptr && SkillData->GetFName() == DicePushBuildSkillAssetName;
	}

	bool IsDiceStaggerBuildSkill(const UStaticSkillData* SkillData)
	{
		return SkillData != nullptr && SkillData->GetFName() == DiceStaggerBuildSkillAssetName;
	}

	bool IsDiceSwapBuildSkill(const UStaticSkillData* SkillData)
	{
		return SkillData != nullptr && SkillData->GetFName() == DiceSwapBuildSkillAssetName;
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

	UBoardActorModel* FindBlockingOccupantExcept(
		UTileMapModel* TileMap,
		const FTileIndex& TileIndex,
		const UBoardActorModel* IgnoredActor)
	{
		if (TileMap == nullptr)
		{
			return nullptr;
		}
		for (UBoardActorModel* Actor : TileMap->GetActorsOnTile(
			TileIndex,
			ETileLayerFlag::Obstacle | ETileLayerFlag::Unit))
		{
			if (Actor != nullptr && Actor != IgnoredActor)
			{
				return Actor;
			}
		}
		return nullptr;
	}

	bool HasBlockingOccupantExcept(
		UTileMapModel* TileMap,
		const FTileIndex& TileIndex,
		const UBoardActorModel* IgnoredActor)
	{
		return FindBlockingOccupantExcept(TileMap, TileIndex, IgnoredActor) != nullptr;
	}

	TArray<FTileIndex> BuildThrowDestinationIndexes(
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
		const bool bIsAdjacent = FMath::Max(
			FMath::Abs(PlayerTile.mX - TargetTile.mX),
			FMath::Abs(PlayerTile.mY - TargetTile.mY)) == 1;
		if (bIsAdjacent == false)
		{
			return Result;
		}

		// 던지기는 이미 인접한 적만 집어 들고, 적의 현재 칸을 중심으로 8방향을 고른다.
		// 당기기와 입력 규칙을 분리해 발앞 칸이 숨은 모드 전환으로 해석되지 않게 한다.
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
						TargetTile.mX + StepX * Distance,
						TargetTile.mY + StepY * Distance);
					if (TileMap->IsValidIndex(Candidate) == false)
					{
						break;
					}
					if (Candidate == PlayerTile)
					{
						// 적을 플레이어 몸을 통과시켜 반대편으로 던지는 방향은 후보로 만들지 않는다.
						DirectionDestination = FTileIndex::Invalid;
						break;
					}
					const bool bBlocked = HasBlockingOccupantExcept(TileMap, Candidate, TargetUnit)
						|| TileMap->CanPlace(Candidate, TargetUnit) == false;
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

FSRPGSkillConfirmCommand::FSRPGSkillConfirmCommand()
{
	mCommandType = ESRPGCommandType::SkillConfirm;
}

FSRPGSkillCancelCommand::FSRPGSkillCancelCommand()
{
	mCommandType = ESRPGCommandType::SkillCancel;
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
	case ESRPGCommandType::SkillConfirm:
	{
		if (mSkillBuildPhase == ESRPGSkillBuildPhase::Preview)
		{
			BuildSkill();
			SetBuildPhase(ESRPGSkillBuildPhase::Build);
			MarkActionCompleted(ESRPGActionResult::Succeeded);
			return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
		}
		return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
	}
	case ESRPGCommandType::SkillCancel:
	{
		ClearAllTileHighlights();
		ResetTargetTile();
		ResetDice();
		ResetSkill();
		MarkActionCompleted(ESRPGActionResult::Cancelled);
		SetBuildPhase(ESRPGSkillBuildPhase::None);
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
				/* 던지기 프리뷰 취소는 적 선택을 유지하고 방향 선택으로 한 단계만 돌아간다. */
				if (IsDiceThrowBuildSkill(mSelectedSkill) && mTargetIndex != FTileIndex::Invalid)
				{
					mDisplacementDestination = FTileIndex::Invalid;
					SetBuildPhase(ESRPGSkillBuildPhase::ThrowDestinationSelection);
					RefreshThrowDestinationHighlights();
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
				// 끌기/던지기만 HUD의 명시적인 실행 버튼으로 확정한다. 다른 기존 스킬은
				// 종전대로 같은 목표 칸을 다시 눌러 실행할 수 있어야 한다.
				if (IsDiceDisplacementBuildSkill(mSelectedSkill) == false
					&& mTargetIndex == TargetTileIndex)
				{
					BuildSkill();
					SetBuildPhase(ESRPGSkillBuildPhase::Build);
					MarkActionCompleted(ESRPGActionResult::Succeeded);
					Result = ESRPGCommandResult::Handled;
					break;
				}

				// 던지기 방향은 프리뷰 중에도 다른 화살표로 교체할 수 있다.
				if (IsDiceThrowBuildSkill(mSelectedSkill)
					&& CanSelectThrowDestinationTile(TargetTileIndex))
				{
					SetBuildPhase(ESRPGSkillBuildPhase::ThrowDestinationSelection);
					SetThrowDestinationTile(TargetTileIndex);
					RefreshEffectTileHighlights();
					SetBuildPhase(ESRPGSkillBuildPhase::Preview);
					Result = ESRPGCommandResult::Handled;
					break;
				}
				break;
            }
			case ESRPGSkillBuildPhase::ThrowDestinationSelection:
			{
				if (CanSelectThrowDestinationTile(TargetTileIndex))
				{
					SetThrowDestinationTile(TargetTileIndex);
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
				/* 당기기와 던지기는 서로 다른 다음 단계로 진행한다. */

                if (CanSelectTargetTile(TargetTileIndex) == true)
                {
                    ResetTargetTile();
                    SetBuildPhase(ESRPGSkillBuildPhase::AimSelection);
					if (IsDicePullBuildSkill(mSelectedSkill)
						|| IsDiceStaggerBuildSkill(mSelectedSkill)
						|| IsDiceSwapBuildSkill(mSelectedSkill))
					{
						LockDisplacementTarget(TargetTileIndex);
						RefreshEffectTileHighlights();
						SetBuildPhase(ESRPGSkillBuildPhase::Preview);
					}
					else if (IsDiceThrowBuildSkill(mSelectedSkill))
					{
						LockDisplacementTarget(TargetTileIndex);
						if (mThrowDestinationIndexes.IsEmpty())
						{
							ResetTargetTile();
							RefreshAimableTileHighlights();
							break;
						}
						RefreshThrowDestinationHighlights();
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

bool USRPGSkillBuildAction::IsPullDisplacementPreview() const
{
	return IsDicePullBuildSkill(mSelectedSkill);
}

bool USRPGSkillBuildAction::IsThrowDisplacementPreview() const
{
	return IsDiceThrowBuildSkill(mSelectedSkill);
}

bool USRPGSkillBuildAction::IsStaggerDisplacementPreview() const
{
	return IsDiceStaggerBuildSkill(mSelectedSkill);
}

bool USRPGSkillBuildAction::IsSwapDisplacementPreview() const
{
	return IsDiceSwapBuildSkill(mSelectedSkill);
}

UUnitModel* USRPGSkillBuildAction::GetDisplacementTarget() const
{
	return FindDisplacementTarget(GetTileMap(), mTargetIndex, mInstigator.Get());
}

UBoardActorModel* USRPGSkillBuildAction::GetDisplacementCollisionBlocker() const
{
	if (IsThrowDisplacementPreview() == false
		|| mDisplacementDestination == FTileIndex::Invalid)
	{
		return nullptr;
	}
	return FindBlockingOccupantExcept(GetTileMap(), mDisplacementDestination, GetDisplacementTarget());
}

void USRPGSkillBuildAction::LockDisplacementTarget(const FTileIndex& TargetIndex)
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
	mThrowDestinationIndexes.Reset();
	if (IsDiceThrowBuildSkill(mSelectedSkill))
	{
		mThrowDestinationIndexes = BuildThrowDestinationIndexes(
			TileMap,
			mInstigator.Get(),
			TargetUnit,
			DicePoolModel->GetSelectedDiceSum());
	}
}

void USRPGSkillBuildAction::SetThrowDestinationTile(const FTileIndex& DestinationIndex)
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
	mThrowDestinationIndexes.Empty();
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
			// 적 계획 오버레이는 Aim과 별도 ISM 레이어다. 먼저 계획을 복원한 뒤 아래에서 실제 사거리도
			// 함께 칠해, 스킬 레일을 고른 순간 "어디까지 닿는가"와 "무엇을 바꾸는가"를 동시에 보인다.
			CombatModel->RefreshEnemyIntentHighlights();
		}
	}

	TileMap->SetTileHighlight(mReachableTileIndexes, ETileHighlightFlag::Aim);
}

void USRPGSkillBuildAction::RefreshThrowDestinationHighlights()
{
	UTileMapModel* TileMap = GetTileMap();
	checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));
	UUnitModel* TargetUnit = FindDisplacementTarget(TileMap, mTargetIndex, mInstigator.Get());
	if (TargetUnit == nullptr)
	{
		return;
	}

	ClearAllTileHighlights();
	// 바닥색은 클릭 가능한 종점만 보조한다. HUD가 이 좌표 위에 큰 8방향 화살표를 따로 그린다.
	TileMap->SetTileHighlight(mThrowDestinationIndexes, ETileHighlightFlag::Aim);
	TileMap->SetTileHighlight(TArray<FTileIndex>({ mTargetIndex }), ETileHighlightFlag::Select);
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
	const bool bIsPull = IsDicePullBuildSkill(mSelectedSkill);
	const bool bIsStagger = IsDiceStaggerBuildSkill(mSelectedSkill);
	const bool bIsSwap = IsDiceSwapBuildSkill(mSelectedSkill);
	TArray<FTileIndex> Trajectory;
	if (bIsPull)
	{
		Trajectory = TileMap->GetPullPath(InstigatorTile, TargetTile, 64);
	}
	else if (bIsStagger)
	{
		Trajectory.Add(TargetTile);
	}
	else if (bIsSwap)
	{
		Trajectory.Add(TargetTile);
		Trajectory.Add(InstigatorTile);
	}
	else
	{
		Trajectory.Add(TargetTile);
		if (mDisplacementDestination != FTileIndex::Invalid)
		{
			const FTileIndex ThrowStep(
				FMath::Sign(mDisplacementDestination.mX - TargetTile.mX),
				FMath::Sign(mDisplacementDestination.mY - TargetTile.mY));
			const int32 SelectedDistance = FMath::Max(
				FMath::Abs(mDisplacementDestination.mX - TargetTile.mX),
				FMath::Abs(mDisplacementDestination.mY - TargetTile.mY));
			const int32 ThrowDistance = FMath::Min(SelectedDistance, GetPullThrowDistance(Distance, TargetUnit));
			for (int32 ThrowIndex = 1; ThrowIndex <= ThrowDistance; ++ThrowIndex)
			{
				const FTileIndex Candidate(
					TargetTile.mX + ThrowStep.mX * ThrowIndex,
					TargetTile.mY + ThrowStep.mY * ThrowIndex);
				if (TileMap->IsValidIndex(Candidate) == false
					|| HasBlockingOccupantExcept(TileMap, Candidate, TargetUnit)
					|| TileMap->CanPlace(Candidate, TargetUnit) == false)
				{
					break;
				}
				Trajectory.Add(Candidate);
			}
		}
	}
	if (Trajectory.IsEmpty())
	{
		return;
	}
	mEffectTileIndexes = Trajectory;
	ClearAllTileHighlights();
	const FTileIndex SelectedTile = bIsPull || bIsSwap
		? Trajectory.Last()
		: (mDisplacementDestination != FTileIndex::Invalid ? mDisplacementDestination : Trajectory.Last());
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
	if (IsDiceThrowBuildSkill(mSelectedSkill))
	{
		if (FindDisplacementTarget(GetTileMap(), Index, mInstigator.Get()) == nullptr)
		{
			return false;
		}
		const FTileIndex PlayerTile = mInstigator->GetTileTransform().mIndex;
		return FMath::Max(
			FMath::Abs(PlayerTile.mX - Index.mX),
			FMath::Abs(PlayerTile.mY - Index.mY)) == 1;
	}
	if (IsDiceStaggerBuildSkill(mSelectedSkill))
	{
		return FindDisplacementTarget(GetTileMap(), Index, mInstigator.Get()) != nullptr;
	}
	if (IsDiceSwapBuildSkill(mSelectedSkill))
	{
		if (FindDisplacementTarget(GetTileMap(), Index, mInstigator.Get()) == nullptr)
		{
			return false;
		}
		const FTileIndex PlayerTile = mInstigator->GetTileTransform().mIndex;
		return FMath::Max(
			FMath::Abs(PlayerTile.mX - Index.mX),
			FMath::Abs(PlayerTile.mY - Index.mY)) == 1;
	}
	return true;
}

bool USRPGSkillBuildAction::CanSelectThrowDestinationTile(const FTileIndex& Index) const
{
	return IsDiceThrowBuildSkill(mSelectedSkill)
		&& mTargetIndex != FTileIndex::Invalid
		&& mThrowDestinationIndexes.Contains(Index);
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

