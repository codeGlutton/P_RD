#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Teleport.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"

void FSkillEffectLayer_Teleport::CommitEffect(const FSkillEffectCommitParams& Params) const
{
	if (Params.mInstigator == nullptr)
	{
		return;
	}

	UBoardActorModel* InstigatorBoardActor = Cast<UBoardActorModel>(Params.mInstigator.GetObject());
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(InstigatorBoardActor);
	UTileMapModel* TileMapModel = CombatModel->GetTileMap();

	FTileIndex TargetTileIndex = Params.mAimedTileIndex;
	if (TargetTileIndex == FTileIndex::Invalid && Params.mFinalTileIndexes.IsEmpty() == false)
	{
		TargetTileIndex = Params.mFinalTileIndexes[0];
	}

	if (TargetTileIndex == FTileIndex::Invalid)
	{
		return;
	}

	if (TileMapModel->CanPlace(TargetTileIndex, InstigatorBoardActor) == true)
	{
		const FTileTransform NextTransform(TargetTileIndex, InstigatorBoardActor->GetTileTransform().mDirection);

		TileMapModel->RemoveActor(InstigatorBoardActor);
		TileMapModel->PlaceActor(NextTransform, InstigatorBoardActor);
	}
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Teleport"

FText FSkillEffectLayer_Teleport::MakeDescription() const
{
	return LOCTEXT("TeleportDesc", "지정한 위치로 순간이동합니다.");
}

#undef LOCTEXT_NAMESPACE

