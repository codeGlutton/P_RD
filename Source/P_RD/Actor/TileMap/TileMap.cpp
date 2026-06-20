#include "Actor/TileMap/TileMap.h"
#include "RDCollision.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Algo/Reverse.h"
#include "SRPGFramework/TileActor.h"

namespace
{
	/**
	 * @brief 타일 액터 방향을 yaw(도)로 변환 (Forward 0 / Right 90 / Backward 180 / Left 270)
	 */
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
	// Effect 하이라이트 펄스를 매 프레임 갱신하기 위해 틱 사용
	PrimaryActorTick.bCanEverTick = true;

	// 루트 컴포넌트 생성 및 지정
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 타일맵 데이터 모델 기본 인스턴스 생성 (에디터 프리뷰·널 안전, 런타임엔 MapModel로 교체)
	mModel = CreateDefaultSubobject<UTileMapModel>(TEXT("Model"));

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

	// 하이라이트 표시용 머티리얼: PerInstanceCustomData(RGBA)를 읽어 타일 색에 합성한다.
	// 없으면 엔진 기본 머티리얼로 렌더되어 하이라이트(Aim/Select/Effect)가 안 보인다.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TileHighlightMatFinder(TEXT("/Game/BP/Materials/M_TileHighlight.M_TileHighlight"));
	if (TileHighlightMatFinder.Succeeded())
	{
		mTileMaterial = TileHighlightMatFinder.Object;
		mTileMeshComponent->SetMaterial(0, mTileMaterial);
	}

	// 강조 스타일 기본값 (모두 타일 위에 자기 알파로 Mix — 알파<1이라 타일이 비침)
	// 조준 범위: 회색 반투명
	mAimStyle.mColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);

	// 선택 타일: 노란색 (겹치면 최우선)
	mSelectStyle.mColor = FLinearColor(1.0f, 0.9f, 0.1f, 0.8f);

	// 영향 범위: 빨간색 (아래 레이어 ↔ 자기 색을 펄스로 보간)
	mEffectStyle.mColor = FLinearColor(1.0f, 0.1f, 0.1f, 0.6f);
}

void ATileMap::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 배치/스폰/프로퍼티 변경 시점에 그리드 인스턴스 재생성
	RebuildTileInstances();
}

void ATileMap::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Effect 하이라이트는 알파가 시간에 따라 진동(펄스)하므로, 매 프레임 재합성
	// Effect 플래그를 가진 타일만 갱신 (나머지는 정적이라 Set/Clear 때만 갱신됨)
	for (int32 Index = 0; Index < mHighlights.Num(); ++Index)
	{
		if (EnumHasAnyFlags(mHighlights[Index], ETileHighlightFlag::Effect))
			RefreshTileCustomData(Index);
	}
}

void ATileMap::RebuildTileInstances()
{
	// 타일 저장소를 현재 크기에 맞춰 기본 타일로 새로 채움 (모델 데이터 책임)
	mModel->RebuildTiles();

	// 강조 표시 상태도 같은 크기로 초기화 (전부 None)
	mHighlights.Init(ETileHighlightFlag::None, FMath::Max(0, mModel->GetWidth() * mModel->GetHeight()));

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

	// 타일별 하이라이트 RGBA를 담을 custom data 슬롯 4개 (0=R,1=G,2=B,3=A)
	mTileMeshComponent->SetNumCustomDataFloats(4);

	// 기존 인스턴스 모두 제거 후 재생성
	mTileMeshComponent->ClearInstances();

	// 메시가 없거나 크기가 비정상이면 빈 그리드로 둠
	if (mTileMesh == nullptr || mModel->GetWidth() <= 0 || mModel->GetHeight() <= 0 || mTileSize <= 0.0f)
	{
		return;
	}

	// 엔진 Plane은 100cm 기준 → 타일 크기에 맞춰 스케일 환산, 시각 비율로 칸 사이 틈 생성
	const float PlaneBaseSize = 100.0f;
	const float InstanceScaleXY = (mTileSize / PlaneBaseSize) * mTileVisualScale;

	// Width x Height 만큼 타일 인스턴스를 로컬 공간에 배치
	for (int32 y = 0; y < mModel->GetHeight(); ++y)
	{
		for (int32 x = 0; x < mModel->GetWidth(); ++x)
		{
			// 타일 (x,y) 중심의 로컬 위치
			const FVector LocalLocation(x * mTileSize, y * mTileSize, 0.0f);
			const FTransform InstanceTransform(FRotator::ZeroRotator, LocalLocation, FVector(InstanceScaleXY, InstanceScaleXY, 1.0f));
			mTileMeshComponent->AddInstance(InstanceTransform, /*bWorldSpace=*/false);
		}
	}
}

