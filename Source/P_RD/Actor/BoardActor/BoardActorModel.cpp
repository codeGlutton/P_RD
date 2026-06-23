#include "Actor/BoardActor/BoardActorModel.h"
#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"

void UBoardActorModel::SetStaticSpawnData(UStaticObstacleSpawnData* StaticSpawnData)
{
	mStaticSpawnData = StaticSpawnData;
}

FName UBoardActorModel::GetBoardActorKeyName() const
{
	checkf(mStaticSpawnData != nullptr, TEXT("스폰 데이터 nullptr"));
	return mStaticSpawnData->GetKeyName();
}

const FText& UBoardActorModel::GetBoardActorDisplayName() const
{
	checkf(mStaticSpawnData != nullptr, TEXT("스폰 데이터 nullptr"));
	return mStaticSpawnData->mDisplayName;
}

const FTileTransform& UBoardActorModel::GetTileTransform() const
{
	// 멤버에 저장된 타일 트랜스폼 반환
	return mTileTransform;
}

void UBoardActorModel::SetTileTransform(const FTileTransform& Transform)
{
	// 멤버에 타일 트랜스폼 저장
	mTileTransform = Transform;
}

ETileLayerFlag UBoardActorModel::GetTileLayerFlags() const
{
	// 멤버에 설정된 레이어 타입 반환
	return StaticCast<ETileLayerFlag>(mTileLayerFlags);
}

ETileLayerFlag UBoardActorModel::GetBlockLayerFlags() const
{
	// 멤버에 설정된 블로킹 레이어 타입들 반환
	return StaticCast<ETileLayerFlag>(mBlockLayerFlags);
}

ETileLayerFlag UBoardActorModel::GetReplaceLayerFlags() const
{
	// 멤버에 설정된 교체 레이어 타입들 반환
	return StaticCast<ETileLayerFlag>(mReplaceLayerFlags);
}

int32 UBoardActorModel::GetOverlayLayerPriority() const
{
	// 멤버에 설정된 교체 우선순위 반환
	return mOverlayLayerPriority;
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
