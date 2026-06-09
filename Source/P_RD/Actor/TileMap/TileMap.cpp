#include "Actor/TileMap/TileMap.h"
#include "SRPGFramework/TileActor.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// @brief 타일 액터 방향을 yaw(도)로 변환 (Forward 0 / Right 90 / Backward 180 / Left 270)
	float DirectionToYaw(ETileActorDirection Direction)
	{
		switch (Direction)
		{
		case ETileActorDirection::Forward:	return 0.0f;
		case ETileActorDirection::Right:	return 90.0f;
		case ETileActorDirection::Backward:	return 180.0f;
		case ETileActorDirection::Left:		return 270.0f;
		default:							return 0.0f;
		}
	}
}

ATileMap::ATileMap()
{
	// 그리드는 OnConstruction에서 재생성되므로 틱 불필요
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트 생성 및 지정
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 타일 그리드용 인스턴스드 메시 컴포넌트 생성 및 루트에 부착
	mTileMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TileMesh"));
	mTileMeshComponent->SetupAttachment(RootComponent);
	// 시각화 전용이므로 충돌 비활성화
	mTileMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 기본 타일 메시로 엔진 기본 Plane(100x100cm, +Z 향) 지정
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshFinder.Succeeded())
	{
		mTileMesh = PlaneMeshFinder.Object;
		mTileMeshComponent->SetStaticMesh(mTileMesh);
	}

	// 강조 스타일 기본값 (우선순위 Aim < Select < Effect)
	// 조준 범위: 회색 반투명, 바닥에 깔림
	mAimStyle.mColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);
	mAimStyle.mBlendMode = ETileHighlightBlend::Mix;
	mAimStyle.mPriority = 0;

	// 선택 타일: 노란색, Aim 위에 덮어씀
	mSelectStyle.mColor = FLinearColor(1.0f, 0.9f, 0.1f, 1.0f);
	mSelectStyle.mBlendMode = ETileHighlightBlend::Overwrite;
	mSelectStyle.mPriority = 1;

	// 영향 범위: 빨간색, 최상위에서 펄스로 섞임
	mEffectStyle.mColor = FLinearColor(1.0f, 0.1f, 0.1f, 0.6f);
	mEffectStyle.mBlendMode = ETileHighlightBlend::Mix;
	mEffectStyle.mPriority = 2;
}

void ATileMap::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 배치/스폰/프로퍼티 변경 시점에 그리드 인스턴스 재생성
	RebuildTileInstances();
}

void ATileMap::RebuildTileInstances()
{
	// 타일 저장소를 현재 크기에 맞춰 기본 타일로 새로 채움
	mTiles.Init(FTile(), FMath::Max(0, mWidth * mHeight));

	// 컴포넌트가 없으면 처리 불가
	if (mTileMeshComponent == nullptr)
	{
		return;
	}

	// 에디터에서 교체된 메시/머티리얼 반영
	mTileMeshComponent->SetStaticMesh(mTileMesh);
	if (mTileMaterial != nullptr)
	{
		mTileMeshComponent->SetMaterial(0, mTileMaterial);
	}

	// 기존 인스턴스 모두 제거 후 재생성
	mTileMeshComponent->ClearInstances();

	// 메시가 없거나 크기가 비정상이면 빈 그리드로 둠
	if (mTileMesh == nullptr || mWidth <= 0 || mHeight <= 0 || mTileSize <= 0.0f)
	{
		return;
	}

	// 엔진 Plane은 100cm 기준 → 타일 크기에 맞춰 스케일 환산, 시각 비율로 칸 사이 틈 생성
	const float PlaneBaseSize = 100.0f;
	const float InstanceScaleXY = (mTileSize / PlaneBaseSize) * mTileVisualScale;

	// Width x Height 만큼 타일 인스턴스를 로컬 공간에 배치
	for (int32 y = 0; y < mHeight; ++y)
	{
		for (int32 x = 0; x < mWidth; ++x)
		{
			// 타일 (x,y) 중심의 로컬 위치
			const FVector LocalLocation(x * mTileSize, y * mTileSize, 0.0f);
			const FTransform InstanceTransform(FRotator::ZeroRotator, LocalLocation, FVector(InstanceScaleXY, InstanceScaleXY, 1.0f));
			mTileMeshComponent->AddInstance(InstanceTransform, /*bWorldSpace=*/false);
		}
	}
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

bool ATileMap::IsValidIndex(const FTileIndex& TileIndex) const
{
	// 0 <= X < Width && 0 <= Y < Height 범위 검사
	return TileIndex.mX >= 0 && TileIndex.mX < mWidth
		&& TileIndex.mY >= 0 && TileIndex.mY < mHeight;
}

int32 ATileMap::TileIndexToLinearIndex(const FTileIndex& TileIndex) const
{
	// 범위 밖이면 무효
	if (!IsValidIndex(TileIndex))
	{
		return INDEX_NONE;
	}
	// 행 우선(y*Width + x)으로 1차원 인덱스 계산
	return TileIndex.mY * mWidth + TileIndex.mX;
}

const FTile* ATileMap::GetTile(const FTileIndex& TileIndex) const
{
	const int32 LinearIndex = TileIndexToLinearIndex(TileIndex);
	// 맵 범위(INDEX_NONE) + mTiles 미초기화/크기 불일치(IsValidIndex)를 모두 방어
	if (LinearIndex == INDEX_NONE || !mTiles.IsValidIndex(LinearIndex))
	{
		return nullptr;
	}
	return &mTiles[LinearIndex];
}

TArray<TScriptInterface<ITileActor>> ATileMap::GetActorsOnTile(const FTileIndex& TileIndex, ETileLayerFlag LayerFilter) const
{
	TArray<TScriptInterface<ITileActor>> Result;

	// 타일 조회 (범위 밖이면 빈 배열)
	const FTile* Tile = GetTile(TileIndex);
	if (Tile == nullptr)
	{
		return Result;
	}

	// 레이어 필터에 걸리는 액터만 수집
	for (const TScriptInterface<ITileActor>& Actor : Tile->mActors)
	{
		if (Actor && EnumHasAnyFlags(Actor->GetTileLayer(), LayerFilter))
		{
			Result.Add(Actor);
		}
	}
	return Result;
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

void ATileMap::SetTileHighlight(ETileHighlightFlag Flag, const TArray<FTileIndex>& Tiles)
{
	// TODO: Flag 비트를 가진 기존 타일에서 끄고, Tiles에 켠 뒤 custom data 갱신
}

void ATileMap::ClearTileHighlight(ETileHighlightFlag Flag)
{
	// TODO: 모든 타일에서 Flag 비트를 끄고 custom data 갱신
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
	Actor->SetTileTransform(FTileTransform::Invalid);
}

