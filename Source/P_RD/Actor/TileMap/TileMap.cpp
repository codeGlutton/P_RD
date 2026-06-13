#include "Actor/TileMap/TileMap.h"
#include "RDCollision.h"
#include "SRPGFramework/TileActor.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Algo/Reverse.h"

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
	// 터치 판정(타일 선택/정보 확인 트레이스)을 받기 위해 타일맵 프로파일 적용 (QueryOnly)
	mTileMeshComponent->SetCollisionProfileName(RDCollisionProfiles::TileMap);

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

TScriptInterface<ITileActor> ATileMap::ToTileActorInterface(ITileActor* Actor)
{
	// TScriptInterface는 UObject 핸들이 필요하므로 _getUObject로 변환
	return TScriptInterface<ITileActor>(Actor->_getUObject());
}

void ATileMap::RegisterActorToTile(FTile* Tile, ITileActor* Actor)
{
	// 타일 액터 목록에 추가
	Tile->mActors.Add(ToTileActorInterface(Actor));
}

void ATileMap::UnregisterActorFromTile(FTile* Tile, ITileActor* Actor)
{
	// 타일 액터 목록에서 제거
	Tile->mActors.Remove(ToTileActorInterface(Actor));
}

void ATileMap::NotifyBeginOverlap(FTile* Tile, ITileActor* Actor)
{
	// 같은 타일의 다른 액터들과 양방향 OnBegin 통지 (자기 제외)
	for (const auto& Other : Tile->mActors)
	{
		if (Other.GetInterface() == Actor)
		{
			continue;
		}
		Actor->OnBeginTileOverlap(Tile, Other.GetInterface());
		Other->OnBeginTileOverlap(Tile, Actor);
	}
}

void ATileMap::NotifyEndOverlap(FTile* Tile, ITileActor* Actor)
{
	// 같은 타일의 다른 액터들과 양방향 OnEnd 통지 (자기 제외)
	for (const auto& Other : Tile->mActors)
	{
		if (Other.GetInterface() == Actor)
		{
			continue;
		}
		Actor->OnEndTileOverlap(Tile, Other.GetInterface());
		Other->OnEndTileOverlap(Tile, Actor);
	}
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

FTile* ATileMap::GetTile(const FTileIndex& TileIndex)
{
	// 로직 중복을 피하려 const 버전에 위임 후 반환값의 const만 제거
	return const_cast<FTile*>(AsConst(*this).GetTile(TileIndex));
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
		if (Actor && EnumHasAnyFlags(Actor->GetTileLayerFlags(), LayerFilter))
		{
			Result.Add(Actor);
		}
	}
	return Result;
}

FTransform ATileMap::TileToWorldTransform(const FTileTransform& TileTransform) const
{
	// 월드 위치 획득
	const FVector WorldLocation = TileToWorldLocation(TileTransform.mIndex);
	// 타일맵의 YAW 회전을 타일에 적용 (그래야 같은 방향을 바라보니까)
	const FQuat WorldRotation = GetActorQuat() * FRotator(0.0f, DirectionToYaw(TileTransform.mDirection), 0.0f).Quaternion();
	// 타일맵의 스케일을 타일에 적용
	return FTransform(WorldRotation, WorldLocation, GetActorScale3D());
}

FVector ATileMap::TileToWorldLocation(const FTileIndex& TileIndex) const
{
	// 타일 (x,y) 중심의 로컬 위치 (RebuildTileInstances의 인스턴스 배치식과 동일)
	// 메시 피벗이 중심인 엔진 Plane 기준이라 (X*TileSize, Y*TileSize)가 곧 타일 중심
	// (피벗이 모서리인 커스텀 메시로 교체 시 이 가정이 깨지므로 양쪽 모두 보정 필요)
	const FVector LocalLocation(TileIndex.mX * mTileSize, TileIndex.mY * mTileSize, 0.0f);
	// 액터 트랜스폼(위치/회전/스케일)을 반영해 월드 위치로 변환
	return GetActorTransform().TransformPosition(LocalLocation);
}

