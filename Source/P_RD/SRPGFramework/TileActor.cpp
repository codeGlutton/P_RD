#include "SRPGFramework/TileActor.h"

bool ITileActor::IsBlocking() const
{
	return true;
}

void ITileActor::OnBeginTileOverlap(ITileActor* Other) const
{
}

void ITileActor::OnEndTileOverlap(ITileActor* Other) const
{
}
