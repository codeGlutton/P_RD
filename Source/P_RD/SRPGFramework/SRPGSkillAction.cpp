#include "SRPGFramework/SRPGSkillAction.h"

#include "Pawn/UnitModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"

#include "TAS/Passive/TacticalPassive.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"

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
        UPassiveComponentModel* PassiveComponentModel = mInstigator->GetPassiveComponentModel();
        checkf(PassiveComponentModel != nullptr, TEXT("패시브 컴포넌트 nullptr"));

        // 스킬 시전 전 패시브
        {
            TArray<UTacticalPassive*> Passives = PassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnStartUsingSkill);
            const int32 PassiveNum = Passives.Num();

            FBoardCombatTargetSnapshotData mInstigatorSnapshot = mInstigator->MakeSnapshotData();

            FPassiveActivateContext PassiveContext;
            PassiveContext.mOwner = mInstigator.Get();
            PassiveContext.mOwnerSnapshot = &mInstigatorSnapshot;

            for (UTacticalPassive*& Passive : Passives)
            {
                TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
                Passive->ActivatePassive(PassiveContext, OUT DynamicPassiveData);
                Passive->CommitPassive(DynamicPassiveData);
            }
        }

        UTileMapModel* TileMap = GetTileMap();
        checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));
        SkillCompModel->ActivateSkill(TileMap, SkillCastCommand.mSkillIndex, SkillCastCommand.mTargetIndex, SkillCastCommand.mDiceSum);

        {
            // TODO : 원래는 비동기 적으로 애니메이션 종료 타이밍을 알려줘서 끝내야함. 임시적으로 곧바로 종료

            // 스킬 시전 후 패시브
            {
                TArray<UTacticalPassive*> Passives = PassiveComponentModel->GetPassivesByTiming(AbilityTags::GameplayAbility_Passive_OnEndUsingSkill);
                const int32 PassiveNum = Passives.Num();

                FBoardCombatTargetSnapshotData mInstigatorSnapshot = mInstigator->MakeSnapshotData();

                FPassiveActivateContext PassiveContext;
                PassiveContext.mOwner = mInstigator.Get();
                PassiveContext.mOwnerSnapshot = &mInstigatorSnapshot;

                for (UTacticalPassive*& Passive : Passives)
                {
                    TInstancedStruct<FDynamicPassiveData> DynamicPassiveData;
                    Passive->ActivatePassive(PassiveContext, OUT DynamicPassiveData);
                    Passive->CommitPassive(DynamicPassiveData);
                }
            }

            MarkActionCompleted(ESRPGActionResult::Succeeded);
        }

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