FTileIndex ATileMap::WorldToTileIndex(const FVector& WorldLocation) const
{
	// 타일 크기가 설정되지 않으면 계산 불가 (0으로 나눗셈 방지)
	if (mTileSize <= 0.0f)
	{
		return FTileIndex::Invalid;
	}

	// 월드 좌표를 로컬 좌표로 변환
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);

	// 로컬 좌표를 타일 크기로 나눈 뒤 반올림해 가장 가까운 타일 중심을 인덱스로 지정
	// 예) 80 -> 80 / 100 = 0 -> 인덱스 0
	// 예) 110 -> 110 / 100 = 1.1 -> 반올림(1.1) = 1 -> 인덱스 1 
	const FTileIndex TileIndex(
		FMath::RoundToInt(LocalLocation.X / mTileSize),
		FMath::RoundToInt(LocalLocation.Y / mTileSize)
	);

	// 맵 범위 밖이면 Invalid 반환
	return IsValidIndex(TileIndex) ? TileIndex : FTileIndex::Invalid;
}

TArray<FTileIndex> ATileMap::GetReachableTiles(const FTileIndex& Origin, int32 MoveDistance) const
{
	// TODO: 경로 기반 도달 가능 타일 계산
	return TArray<FTileIndex>();
}

void ATileMap::AppendRayTiles(const FTileIndex& Origin, const FTileIndex& Step, int32 Range, TArray<FTileIndex>& Out) const
{
	// 원점에서 Step 방향으로 한 칸씩 전진하며 수집 (원점 자신은 제외)
	FTileIndex Current = Origin;
	for (int32 Distance = 0; Distance < Range; ++Distance)
	{
		// 다음 칸으로 전진
		Current.mX += Step.mX;
		Current.mY += Step.mY;

		// 맵 밖으로 나가면 이 방향은 더 진행하지 않고 종료
		if (!IsValidIndex(Current))
			break;

		// 맵 안의 칸만 후보로 누적
		Out.Add(Current);
	}
}

void ATileMap::BresenhamLine(const FTileIndex& From, const FTileIndex& To, TArray<FTileIndex>& Out) const
{
	// 동점(2*Error == ±Delta) 처리가 진행 방향에 따라 다른 칸을 고르지 않도록,
	// 항상 사전순(X 우선, 같으면 Y)으로 작은 쪽에서 큰 쪽으로만 그린다
	const bool bSwapped = (To.mX < From.mX) || (To.mX == From.mX && To.mY < From.mY);
	const FTileIndex& Start = bSwapped ? To : From;
	const FTileIndex& End = bSwapped ? From : To;

	// 이번 호출이 추가하는 구간의 시작 위치 (Out은 누적 배열이므로 이 구간만 뒤집어야 함)
	const int32 FirstIndex = Out.Num();

	// 시작점에서 끝점까지의 정수 좌표 (Start부터 한 칸씩 전진)
	int32 X = Start.mX;
	int32 Y = Start.mY;
	const int32 X1 = End.mX;
	const int32 Y1 = End.mY;

	// 각 축별로 전체이동량과 전진방향 결정
	const int32 DeltaX = FMath::Abs(X1 - X);
	const int32 DeltaY = FMath::Abs(Y1 - Y);
	const int32 StepX = (X < X1) ? 1 : -1;
	const int32 StepY = (Y < Y1) ? 1 : -1;

	// 직선에서 타일이 얼마나 멀리 있는 지 나타내는 오차 누적값 (dx - dy 기준)
	// 이 오차를 줄이는 방향, 즉 직선과 가까운 방향으로 움직이는 게 기본 아이디어
	// 단, dx - dy 이므로 x축으로 편향된 오차이므로, 오차가 클 수록 x축으로, 오차가 작을수록 y축으로 이동하는 압력이 세진다.
	int32 Error = DeltaX - DeltaY;

	while (true)
	{
		// 현재 칸 수집 (정규화된 방향이므로 Start가 첫 원소, End가 마지막 원소)
		Out.Add(FTileIndex(X, Y));

		// 끝점에 도달하면 종료
		if (X == X1 && Y == Y1)
			break;

		// 오차에 따라 x축/y축 전진 결정
		// 양쪽 동시에 넘으면 대각선 이동
		// @note 나눗셈 계산을 생략하기 위해 x2 값과 비교
		const int32 DoubleError = 2 * Error;
		
		// x축으로 이동하는 게 이동하지 않는 것보다 오차를 줄여주는 경우 -> x축으로 이동
		// @note 원래는 DeltaY의 절반을 기준으로 판단해야 하는데, 나눗셈을 피하기 위해 오차를 두 배 해서 비교한다.
		if (DoubleError > -DeltaY)
		{
			// x축으로 이동하는 게 y축 오차를 줄여주니까 dy만큼 오차 감소
			Error -= DeltaY;
			X += StepX;
		}
		// y축으로 이동하는 게 이동하지 않는 것보다 오차를 줄여주는 경우 -> y축으로 이동
		// @note 원래는 DeltaX의 절반을 기준으로 판단해야 하는데, 나눗셈을 피하기 위해 오차를 두 배 해서 비교한다.
		if (DoubleError < DeltaX)
		{
			// y축으로 이동하면 그 다음부터는 x축 이동 압력이 커질 수 있도록 Error 수치 조정
			Error += DeltaX;
			Y += StepY;
		}
	}

	// 뒤집어 그린 경우 이번에 추가한 구간만 반전해 "From이 첫 원소, To가 마지막 원소" 되도록 변경
	if (bSwapped)
		Algo::Reverse(MakeArrayView(Out.GetData() + FirstIndex, Out.Num() - FirstIndex));
}