float ATileMap::GetTileSize() const
{
	return mTileSize;
}

FTransform ATileMap::TileToWorldTransform(const FTileTransform& TileTransform) const
{
	// 월드 위치 획득
	const FVector WorldLocation = TileToWorldLocation(TileTransform.mIndex);
	// 타일맵 메시 컴포넌트의 YAW 회전을 타일에 적용 (그래야 같은 방향을 바라보니까)
	const FQuat WorldRotation = mTileMeshComponent->GetComponentQuat() * FRotator(0.0f, DirectionToYaw(TileTransform.mDirection), 0.0f).Quaternion();
	// 타일맵 메시 컴포넌트의 스케일을 타일에 적용
	return FTransform(WorldRotation, WorldLocation, mTileMeshComponent->GetComponentScale());
}

FVector ATileMap::TileToWorldLocation(const FTileIndex& TileIndex) const
{
	// 타일 (x,y) 중심의 로컬 위치 (RebuildTileInstances의 인스턴스 배치식과 동일)
	// 메시 피벗이 중심인 엔진 Plane 기준이라 (X*TileSize, Y*TileSize)가 곧 타일 중심
	// (피벗이 모서리인 커스텀 메시로 교체 시 이 가정이 깨지므로 양쪽 모두 보정 필요)
	const FVector LocalLocation(TileIndex.mX * mTileSize, TileIndex.mY * mTileSize, 0.0f);
	// 타일이 배치된 메시 컴포넌트 트랜스폼(위치/회전/스케일)을 반영해 월드 위치로 변환
	return mTileMeshComponent->GetComponentTransform().TransformPosition(LocalLocation);
}

FTileIndex ATileMap::WorldToTileIndex(const FVector& WorldLocation) const
{
	// 타일 크기가 설정되지 않으면 계산 불가 (0으로 나눗셈 방지)
	if (mTileSize <= 0.0f)
	{
		return FTileIndex::Invalid;
	}

	// 월드 좌표를 타일이 배치된 메시 컴포넌트 로컬 좌표로 변환
	const FVector LocalLocation = mTileMeshComponent->GetComponentTransform().InverseTransformPosition(WorldLocation);

	// 로컬 좌표를 타일 크기로 나눈 뒤 반올림해 가장 가까운 타일 중심을 인덱스로 지정
	// 예) 80 -> 80 / 100 = 0 -> 인덱스 0
	// 예) 110 -> 110 / 100 = 1.1 -> 반올림(1.1) = 1 -> 인덱스 1
	const FTileIndex TileIndex(
		FMath::RoundToInt(LocalLocation.X / mTileSize),
		FMath::RoundToInt(LocalLocation.Y / mTileSize)
	);

	// 맵 범위 밖이면 Invalid 반환
	return mModel->IsValidIndex(TileIndex) ? TileIndex : FTileIndex::Invalid;
}

/**
 * @details
 * - 모든 강조는 타일 위에 자기 알파로 Mix (프리멀티플라이드 RGB + 커버리지 알파)
 * - Select가 겹치면 최우선: 자기 색만 칠하고 Aim/Effect 무시
 * - Effect는 [아래 레이어(Aim/타일)] ↔ [자기 색]을 펄스로 크로스페이드
 * - 알파까지 계산에 포함시켜서, 출력단에서 알파 합성을 따로 안해도 되게끔 최적화
 */
