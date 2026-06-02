#include "Actor/TileMap/TileMap.h"
#include "SRPGFramework/TileActor.h"
#include "Components/SceneComponent.h"

ATileMap::ATileMap()
{
	PrimaryActorTick.bCanEverTick = true;

	// 루트 컴포넌트 생성 및 지정
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

int32 ATileMap::GetWidth() const
{
	return mWidth;
}

int32 ATileMap::GetHeight() const
{
	return mHeight;
}

float ATileMap::GetTileSize() const
{
	return mTileSize;
}

FTransform ATileMap::TileToWorldTransform(const FTileTransform& TileTransform) const
{
	// TODO: 인덱스+방향 → 월드 트랜스폼 변환
	return FTransform::Identity;
}

FVector ATileMap::TileToWorldLocation(const FTileIndex& TileIndex) const
{
	// TODO: 인덱스 → 월드 위치 변환
	return FVector::ZeroVector;
}

FTileIndex ATileMap::WorldToTileIndex(const FVector& WorldLocation) const
{
	// TODO: 월드 위치 → 타일 인덱스 변환
	return FTileIndex();
}

TArray<FTileIndex> ATileMap::GetReachableTiles(const FTileIndex& Origin, int32 MoveDistance) const
{
	// TODO: 경로 기반 도달 가능 타일 계산
	return TArray<FTileIndex>();
}

TArray<FTileIndex> ATileMap::GetAimableTiles(const FTileIndex& Origin, int32 Range, EAimPattern Pattern, bool bIncludeOccupied, bool bIndirect) const
{
	// TODO: 조준 패턴별 조준 가능 타일 계산
	return TArray<FTileIndex>();
}

TArray<FTileIndex> ATileMap::GetEffectTiles(const FTileIndex& Caster, const FTileIndex& Target, EEffectPattern Pattern, int32 Size, bool bPenetrate) const
{
	// TODO: 영향 패턴별 영향 타일 계산
	return TArray<FTileIndex>();
}

bool ATileMap::IsBlocking(const FTileIndex& TileIndex, ETileLayerFlag LayerFlag) const
{
	// TODO: 해당 타일의 특정 레이어 블로킹 여부 검사
	return false;
}

void ATileMap::StartActorMovement(const FTileTransform& NextTransform, ITileActor* Actor)
{
	checkf(IsBlocking(NextTransform.mIndex, Actor->GetTileLayer()) == false, TEXT("배치할 수 없는 타일"));

	// TODO: 이전 타일에 겹치는 객체마다 OnEndTileOverlap 이벤트

	// TODO: 이전 타일에서 해제
	
	// TODO: 다음 타일에 등록
	Actor->SetTileTransform(NextTransform);
}

void ATileMap::CompleteActorMovement(ITileActor* Actor)
{
	// TODO: 다음 타일에 겹치는 객체마다 OnBeginTileOverlap 이벤트
}

void ATileMap::PlaceActor(const FTileTransform& NextTransform, ITileActor* Actor)
{
	checkf(IsBlocking(NextTransform.mIndex, Actor->GetTileLayer()) == false, TEXT("배치할 수 없는 타일"));

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

