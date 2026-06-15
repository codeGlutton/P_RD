#include "Actor/TileMap/Tile.h"
#include "SRPGFramework/TileActor.h"

bool FTile::IsBlocked(const ITileActor* Incoming) const
{
	// 진입 액터가 없으면 블록 없음으로 처리
	if (Incoming == nullptr)
		return false;

	// 기존 액터가 블록하는 레이어가 진입 액터의 레이어와 일치하면 블록
	for (const TScriptInterface<ITileActor>& Actor : mActors)
		if (Actor && EnumHasAnyFlags(Actor->GetBlockLayerFlags(), Incoming->GetTileLayerFlags()))
			return true;

	return false;
}
