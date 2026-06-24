#include "TAS/Effect/TacticalEffect.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "TAS/Effect/TacticalEffectContext.h"


UBoardActorModel* UTacticalEffect::ExtractTarget(const FTileIndex& TargetTile, const UTacticalEffectContext* EffectContext)
{
	// CombatModel을 가져옵니다.
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(IsValid(CombatModel), TEXT("전투 모델이 비어있습니다."));

	// 서브 시스템에 접근하여 타일맵 모델을 가져온다.
	TWeakObjectPtr<UTileMapModel> TMModel = CombatModel->GetTileMap();
	checkf(TMModel.IsValid(), TEXT("타일맵 모델이 존재하지 않습니다."));
	// ===============================================================

	// 해당 타일에 유닛 모델을 가져온다.
	TWeakObjectPtr<UBoardActorModel> TargetBoardModel = TMModel->GetActorOnTile<UBoardActorModel>(TargetTile, EffectContext->mTileLayerFlag);

	// 유닛이 없다면 반환한다.
	if (!TargetBoardModel.IsValid())
		return nullptr;

	return TargetBoardModel.Get();
}