void ATileMap::RasterizeLine(const FTileIndex& From, const FTileIndex& To, TArray<FTileIndex>& Out) const
{
	// 현재 래스터화 방식: Bresenham (Supercover 등으로 바꾸려면 이 호출만 교체)
	BresenhamLine(From, To, Out);
}

bool ATileMap::HasLineOfSight(const FTileIndex& From, const FTileIndex& To) const
{
	// From→To 직선이 지나는 칸들을 래스터화 (첫 원소=From, 마지막 원소=To 보장)
	TArray<FTileIndex> LineTiles;
	RasterizeLine(From, To, LineTiles);

	// 양 끝(From, To)을 제외한 중간 칸만 검사
	for (int32 Index = 1; Index < LineTiles.Num() - 1; ++Index)
	{
		// 중간 칸에 시야를 막는 액터(Obstacle 또는 Unit)가 있으면 시야가 막힘(=LoS:false)
		if (GetActorsOnTile(LineTiles[Index], ETileLayerFlag::Obstacle | ETileLayerFlag::Unit).Num() > 0)
		{
			return false;
		}
	}

	// 시야를 막는 액터가 없음 (=LoS:true)
	return true;
}

TArray<FTileIndex> ATileMap::GetAimableTiles(const FTileIndex& Origin, int32 Range, EAimPattern Pattern, bool bIncludeOccupied, bool bIndirect, const ITileActor* Incoming) const
{
	// TODO: 조준 패턴별 조준 가능 타일 계산
	return TArray<FTileIndex>();
}

TArray<FTileIndex> ATileMap::GetEffectTiles(const FTileIndex& Caster, const FTileIndex& Target, EEffectPattern Pattern, int32 Size, bool bPenetrate) const
{
	// TODO: 영향 패턴별 영향 타일 계산
	return TArray<FTileIndex>();
}

void ATileMap::SetTileHighlight(const TArray<FTileIndex>& Tiles, ETileHighlightFlag Flag)
{
	// TODO: Flag 비트를 가진 기존 타일에서 끄고, Tiles에 켠 뒤 custom data 갱신
}

void ATileMap::ClearTileHighlight(ETileHighlightFlag Flag)
{
	// TODO: 모든 타일에서 Flag 비트를 끄고 custom data 갱신
}

bool ATileMap::CanPlace(const FTileIndex& TileIndex, const ITileActor* Incoming) const
{
	// 막히지 않으면 즉시 배치 가능
	if (IsBlocked(TileIndex, Incoming) == false)
		return true;

	// 막혔어도 막는 액터를 교체할 수 있으면 배치 가능
	return GetReplaceableActors(TileIndex, Incoming).Num() > 0;
}

bool ATileMap::IsBlocked(const FTileIndex& TileIndex, const ITileActor* Incoming) const
{
	auto* Tile = GetTile(TileIndex);
	// 맵 범위 밖은 블록된 것으로 간주
	if (Tile == nullptr)
	{
		return true;
	}

	// 타일에 블록 당하는 지 체크
	return Tile->IsBlocked(Incoming);
}

