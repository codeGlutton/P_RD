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

        UTileMapModel* TileMap = GetTileMap();
        checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

        if (SkillCompModel->IsAnySkillActivated() == true)
        {
            // 이미 스킬 실행 중 무시
            return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
        }

        FOnEndSkillUI Callback;
        Callback.AddWeakLambda(this, [this](const FActiveSkillContext& Context, const UStaticSkillData* PreSkillData) {
            MarkActionCompleted(ESRPGActionResult::Succeeded);
            });

        SkillCompModel->ActivateSkill(TileMap, SkillCastCommand.mSkillIndex, SkillCastCommand.mTargetIndex, MoveTemp(Callback));

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

