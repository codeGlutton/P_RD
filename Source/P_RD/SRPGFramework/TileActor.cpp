#include "SRPGFramework/TileActor.h"

ETileLayerFlag ITileActor::GetBlockLayerFlags() const
{
	return ETileLayerFlag::None;
}

ETileLayerFlag ITileActor::GetReplaceLayerFlags() const
{
	return ETileLayerFlag::None;
}

void ITileActor::OnBeginTileOverlap(ITileActor* Other, FTile* CurTile)
{
}

void ITileActor::OnEndTileOverlap(ITileActor* Other, FTile* CurTile)
{
}

void OnBeginRound()
{
}

void OnBeginCycle()
{
}

void OnBeginTurn()
{
}
