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

void ITileActor::OnBeginRound()
{
}

void ITileActor::OnEndRound()
{
}

void ITileActor::OnBeginTurn()
{
}

void ITileActor::OnEndTurn()
{
}
