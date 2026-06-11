#include "SRPGFramework/TileActor.h"

ETileLayerFlag ITileActor::GetBlockLayerFlags() const
{
	return ETileLayerFlag::None;
}

ETileLayerFlag ITileActor::GetReplaceLayerFlags() const
{
	return ETileLayerFlag::None;
}

int32 ITileActor::GetOverlayLayerPriority() const
{
	return 0;
}

void ITileActor::OnBeginTileOverlap(FTile* CurTile, ITileActor* Other)
{
}

void ITileActor::OnEndTileOverlap(FTile* CurTile, ITileActor* Other)
{
}

void ITileActor::OnReplaced(FTile* CurTile, ITileActor* Other)
{
}

void ITileActor::OnBeginRound()
{
}

void ITileActor::OnBeginCycle()
{
}

void ITileActor::OnBeginTurn()
{
}
