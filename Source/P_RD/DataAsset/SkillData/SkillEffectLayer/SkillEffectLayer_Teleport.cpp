#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Teleport.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/BoardMovementComponent/BoardMovementComponentModel.h"

void FSkillEffectLayer_Teleport::CommitEffect(const FSkillEffectCommitParams& Params) const
{
	if (Params.mInstigator == nullptr)
	{
		return;
	}

	UBoardActorModel* InstigatorBoardActor = Cast<UBoardActorModel>(Params.mInstigator.GetObject());

	FTileIndex TargetTileIndex = Params.mAimedTileIndex;
	if (TargetTileIndex == FTileIndex::Invalid && Params.mFinalTileIndexes.IsEmpty() == false)
	{
		TargetTileIndex = Params.mFinalTileIndexes[0];
	}

	if (TargetTileIndex == FTileIndex::Invalid)
	{
		return;
	}

	// 이동 컴포넌트 모델로 텔레포트. 막힌 타일 검사는 TeleportTo가 처리
	IBoardCombatTarget* CombatTarget = Cast<IBoardCombatTarget>(InstigatorBoardActor);
	UBoardMovementComponentModel* MoveCompModel = (CombatTarget != nullptr) ? CombatTarget->GetBoardMovementComponentModel() : nullptr;
	if (MoveCompModel != nullptr)
	{
		const FTileTransform NextTransform(TargetTileIndex, InstigatorBoardActor->GetTileTransform().mDirection);
		MoveCompModel->TeleportTo(NextTransform);
	}
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Teleport"

FText FSkillEffectLayer_Teleport::MakeDescription() const
{
	return LOCTEXT("TeleportDesc", "지정한 위치로 순간이동합니다.");
}

#undef LOCTEXT_NAMESPACE

