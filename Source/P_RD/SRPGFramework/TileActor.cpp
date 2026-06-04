#include "SRPGFramework/TileActor.h"

ETileLayerFlag ITileActor::GetBlockFlags() const
{
	return ETileLayerFlag::None;
}

void ITileActor::OnBeginTileOverlap(ITileActor* Other, FTile* CurTile)
{
}

void ITileActor::OnEndTileOverlap(ITileActor* Other, FTile* CurTile)
{
}
