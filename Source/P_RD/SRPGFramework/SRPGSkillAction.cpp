#include "SRPGFramework/SRPGSkillAction.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/PresentationSyncSubsystem.h"

#include "Pawn/Unit.h"

#include "FunctionLibrary/GASTargetFunctionLibrary.h"

#include "DataAsset/SkillData/StaticSkillData.h"
#include "Pawn/SkillComponent.h"

TSharedPtr<FSRPGAction> FSRPGSkillActionDraft::FinalizeDraft() const
{
    TSharedPtr<FSRPGSkillAction> SkillAction = TSharedPtr<FSRPGSkillAction>(new FSRPGSkillAction(), [](FSRPGSkillAction* Action) {
        delete Action;
        });

    return SkillAction;
}

void FSRPGSkillActionDraft::OnFinalizeDraft()
{
    
}

void FSRPGSkillActionDraft::OnDiscardDraft()
{

}

ESRPGActionDraftType FSRPGSkillActionDraft::GetDraftType() const
{
    if (mInstigator->IsPlayerUnit() == true)
    {
        return ESRPGActionDraftType::Interactive;
    }
    else
    {
        return ESRPGActionDraftType::Immediate;
    }
}

void FSRPGSkillActionDraft::SetSkill(int32 SkillIndex)
{
    USkillComponent* SkillComp = mInstigator->GetSkillComponent();
    
    TSoftObjectPtr<UStaticSkillData> StaticSkillDataSoftObj = nullptr;
    SkillComp->GetSkillData(SkillIndex, OUT StaticSkillDataSoftObj);
    if (StaticSkillDataSoftObj.IsNull() == true)
    {
        UE_LOG(LogSRPGCombat, Warning, TEXT("스킬 시전 시 비정상적 스킬 선택"));

        return;
    }
    mSelectedSkill = StaticSkillDataSoftObj.Get();

    // TODO
}

void FSRPGSkillActionDraft::SetTargetTile(const FTileIndex& TileIndex)
{
    // TODO
}

FTileIndex FSRPGSkillActionDraft::FindTargetTileUnderCursor() const
{
    // TODO

    return FTileIndex::Invalid;
}

void FSRPGSkillAction::BeginAction()
{
    Super::BeginAction();
}

void FSRPGSkillAction::TickAction(float DeltaTime)
{
    Super::TickAction(DeltaTime);
}

void FSRPGSkillAction::EndAction()
{
    Super::EndAction();
}

bool FSRPGSkillAction::IsTurnEndingAction() const
{
    return false;
}