void ATileMap::RefreshTileCustomData(int32 LinearIndex)
{
	// 컴포넌트/인덱스 유효성 (인스턴스 인덱스 = 타일 1D 인덱스)
	if (mTileMeshComponent == nullptr || !mHighlights.IsValidIndex(LinearIndex))
		return;

	const ETileHighlightFlag Flags = mHighlights[LinearIndex];

	// 플래그 해석 (Select가 겹치면 최우선 → Effect 억제)
	const bool bHasAim    = EnumHasAnyFlags(Flags, ETileHighlightFlag::Aim);
	const bool bHasSelect = EnumHasAnyFlags(Flags, ETileHighlightFlag::Select);
	const bool bHasEffect = EnumHasAnyFlags(Flags, ETileHighlightFlag::Effect) && !bHasSelect;

	// 스타일 색을 프리멀티플라이드(알파 곱한 RGB + 커버리지 알파)로 변환 — 타일 위 Mix용
	auto Premultiply = [](const FTileHighlightStyle& Style)
	{
		const FLinearColor& C = Style.mColor;
		return FLinearColor(C.R * C.A, C.G * C.A, C.B * C.A, C.A);
	};

	// 최종색 (기본=타일만 보이는 투명)
	FLinearColor Accum(0.0f, 0.0f, 0.0f, 0.0f);

	if (bHasSelect)
	{
		// 최우선: 선택 색만 칠함 (Aim/Effect 무시)
		Accum = Premultiply(mSelectStyle);
	}
	else if (bHasEffect)
	{
		// 펄스: [아래 레이어 표시] ↔ [Effect 자기 색] 크로스페이드 (둘 다 타일 위)
		// 저점 = Aim 있으면 Aim 색, 없으면 타일(투명) / 고점 = Effect 색
		const FLinearColor Low  = bHasAim ? Premultiply(mAimStyle) : FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		const FLinearColor High = Premultiply(mEffectStyle);

		// 펄스 파동(0~1)으로 저점↔고점 보간
		const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		const float PulseWave = 0.5f - 0.5f * FMath::Cos(2.0f * PI * Time / mPulsePeriod);
		Accum = FMath::Lerp(Low, High, PulseWave);
	}
	else if (bHasAim)
	{
		// Aim만: 자기 색
		Accum = Premultiply(mAimStyle);
	}

	// custom data 슬롯에 기록 (마지막 슬롯에서 렌더 상태 갱신)
	mTileMeshComponent->SetCustomDataValue(LinearIndex, 0, Accum.R);
	mTileMeshComponent->SetCustomDataValue(LinearIndex, 1, Accum.G);
	mTileMeshComponent->SetCustomDataValue(LinearIndex, 2, Accum.B);
	mTileMeshComponent->SetCustomDataValue(LinearIndex, 3, Accum.A, /*bMarkRenderStateDirty=*/true);
}

void ATileMap::SetTileHighlight(const TArray<FTileIndex>& Tiles, ETileHighlightFlag Flag)
{
	if (Flag == ETileHighlightFlag::None)
		return;

	// 이 레이어를 새로 칠하면 무효가 되는 하위(더 구체적) 의존 레이어
	// 드릴다운: Aim → Select → Effect (상위를 새로 하면 하위는 무효, 상위는 맥락으로 보존)
	ETileHighlightFlag Dependents = ETileHighlightFlag::None;
	if (EnumHasAnyFlags(Flag, ETileHighlightFlag::Aim))
		Dependents |= ETileHighlightFlag::Select | ETileHighlightFlag::Effect;
	if (EnumHasAnyFlags(Flag, ETileHighlightFlag::Select))
		Dependents |= ETileHighlightFlag::Effect;

	// 모든 타일에서 끌 비트 = Flag 자신 + 하위 의존 레이어
	const ETileHighlightFlag ClearBits = Flag | Dependents;

	// 새로 켤 타일 집합 (맵 밖 좌표는 무시)
	TSet<int32> NewOn;
	for (const FTileIndex& Tile : Tiles)
	{
		const int32 Index = mModel->TileIndexToLinearIndex(Tile);
		if (Index != INDEX_NONE)
			NewOn.Add(Index);
	}

	// 전체 타일 순회: ClearBits 끄고, 지정 타일이면 Flag 켜기
	for (int32 Index = 0; Index < mHighlights.Num(); ++Index)
	{
		const ETileHighlightFlag Before = mHighlights[Index];

		// 이 Flag와 하위 의존 레이어 비트를 끔 (상위/무관 비트는 보존)
		ETileHighlightFlag After = Before & ~ClearBits;

		// 지정된 타일이면 Flag 비트를 켬
		if (NewOn.Contains(Index))
			After |= Flag;

		// 바뀐 타일만 custom data 갱신
		if (After != Before)
		{
			mHighlights[Index] = After;
			RefreshTileCustomData(Index);
		}
	}
}