TArray<TScriptInterface<ITileActor>> ATileMap::GetReplaceableActors(const FTileIndex& TileIndex, const ITileActor* Incoming) const
{
	TArray<TScriptInterface<ITileActor>> Result;

	// 진입 액터가 없으면 교체 대상 없음
	if (Incoming == nullptr)
		return Result;

	// 타일 위 모든 액터를 받아서 교체 조건으로 거름
	for (const TScriptInterface<ITileActor>& Actor : GetActorsOnTile(TileIndex, ETileLayerFlag::All))
	{
		if (Actor == nullptr)
			continue;

		// 기존 액터의 레이어가 진입 액터의 레이어와 다르면 제외
		if (!EnumHasAnyFlags(Actor->GetTileLayerFlags(), Incoming->GetTileLayerFlags()))
			continue;

		// 기존 액터가 진입 액터의 교체를 허용하지 않으면 제외
		if (!EnumHasAnyFlags(Actor->GetReplaceLayerFlags(), Incoming->GetTileLayerFlags()))
			continue;

		// 진입 액터의 우선순위가 낮으면 제외
		if (Incoming->GetOverlayLayerPriority() < Actor->GetOverlayLayerPriority())
			continue;

		// 모든 조건을 충족하면 교체대상
		Result.Add(Actor);
	}
	return Result;
}

void ATileMap::StartActorMovement(const FTileTransform& NextTransform, ITileActor* Actor)
{
	checkf(Actor != nullptr, TEXT("Actor가 nullptr"));
	checkf(IsBlocked(NextTransform.mIndex, Actor) == false, TEXT("배치할 수 없는 타일"));

	// 이전 타일에서 이탈 오버랩 통지 후 해제 (좌표 전환 전에 현재 위치를 읽음)
	FTile* PrevTile = GetTile(Actor->GetTileTransform().mIndex);
	if (PrevTile != nullptr)
	{
		NotifyEndOverlap(PrevTile, Actor);
		UnregisterActorFromTile(PrevTile, Actor);
	}

	// 논리 좌표 전환 (점유는 즉시, 진입 오버랩 통지는 이동 연출 완료 후 CompleteActorMovement에서)
	Actor->SetTileTransform(NextTransform);
	FTile* NextTile = GetTile(NextTransform.mIndex);
	RegisterActorToTile(NextTile, Actor);
}

void ATileMap::CompleteActorMovement(ITileActor* Actor)
{
	checkf(Actor != nullptr, TEXT("Actor가 nullptr"));

	// 이동 연출 완료 후 도착 타일에서 진입 오버랩 통지 (Start에서 좌표는 이미 전환됨)
	FTile* Tile = GetTile(Actor->GetTileTransform().mIndex);
	if (Tile != nullptr)
	{
		NotifyBeginOverlap(Tile, Actor);
	}
}

void ATileMap::PlaceActor(const FTileTransform& NextTransform, ITileActor* Actor)
{
	checkf(Actor != nullptr, TEXT("Actor가 nullptr"));
	checkf(CanPlace(NextTransform.mIndex, Actor), TEXT("배치할 수 없는 타일"));

	FTile* Tile = GetTile(NextTransform.mIndex);

	// 교체 대상을 타일에서 밀어냄 (등록 해제 → 교체 통지 순서)
	for (const TScriptInterface<ITileActor>& Victim : GetReplaceableActors(NextTransform.mIndex, Actor))
	{
		RemoveActor(Victim.GetInterface());
		Victim->OnReplaced(Tile, Actor);
	}

	// 논리 좌표 갱신
	Actor->SetTileTransform(NextTransform);

	// 다음 타일에 등록 후 진입 오버랩 통지 (등록 → Begin 순서)
	RegisterActorToTile(Tile, Actor);
	NotifyBeginOverlap(Tile, Actor);
}

void ATileMap::RemoveActor(ITileActor* Actor)
{
	checkf(Actor != nullptr, TEXT("Actor가 nullptr"));

	// 현재 타일에서 이탈 오버랩 통지 후 해제 (End → 해제 순서)
	if (FTile* Tile = GetTile(Actor->GetTileTransform().mIndex))
	{
		NotifyEndOverlap(Tile, Actor);
		UnregisterActorFromTile(Tile, Actor);
	}

	// 논리 좌표 무효화
	Actor->SetTileTransform(FTileTransform::Invalid);
}

