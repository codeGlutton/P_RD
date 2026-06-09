#include "Actor/TileMap/Tile.h"
#include "SRPGFramework/TileActor.h"

/**
 * @note 현재 단방향(진입자 GetBlockFlags 기준)만 검사한다
 * @todo 블로킹 규칙이 양방향으로 확정되면 기존 액터의 GetBlockFlags 검사를 OR로 추가
 */
bool FTile::IsBlocked(const ITileActor* Incoming) const
{
	// 진입 액터가 없으면 블록 없음으로 처리
	if (Incoming == nullptr)
	{
		return false;
	}

	// 진입 액터가 블록하는 레이어와 기존 액터의 레이어가 일치하면 블록
	for (const TScriptInterface<ITileActor>& Actor : mActors)
	{
		if (Actor && EnumHasAnyFlags(Incoming->GetBlockFlags(), Actor->GetTileLayer()))
		{
			return true;
		}
	}
	return false;
}
