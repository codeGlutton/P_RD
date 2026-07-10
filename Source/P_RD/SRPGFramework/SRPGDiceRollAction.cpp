#include "SRPGFramework/SRPGDiceRollAction.h"
#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

#include "Pawn/Player/PlayerUnitModel.h"

#include "Dice/DicePoolModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"

#include "TAS/Passive/TacticalPassive.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData.h"

FSRPGDicePrepareCommand::FSRPGDicePrepareCommand()
{
    mCommandType = ESRPGCommandType::DicePrepare;
    mRequestedAction = USRPGDiceRollAction::StaticClass();
}

FSRPGDiceRollCommand::FSRPGDiceRollCommand()
{
    mCommandType = ESRPGCommandType::DiceRoll;
}

USRPGDiceRollAction::USRPGDiceRollAction()
{
    mActionType = ESRPGActionType::InPlayAction;
    mConsumesTurn = false;
}

ESRPGCommandResult USRPGDiceRollAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = Super::HandleCommand(Command);
    if (Result == ESRPGCommandResult::Handled)
    {
        return Result;
    }

    switch (Command.Get().GetCommandType())
    {
    case ESRPGCommandType::DicePrepare:
    {
        /* 주사위 팝업 보여주기 */

        UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
        checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));
        UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
        checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트 모델 nullptr"));
        
        DicePoolModel->ResetUsed();

        const FSRPGDicePrepareCommand& DicePrepareCommand = Command.Get<FSRPGDicePrepareCommand>();
        DicePrepareCommand.OnShowDicePanelUI.Broadcast();

        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    case ESRPGCommandType::DiceRoll:
    {
        /* 주사위 굴리기 */

        UPlayerUnitModel* PlayerUnit = Cast<UPlayerUnitModel>(mInstigator.Get());
        checkf(PlayerUnit != nullptr, TEXT("주사위를 굴릴 수 있는 플레이어 유닛이 아님"));
        UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
        checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트 모델 nullptr"));
        UPassiveComponentModel* PassiveComponentModel = PlayerUnit->GetPassiveComponentModel();
        checkf(PassiveComponentModel != nullptr, TEXT("패시브 컴포넌트 nullptr"));

        // 주사위 굴리기 전 패시브
        {
            TArray<UTacticalPassive*> Passives = PassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartRollingDice);
            const int32 PassiveNum = Passives.Num();

            FBoardCombatTargetSnapshotData PlayerUnitSnapshot = PlayerUnit->MakeSnapshotData();

            FPassiveActivateContext PassiveContext;
            PassiveContext.mOwner = PlayerUnit;
            PassiveContext.mOwnerSnapshot = &PlayerUnitSnapshot;
            PassiveContext.mTargets.Add(PlayerUnit);
            PassiveContext.mTargetSnapshots.Add(&PlayerUnitSnapshot);

            for (UTacticalPassive*& Passive : Passives)
            {
                TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
                Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnStartRollingDice, PassiveContext, OUT DynamicPassiveData);
                Passive->CommitPassive(DynamicPassiveData);
            }
        }

        const FSRPGDiceRollCommand& DiceRollCommand = Command.Get<FSRPGDiceRollCommand>();
        if (DiceRollCommand.mRolledFaceIndices.IsEmpty() == true)
        {
            DicePoolModel->RollAll(URandomStreamFunctionLibrary::GetEventStream(this));
        }
        else
        {
            // 물리 굴림 연출이 보여준 윗면을 그대로 기록한다("보이는 면 = 기록 숫자").
            DicePoolModel->ApplyRolledFaceIndices(DiceRollCommand.mRolledFaceIndices);
        }

        // 주사위 굴리기 후 패시브
        {
            TArray<UTacticalPassive*> Passives = PassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnEndRollingDice);
            const int32 PassiveNum = Passives.Num();

            FBoardCombatTargetSnapshotData PlayerUnitSnapshot = PlayerUnit->MakeSnapshotData();

            FPassiveActivateContext PassiveContext;
            PassiveContext.mOwner = PlayerUnit;
            PassiveContext.mOwnerSnapshot = &PlayerUnitSnapshot;
            PassiveContext.mTargets.Add(PlayerUnit);
            PassiveContext.mTargetSnapshots.Add(&PlayerUnitSnapshot);

            for (UTacticalPassive*& Passive : Passives)
            {
                TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
                Passive->ActivatePassive(AbilityTags::GameplayAbility_Passive_OnEndRollingDice, PassiveContext, OUT DynamicPassiveData);
                Passive->CommitPassive(DynamicPassiveData);
            }
        }

        MarkActionCompleted(ESRPGActionResult::Succeeded);
        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }
    }

    return ESRPGCommandResult::Ignored;
}