void ATileMap::ClearTileHighlight(ETileHighlightFlag Flag)
{
	if (Flag == ETileHighlightFlag::None)
		return;

	// 전체 타일에서 지정 비트만 끔 (캐스케이드 없음, 무관 비트는 보존)
	for (int32 Index = 0; Index < mHighlights.Num(); ++Index)
	{
		const ETileHighlightFlag Before = mHighlights[Index];
		const ETileHighlightFlag After = Before & ~Flag;

		// 바뀐 타일만 custom data 갱신
		if (After != Before)
		{
			mHighlights[Index] = After;
			RefreshTileCustomData(Index);
		}
	}
}

/* 옛 ITileActor 인터페이스 호환 (전환기 — 호출부 이관 후 제거) */

TArray<FTileIndex> ATileMap::GetAimableTiles(const FTileIndex& Origin, int32 Range, EAimPattern Pattern, bool bIncludeOccupied, bool bIndirect, const ITileActor* Incoming) const
{
	// 모델로 위임 (Incoming은 모델 액터로 매핑 불가라 무시)
	return mModel != nullptr
		? mModel->GetAimableTiles(Origin, Range, Pattern, bIncludeOccupied, bIndirect, nullptr)
		: TArray<FTileIndex>();
}

TArray<FTileIndex> ATileMap::GetEffectTiles(const FTileIndex& Caster, const FTileIndex& Target, EEffectPattern Pattern, int32 Size, bool bPenetrate) const
{
	// 모델로 위임
	return mModel != nullptr
		? mModel->GetEffectTiles(Caster, Target, Pattern, Size, bPenetrate)
		: TArray<FTileIndex>();
}

TArray<FTileIndex> ATileMap::GetReachableTiles(const FTileIndex& Origin, int32 MoveDistance) const
{
	// 모델로 위임 (BFS 경로 기반 도달성)
	return mModel != nullptr
		? mModel->GetReachableTiles(Origin, MoveDistance)
		: TArray<FTileIndex>();
}

bool ATileMap::CanPlace(const FTileIndex& TileIndex, const ITileActor* Incoming) const
{
	// 스텁: ITileActor를 모델 UBoardActorModel로 매핑하기 전까지 배치 가능으로 간주
	return true;
}

void ATileMap::PlaceActor(const FTileTransform& NextTransform, ITileActor* Actor)
{
	// 스텁: ITileActor↔모델 매핑 전까지 동작 없음
}

TArray<TScriptInterface<ITileActor>> ATileMap::GetActorsOnTile(const FTileIndex& TileIndex, ETileLayerFlag LayerFilter) const
{
	// 스텁: ITileActor↔모델 매핑 전까지 빈 목록
	return TArray<TScriptInterface<ITileActor>>();
}

bool ATileMap::IsValidIndex(const FTileIndex& TileIndex) const
{
	// 모델로 위임
	return mModel != nullptr && mModel->IsValidIndex(TileIndex);
}

void ATileMap::StartActorMovement(const FTileTransform& NextTransform, ITileActor* Actor)
{
	// 스텁: ITileActor↔모델 매핑 전까지 동작 없음
}

void ATileMap::CompleteActorMovement(ITileActor* Actor)
{
	// 스텁: ITileActor↔모델 매핑 전까지 동작 없음
}

#if WITH_EDITOR
void ATileMap::DebugPaintTest()
{
	// 기존 하이라이트 초기화
	ClearTileHighlight(ETileHighlightFlag::Aim | ETileHighlightFlag::Select | ETileHighlightFlag::Effect);

	// 조준 범위 (가로 한 줄)
	SetTileHighlight({ FTileIndex(1, 1), FTileIndex(2, 1), FTileIndex(3, 1), FTileIndex(4, 1) }, ETileHighlightFlag::Aim);

	// 선택 타일 (Aim 위에 겹침)
	SetTileHighlight({ FTileIndex(3, 1) }, ETileHighlightFlag::Select);

	// 영향 범위 (일부는 Aim/Select와 겹침, (3,2)는 Effect 단독)
	SetTileHighlight({ FTileIndex(3, 1), FTileIndex(3, 2), FTileIndex(4, 1) }, ETileHighlightFlag::Effect);
}
#endif

