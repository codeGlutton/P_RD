#include "Actor/BoardActor/BoardActorModel.h"
#include "Component/ComponentModel.h"

ETileLayerFlag UBoardActorModel::GetBlockLayerFlags() const
{
	return ETileLayerFlag::None;
}

ETileLayerFlag UBoardActorModel::GetReplaceLayerFlags() const
{
	return ETileLayerFlag::None;
}

int32 UBoardActorModel::GetOverlayLayerPriority() const
{
	return 0;
}

void UBoardActorModel::OnBeginTileOverlap(FTile* CurTile, UBoardActorModel* Other)
{
}

void UBoardActorModel::OnEndTileOverlap(FTile* CurTile, UBoardActorModel* Other)
{
}

void UBoardActorModel::OnReplaced(FTile* CurTile, UBoardActorModel* Other)
{
}

void UBoardActorModel::OnBeginRound()
{
}

void UBoardActorModel::OnEndRound()
{
}

void UBoardActorModel::OnBeginTurn()
{
}

void UBoardActorModel::OnEndTurn()
{
}
