#include "Actor/TileMap/TileMap.h"
#include "SRPGFramework/TileActor.h"

ATileMap::ATileMap()
{
	PrimaryActorTick.bCanEverTick = true;
}

const FTransform& ATileMap::GetTileTransform(const FTileTransform& Transform) const
{
	// TODO: 여기에 return 문을 삽입합니다.
	static FTransform Tmp;
	return Tmp;
}

const FTransform& ATileMap::GetTileTransform(ETileRotation Direction, int32 Row, int32 Column) const
{
	// TODO: 여기에 return 문을 삽입합니다.
	static FTransform Tmp;
	return Tmp;
}

const FVector& ATileMap::GetTilePosition(int32 Row, int32 Column) const
{
	// TODO: 여기에 return 문을 삽입합니다.
	static FVector Tmp;
	return Tmp;
}

bool ATileMap::IsBlocking(const FTileTransform& Transform, ETileLayerFlag LayerFlag) const
{
	// TODO: 해당 타일의 특정 레이어 블로킹 여부 검사
	return false;
}

void ATileMap::BeginActorMovement(const FTileTransform& NextTransform, ITileActor* Actor)
{
	checkf(IsBlocking(NextTransform, Actor->GetTileLayer()) == false, TEXT("배치할 수 없는 타일"));

	// TODO: 이전 타일에 겹치는 객체마다 OnEndTileOverlap 이벤트

	// TODO: 이전 타일에서 해제
	
	// TODO: 다음 타일에 등록
	Actor->SetTileTransform(NextTransform);
}

void ATileMap::FinishActorMovement(ITileActor* Actor)
{
	// TODO: 다음 타일에 겹치는 객체마다 OnBeginTileOverlap 이벤트
}

void ATileMap::PlaceActor(const FTileTransform& NextTransform, ITileActor* Actor)
{
	checkf(IsBlocking(NextTransform, Actor->GetTileLayer()) == false, TEXT("배치할 수 없는 타일"));

	// TODO: 다음 타일에 등록
	Actor->SetTileTransform(NextTransform);
	
	// TODO: 다음 타일에 겹치는 객체마다 OnBeginTileOverlap 이벤트
}

void ATileMap::RemoveActor(ITileActor* Actor)
{
	// TODO: 이전 타일에 겹치는 객체마다 OnEndTileOverlap 이벤트
	
	// TODO: 이전 타일에서 해제
	Actor->SetTileTransform(FTileTransform::Zero);
}

