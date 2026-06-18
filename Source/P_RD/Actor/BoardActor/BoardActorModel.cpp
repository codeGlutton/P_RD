#include "Actor/BoardActor/BoardActorModel.h"

const FTileTransform& UBoardActorModel::GetTileTransform() const
{
	// TODO: 모델이 mTileTransform 저장하도록 전환, 현재는 무효 트랜스폼 반환
	return FTileTransform::Invalid;
}

void UBoardActorModel::SetTileTransform(const FTileTransform& Transform)
{
	// TODO: 모델이 mTileTransform 저장하도록 전환, 현재는 빈 구현
}

ETileLayerFlag UBoardActorModel::GetTileLayerFlags() const
{
	// TODO: 서브클래스/데이터로 레이어 타입 지정, 현재는 None 반환
	return ETileLayerFlag::None;
}

ETileLayerFlag UBoardActorModel::GetBlockLayerFlags() const
{
	// TODO: 구현 예정, 현재는 None 반환
	return ETileLayerFlag::None;
}

ETileLayerFlag UBoardActorModel::GetReplaceLayerFlags() const
{
	// TODO: 구현 예정, 현재는 None 반환
	return ETileLayerFlag::None;
}

int32 UBoardActorModel::GetOverlayLayerPriority() const
{
	// TODO: 구현 예정, 현재는 0 반환
	return 0;
}

void UBoardActorModel::OnBeginTileOverlap(FTile* CurTile, UBoardActorModel* Other)
{
	// TODO: 구현 예정
}

void UBoardActorModel::OnEndTileOverlap(FTile* CurTile, UBoardActorModel* Other)
{
	// TODO: 구현 예정
}

void UBoardActorModel::OnReplaced(FTile* CurTile, UBoardActorModel* Other)
{
	// TODO: 구현 예정
}

void UBoardActorModel::OnBeginRound()
{
	// TODO: 구현 예정
}

void UBoardActorModel::OnEndRound()
{
	// TODO: 구현 예정
}

void UBoardActorModel::OnBeginTurn()
{
	// TODO: 구현 예정
}

void UBoardActorModel::OnEndTurn()
{
	// TODO: 구현 예정
}
