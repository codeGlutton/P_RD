#include "Actor/TileMap/TileMap.h"
#include "RDCollision.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Algo/Reverse.h"

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
	// M_TileTransparent는 반투명이라 타일맵이 배경에 깔릴 때 바닥이 비치고, 남색 바탕이 없어 하이라이트 색 구분이 잘 된다.
	// 없으면 엔진 기본 머티리얼로 렌더되어 하이라이트(Aim/Select/Effect)가 안 보인다.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TileTransparentMatFinder(TEXT("/Game/SVN/InSideAsset/Material/M_TileTransparent.M_TileTransparent"));
	if (TileTransparentMatFinder.Succeeded())
	{
		mTileMaterial = TileTransparentMatFinder.Object;
		mTileMeshComponent->SetMaterial(0, mTileMaterial);
	}

	// 강조 스타일 기본값 (모두 타일 위에 자기 알파로 Mix — 알파<1이라 타일이 비침)
	// 조준 범위: 회색 반투명
	mAimStyle.mColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);

	// 선택 타일: 노란색 (겹치면 최우선)
	mSelectStyle.mColor = FLinearColor(1.0f, 0.9f, 0.1f, 0.8f);

	// 영향 범위: 빨간색 (아래 레이어 ↔ 자기 색을 펄스로 보간)
	mEffectStyle.mColor = FLinearColor(1.0f, 0.1f, 0.1f, 0.6f);

	// 기본 타일 구분색
	mTileBaseStyle.mColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.25f);

	// 테두리 기본값: 짙은 회색 (머티리얼에 BorderColor/BorderWidth 파라미터가 있어야 표시됨)
	mTileBorderStyle.mColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.8f);

	// 경로 화살표/도착 마커 컴포넌트 생성 (타일 클릭 트레이스를 방해하지 않도록 콜리전 비활성화)
	mPathArrowComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PathArrow"));
	mPathArrowComponent->SetupAttachment(RootComponent);
	mPathArrowComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mPathArrowComponent->SetNumCustomDataFloats(4);

	// 경로 좌회전 화살표 컴포넌트 생성
	mPathTurnLeftComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PathTurnLeft"));
	mPathTurnLeftComponent->SetupAttachment(RootComponent);
	mPathTurnLeftComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mPathTurnLeftComponent->SetNumCustomDataFloats(4);

	// 경로 우회전 화살표 컴포넌트 생성
	mPathTurnRightComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PathTurnRight"));
	mPathTurnRightComponent->SetupAttachment(RootComponent);
	mPathTurnRightComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mPathTurnRightComponent->SetNumCustomDataFloats(4);

	mPathEndComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PathEnd"));
	mPathEndComponent->SetupAttachment(RootComponent);
	mPathEndComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mPathEndComponent->SetNumCustomDataFloats(4);

	// 경유지 마커, 도착지 원뿔 컴포넌트 생성 (타일 클릭 트레이스를 방해하지 않도록 콜리전 비활성화)
	mWaypointComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Waypoint"));
	mWaypointComponent->SetupAttachment(RootComponent);
	mWaypointComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mWaypointComponent->SetNumCustomDataFloats(4);

	mDestConeComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DestCone"));
	mDestConeComponent->SetupAttachment(RootComponent);
	mDestConeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mDestConeComponent->SetNumCustomDataFloats(4);

	// 경로 표시 컴포넌트들은 RF_Transient 플래그 설정
	// 런타임에 생성/설정되는 컴포넌트들이므로 굳이 파일로 저장할 필요가 없음
	mPathArrowComponent->SetFlags(RF_Transient);
	mPathTurnLeftComponent->SetFlags(RF_Transient);
	mPathTurnRightComponent->SetFlags(RF_Transient);
	mPathEndComponent->SetFlags(RF_Transient);
	mWaypointComponent->SetFlags(RF_Transient);
	mDestConeComponent->SetFlags(RF_Transient);

	// 경로 중간 화살표 기본 메시: +X를 가리키는 화살표 (방향 회전이 이 형상 기준이라 +X 향이어야 함)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ArrowMeshFinder(TEXT("/Game/SVN/OutSideAsset/Kenney/FactoryKit/SM_Kenney_FactoryKit_ArrowStraight.SM_Kenney_FactoryKit_ArrowStraight"));
	if (ArrowMeshFinder.Succeeded())
	{
		mPathArrowMesh = ArrowMeshFinder.Object;
		mPathArrowComponent->SetStaticMesh(mPathArrowMesh);
	}

	// 경로 좌회전 화살표 기본 메시 (+X로 진입해 -Y로 꺾이는 형상)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TurnLeftMeshFinder(TEXT("/Game/SVN/OutSideAsset/Kenney/FactoryKit/SM_Kenney_FactoryKit_ArrowTurnLeft.SM_Kenney_FactoryKit_ArrowTurnLeft"));
	if (TurnLeftMeshFinder.Succeeded())
	{
		mPathTurnLeftMesh = TurnLeftMeshFinder.Object;
		mPathTurnLeftComponent->SetStaticMesh(mPathTurnLeftMesh);
	}

	// 경로 우회전 화살표 기본 메시 (+X로 진입해 +Y로 꺾이는 형상)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TurnRightMeshFinder(TEXT("/Game/SVN/OutSideAsset/Kenney/FactoryKit/SM_Kenney_FactoryKit_ArrowTurnRight.SM_Kenney_FactoryKit_ArrowTurnRight"));
	if (TurnRightMeshFinder.Succeeded())
	{
		mPathTurnRightMesh = TurnRightMeshFinder.Object;
		mPathTurnRightComponent->SetStaticMesh(mPathTurnRightMesh);
	}

	// 도착(끝) 타일 마커 기본 메시: Kenney 특수 인디케이터 화살표 에셋
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EndMeshFinder(TEXT("/Game/SVN/OutSideAsset/Kenney/FactoryKit/SM_Kenney_FactoryKit_IndicatorSpecialArrow.SM_Kenney_FactoryKit_IndicatorSpecialArrow"));
	if (EndMeshFinder.Succeeded())
	{
		mPathEndMesh = EndMeshFinder.Object;
		mPathEndComponent->SetStaticMesh(mPathEndMesh);
	}

	// 경유지 마커 기본 메시: 엔진 기본 Cube 활용 (디지털 시계 숫자처럼 막대로 숫자 조립)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		mWaypointBarMesh = CubeMeshFinder.Object;
		mWaypointComponent->SetStaticMesh(mWaypointBarMesh);
	}

	// 도착지 원뿔 기본 메시: 엔진 기본 Cone 활용 (뒤집어서 역원뿔 모양이 위아래로 진동)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMeshFinder.Succeeded())
	{
		mDestConeMesh = ConeMeshFinder.Object;
		mDestConeComponent->SetStaticMesh(mDestConeMesh);
	}

	// 화살표/마커는 전용 발광 머티리얼(custom data RGBA + EmissiveBoost) 사용 — SetMovePath에서 컴포넌트에 적용
	// 타일 머티리얼(M_TileTransparent)과 분리해 타일 쪽 테두리 파라미터의 영향 없이 블룸으로 도드라지게 함
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PathIndicatorMatFinder(TEXT("/Game/SVN/InSideAsset/Material/M_PathIndicator.M_PathIndicator"));
	if (PathIndicatorMatFinder.Succeeded())
	{
		mPathArrowMaterial = PathIndicatorMatFinder.Object;
		mPathEndMaterial = PathIndicatorMatFinder.Object;
	}

	// 경로 화살표와 도착지 화살표 기본 값
	mPathArrowStyle.mColor = FLinearColor(0.9f, 0.45f, 0.0f, 0.95f);
	mPathEndStyle.mColor = FLinearColor(0.9f, 0.45f, 0.0f, 0.95f);

	// 경유지 마커 기본 색: 경로(주황)와 구분되는 파랑
	mWaypointStyle.mColor = FLinearColor(0.15f, 0.5f, 1.0f, 1.0f);

	// 도착지 원뿔 기본 색: 지도 핀을 연상시키는 빨강
	mDestConeStyle.mColor = FLinearColor(0.5f, 0.0f, 0.0f, 1.0f);

	// 생성자에서 만든 기본 모델에 표시 델리깃을 임시 바인딩 (런타임 모델 매핑 시 재호출)
	BindModelDelegates();
}

void ATileMap::BindModelDelegates()
{
	// 모델이 없으면 바인딩 불가
	if (mModel == nullptr)
		return;

	// 이동경로 표시 요청을 뷰의 SetMovePath로 연결 (싱글캐스트라 재호출 시 덮어씀)
	mModel->mSetMovePathDelegate.BindUObject(this, &ATileMap::SetMovePath);

	// 타일 강조 표시/해제 요청을 뷰의 SetTileHighlight/ClearTileHighlight로 연결
	mModel->mSetTileHighlightDelegate.BindUObject(this, &ATileMap::SetTileHighlight);
	mModel->mClearTileHighlightDelegate.BindUObject(this, &ATileMap::ClearTileHighlight);

	// 좌표 변환 질의를 뷰의 컴포넌트 트랜스폼 기반 함수로 연결 (모델이 시각 정보가 필요한 변환을 질의)
	mModel->mTileToWorldTransformDelegate.BindUObject(this, &ATileMap::TileToWorldTransform);
	mModel->mTileToWorldLocationDelegate.BindUObject(this, &ATileMap::TileToWorldLocation);
	mModel->mWorldToTileIndexDelegate.BindUObject(this, &ATileMap::WorldToTileIndex);
}

void ATileMap::BindModel(UObjectModel* Model)
{
	// 컴포넌트 뷰 바인딩 (기반 구현 — 현재 부착 컴포넌트 뷰는 없지만 일관성 유지)
	IActorView::BindModel(Model);

	// 런타임 모델을 뷰의 모델로 교체
	mModel = Cast<UTileMapModel>(Model);

	// 표시·좌표 델리깃을 교체된 모델에 재바인딩
	BindModelDelegates();

	// 모델 크기에 맞춰 그리드 인스턴스 재생성
	RebuildTileInstances();
}

void ATileMap::UnbindModel(UObjectModel* Model)
{
	// 컴포넌트 뷰 해제 (기반 구현)
	IActorView::UnbindModel(Model);

	// 교체된 모델의 델리깃 정리 (싱글캐스트라 명시적으로 비움)
	if (mModel != nullptr)
	{
		mModel->mSetMovePathDelegate.Unbind();
		mModel->mSetTileHighlightDelegate.Unbind();
		mModel->mClearTileHighlightDelegate.Unbind();
		mModel->mTileToWorldTransformDelegate.Unbind();
		mModel->mTileToWorldLocationDelegate.Unbind();
		mModel->mWorldToTileIndexDelegate.Unbind();
	}
}

UObjectModel* ATileMap::GetModel_Internal() const
{
	// IObjectView::GetModel 템플릿이 참조하는 보유 모델 반환
	return mModel;
}

void ATileMap::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 게임 월드(PIE/런타임)에선 프로퍼티 편집/이동이 컨스트럭션을 재실행하는데,
	// 이때 저장소(mTiles=배치된 유닛 등록)를 날리지 않도록 그리드 시각만 갱신 (저장소 빌드는 BindModel이 담당)
	// 에디터(비게임)에선 크기/그리드 편집 반영을 위해 저장소까지 재생성
	if (GetWorld() != nullptr && GetWorld()->IsGameWorld())
	{
		RefreshTileVisuals();
	}
	else
	{
		RebuildTileInstances();
	}

#if WITH_EDITOR
	// [에디터 전용] 에디터 뷰포트에서 경유지 디버그 경로 미리보기 (진동, 펄스 애니메이션은 틱이 도는 PIE에서만)
	if (GetWorld() != nullptr && !GetWorld()->IsGameWorld())
	{
		if (mDebugDrawWaypointPath)
			DebugWaypointTest();
		else
			ClearMovePath();
	}
#endif
}

void ATileMap::BeginPlay()
{
	Super::BeginPlay();

	// 타일맵뷰의 트랜스폼이 변경되면 유닛뷰들의 위치도 재조정하기 위해 이벤트 구독
	if (RootComponent != nullptr)
	{
		RootComponent->TransformUpdated.AddUObject(this, &ATileMap::OnRootTransformUpdated);
	}

#if WITH_EDITOR
	// [에디터 전용] 토글이 켜진 인스턴스에서만 PIE 시작 시 경유지 디버그 경로를 그려 진동, 펄스 검증 (패키징 빌드에선 제거됨)
	if (mDebugDrawWaypointPath)
		DebugWaypointTest();
#endif
}

#if WITH_EDITOR
void ATileMap::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 구조체 내부(mColor 등) 편집도 소유 멤버 이름으로 판별
	const FName MemberName = PropertyChangedEvent.MemberProperty != nullptr ?
		PropertyChangedEvent.MemberProperty->GetFName() : PropertyChangedEvent.GetPropertyName();

	// 배치 계열: 그리드 인스턴스 재생성 + 배치된 액터 위치 재통지 (크기가 바뀌면 테두리 cm→UV 환산도 함께 갱신됨)
	if (MemberName == GET_MEMBER_NAME_CHECKED(ATileMap, mTileSize) ||
		MemberName == GET_MEMBER_NAME_CHECKED(ATileMap, mTileVisualScale) ||
		MemberName == GET_MEMBER_NAME_CHECKED(ATileMap, mTileMesh) ||
		MemberName == GET_MEMBER_NAME_CHECKED(ATileMap, mTileMaterial))
	{
		RefreshTileVisuals();
		if (mModel != nullptr)
		{
			mModel->RefreshActorPlacements();
		}
	}
	// 색 계열: 전 타일 표시만 재합성 (배치 불변)
	else if (MemberName == GET_MEMBER_NAME_CHECKED(ATileMap, mTileBaseStyle) ||
		MemberName == GET_MEMBER_NAME_CHECKED(ATileMap, mAimStyle) ||
		MemberName == GET_MEMBER_NAME_CHECKED(ATileMap, mSelectStyle) ||
		MemberName == GET_MEMBER_NAME_CHECKED(ATileMap, mEffectStyle))
	{
		for (int32 Index = 0; Index < mHighlights.Num(); ++Index)
		{
			RefreshTileCustomData(Index);
		}
	}
	// 테두리 계열: MID 파라미터만 갱신
	else if (MemberName == GET_MEMBER_NAME_CHECKED(ATileMap, mTileBorderStyle) ||
		MemberName == GET_MEMBER_NAME_CHECKED(ATileMap, mTileBorderWidth))
	{
		ApplyBorderParameters();
	}
}
#endif

void ATileMap::OnRootTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	// 타일맵뷰 트랜스폼이 변경되면 유닛뷰들의 위치도 조정
	if (mModel != nullptr)
	{
		mModel->RefreshActorPlacements();
	}
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

	// 경로 화살표/도착 마커 알파도 펄스 — 표시 중일 때만 갱신
	if (mPathLength > 0)
		RefreshPathPulse();
}

void ATileMap::RebuildTileInstances()
{
	// 타일 저장소를 현재 크기에 맞춰 기본 타일로 새로 채움 (모델 데이터 책임)
	mModel->RebuildTiles();

	// 강조 표시 상태도 같은 크기로 초기화 (전부 None)
	mHighlights.Init(ETileHighlightFlag::None, FMath::Max(0, mModel->GetWidth() * mModel->GetHeight()));

	// 시각 요소(인스턴스/머티리얼/타일 색) 재생성
	RefreshTileVisuals();
}

void ATileMap::RefreshTileVisuals()
{
	// 컴포넌트가 없으면 처리 불가
	if (mTileMeshComponent == nullptr)
	{
		return;
	}

	// 에디터에서 교체된 메시 반영
	mTileMeshComponent->SetStaticMesh(mTileMesh);

	// 테두리 파라미터를 원본 에셋 수정 없이 푸시하기 위한 타일 전용 MID 생성 후 적용
	if (mTileMaterial != nullptr)
	{
		mTileMID = UMaterialInstanceDynamic::Create(mTileMaterial, this);
		mTileMeshComponent->SetMaterial(0, mTileMID);
	}
	ApplyBorderParameters();

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

	// 새 인스턴스에 기본 구분색과 하이라이트를 다시 칠함
	for (int32 Index = 0; Index < mHighlights.Num(); ++Index)
	{
		RefreshTileCustomData(Index);
	}
}

void ATileMap::ApplyBorderParameters()
{
	// 테두리 정보가 없으면 바로 리턴
	if (mTileMID == nullptr)
	{
		return;
	}

	// 타일은 mTileVisualScale만큼 축소/확대 되므로
	// cm 단위로 설정한 테두리 굵기도 실제 그려지는 타일판 크기에 맞춰 재계산
	const float TileVisualSize = mTileSize * mTileVisualScale;
	const float BorderWidthUV = TileVisualSize > 0.0f ? FMath::Clamp(mTileBorderWidth / TileVisualSize, 0.0f, 0.5f) : 0.0f;

	mTileMID->SetVectorParameterValue(TEXT("BorderColor"), mTileBorderStyle.mColor);
	mTileMID->SetScalarParameterValue(TEXT("BorderWidth"), BorderWidthUV);
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

FRotator ATileMap::TileToWorldRotation(ETileActorDirection Direction) const
{
	// 타일맵 메시 컴포넌트의 YAW 회전을 타일에 적용
	const FQuat WorldRotation = mTileMeshComponent->GetComponentQuat() * FRotator(0.0f, DirectionToYaw(Direction), 0.0f).Quaternion();
	return WorldRotation.Rotator();
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

	// 최종색 (기본=머티리얼 바탕 위에 기본 구분색만 얹은 상태)
	FLinearColor Accum = Premultiply(mTileBaseStyle);

	if (bHasSelect)
	{
		// 최우선: 선택 색만 칠함 (Aim/Effect 무시)
		Accum = Premultiply(mSelectStyle);
	}
	else if (bHasEffect)
	{
		// 펄스: [아래 레이어 표시] ↔ [Effect 자기 색] 크로스페이드 (둘 다 타일 위)
		// 저점 = Aim 있으면 Aim 색, 없으면 기본 구분색 / 고점 = Effect 색
		const FLinearColor Low  = bHasAim ? Premultiply(mAimStyle) : Premultiply(mTileBaseStyle);
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

/* 좌표 유효성 (모델 위임) */

bool ATileMap::IsValidIndex(const FTileIndex& TileIndex) const
{
	// 모델로 위임
	return mModel != nullptr && mModel->IsValidIndex(TileIndex);
}

float ATileMap::StepToYaw(const FTileIndex& Step)
{
	// +X 기준 방향 스텝을 yaw로 (atan2는 라디안 → 도). (1,0)=0°, (0,1)=90°, (-1,0)=180°, (0,-1)=-90°
	return FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Step.mY), static_cast<float>(Step.mX)));
}

void ATileMap::SetMovePath(const TArray<FMovePathTile>& PathTiles)
{
	// 기존 표시 제거 후 다시 그림
	ClearMovePath();

	// 경로가 비었으면 표시할 것 없음 (해제와 동일)
	if (PathTiles.Num() == 0)
		return;

	// 에디터에서 교체된 메시/머티리얼 반영
	if (mPathArrowComponent != nullptr)
	{
		mPathArrowComponent->SetStaticMesh(mPathArrowMesh);
		if (mPathArrowMaterial != nullptr)
			mPathArrowComponent->SetMaterial(0, mPathArrowMaterial);
	}
	if (mPathTurnLeftComponent != nullptr)
	{
		mPathTurnLeftComponent->SetStaticMesh(mPathTurnLeftMesh);
		if (mPathArrowMaterial != nullptr)
			mPathTurnLeftComponent->SetMaterial(0, mPathArrowMaterial);
	}
	if (mPathTurnRightComponent != nullptr)
	{
		mPathTurnRightComponent->SetStaticMesh(mPathTurnRightMesh);
		if (mPathArrowMaterial != nullptr)
			mPathTurnRightComponent->SetMaterial(0, mPathArrowMaterial);
	}
	if (mPathEndComponent != nullptr)
	{
		mPathEndComponent->SetStaticMesh(mPathEndMesh);
		if (mPathEndMaterial != nullptr)
			mPathEndComponent->SetMaterial(0, mPathEndMaterial);
	}

	// 경유지 마커: 화살표와 같은 발광 머티리얼 공유
	if (mWaypointComponent != nullptr)
	{
		mWaypointComponent->SetStaticMesh(mWaypointBarMesh);
		if (mPathArrowMaterial != nullptr)
			mWaypointComponent->SetMaterial(0, mPathArrowMaterial);
	}

	// 도착지 원뿔: 도착 마커와 같은 발광 머티리얼 공유
	if (mDestConeComponent != nullptr)
	{
		mDestConeComponent->SetStaticMesh(mDestConeMesh);
		if (mPathEndMaterial != nullptr)
			mDestConeComponent->SetMaterial(0, mPathEndMaterial);
	}

	// 경유지 타일 집합
	// 경유지가 일반화살표와 겹치지 않게 미리 집합을 만들어서 비교할 때 활용
	TSet<FTileIndex> WaypointTiles;
	for (const FMovePathTile& PathTile : PathTiles)
	{
		if (PathTile.mIsWaypoint)
			WaypointTiles.Add(PathTile.mIndex);
	}

	// 화살표/마커 균일 스케일 (타일 크기에 맞춤)
	const float ArrowScale = (mTileSize / 100.0f) * mPathArrowScale;

	// 경유지 순번 (등장 순서대로 1부터 부여)
	int32 WaypointNumber = 0;

	// 마지막을 제외한 각 타일에 화살표 배치
	const int32 LastIndex = PathTiles.Num() - 1;
	for (int32 Index = 0; Index < LastIndex; ++Index)
	{
		const FTileIndex& Tile = PathTiles[Index].mIndex;
		const FTileIndex& Next = PathTiles[Index + 1].mIndex;

		// 경유지 타일이면 화살표 대신 순번 마커 조립
		if (PathTiles[Index].mIsWaypoint)
		{
			AppendWaypointMarker(Tile, ++WaypointNumber);
			continue;
		}

		// 경유지 타일을 다시 지나가는 경우도 화살표 생략 (마커 우선)
		if (WaypointTiles.Contains(Tile))
			continue;

		// 진출 방향 스텝
		const FTileIndex OutStep(Next.mX - Tile.mX, Next.mY - Tile.mY);

		// 기본은 직진 화살표
		UInstancedStaticMeshComponent* Component = mPathArrowComponent;
		TArray<int32>* Orders = &mPathArrowOrders;
		float Yaw = StepToYaw(OutStep);

		if (Index > 0)
		{
			// 진입/진출 방향의 벡터 외적 부호로 회전 판별
            // 0: 직진
		    // 양수: 우회전
		    // 음수: 좌회전
			const FTileIndex& Prev = PathTiles[Index - 1].mIndex;
			const FTileIndex InStep(Tile.mX - Prev.mX, Tile.mY - Prev.mY);
			const int32 Cross = InStep.mX * OutStep.mY - InStep.mY * OutStep.mX;

			// 회전 타일이면 회전 화살표로 교체
		    // @note 회전 화살표 메시가 없으면 직진 화살표 사용 (회전 화살표 메시만 없는 경우는 없겠지만...)
			if (Cross > 0 && mPathTurnRightMesh != nullptr)
			{
				Component = mPathTurnRightComponent;
				Orders = &mPathTurnRightOrders;
				Yaw = StepToYaw(InStep);
			}
			else if (Cross < 0 && mPathTurnLeftMesh != nullptr)
			{
				Component = mPathTurnLeftComponent;
				Orders = &mPathTurnLeftOrders;
				Yaw = StepToYaw(InStep);
			}
		}

		if (Component == nullptr)
			continue;

		// 타일 중심 로컬 위치 + Z 오프셋에 배치 (바닥에서 아주 살짝 뜬 위치, Z축은 별도 스케일로 납작하게)
		const FVector Location(Tile.mX * mTileSize, Tile.mY * mTileSize, mPathHeightOffset);
		const FTransform InstanceTransform(FRotator(0.0f, Yaw, 0.0f), Location, FVector(ArrowScale, ArrowScale, ArrowScale * mPathArrowZScale));
		Component->AddInstance(InstanceTransform, /*bWorldSpace=*/false);

		// 펄스 위상용 경로 순번 기록 (이 순번과 시간 정보로 파도타기 응원 가능)
		Orders->Add(Index);
	}

	// 마지막(도착) 타일엔 도착 마커 배치 (인디케이터 메시가 방향성이 있어 진입 방향으로 회전)
	if (mPathEndComponent != nullptr)
	{
		const FTileIndex& EndTile = PathTiles[LastIndex].mIndex;

		// 진입 방향 = 마지막 스텝(직전 타일 → 도착 타일), 마지막 화살표와 같은 방향을 가리킴
		// 단일 타일 경로(직전 타일 없음)면 진입 방향이 없어 회전 없음(+X)
		FRotator Rotation = FRotator::ZeroRotator;
		if (LastIndex >= 1)
		{
			const FTileIndex& PrevTile = PathTiles[LastIndex - 1].mIndex;
			const FTileIndex Step(EndTile.mX - PrevTile.mX, EndTile.mY - PrevTile.mY);
			Rotation = FRotator(0.0f, StepToYaw(Step), 0.0f);
		}

		const FVector Location(EndTile.mX * mTileSize, EndTile.mY * mTileSize, mPathHeightOffset);
		const FTransform InstanceTransform(Rotation, Location, FVector(ArrowScale));
		mPathEndComponent->AddInstance(InstanceTransform, /*bWorldSpace=*/false);
	}

	// 도착지 원뿔 배치 (뒤집어서 꼭짓점이 도착 타일을 가리킴, 위아래 진동은 틱에서 처리)
	if (mDestConeComponent != nullptr)
	{
		const FTileIndex& EndTile = PathTiles[LastIndex].mIndex;
		const float ConeScale = (mTileSize / 100.0f) * mDestConeScale;
		const FRotator ConeRotation(180.0f, 0.0f, 0.0f);
		const FVector ConeLocation(EndTile.mX * mTileSize, EndTile.mY * mTileSize, GetDestConeBaseZ());
		mDestConeComponent->AddInstance(FTransform(ConeRotation, ConeLocation, FVector(ConeScale)), /*bWorldSpace=*/false);

		// 원뿔 색 설정
		mDestConeComponent->SetCustomDataValue(0, 0, mDestConeStyle.mColor.R);
		mDestConeComponent->SetCustomDataValue(0, 1, mDestConeStyle.mColor.G);
		mDestConeComponent->SetCustomDataValue(0, 2, mDestConeStyle.mColor.B);
		mDestConeComponent->SetCustomDataValue(0, 3, 1.0f, /*bMarkRenderStateDirty=*/true);
	}

	// 표시 중인 경로 길이 기록 (틱 펄스 대상 판단 + 도착 마커 위상 인덱스)
	mPathLength = PathTiles.Num();

	// 최초 1회 펄스 색 기록 (이후 틱마다 자동 갱신)
	RefreshPathPulse();
}

void ATileMap::ClearMovePath()
{
	// 화살표, 마커, 원뿔 인스턴스 모두 제거
	if (mPathArrowComponent != nullptr)
		mPathArrowComponent->ClearInstances();
	if (mPathTurnLeftComponent != nullptr)
		mPathTurnLeftComponent->ClearInstances();
	if (mPathTurnRightComponent != nullptr)
		mPathTurnRightComponent->ClearInstances();
	if (mPathEndComponent != nullptr)
		mPathEndComponent->ClearInstances();
	if (mWaypointComponent != nullptr)
		mWaypointComponent->ClearInstances();
	if (mDestConeComponent != nullptr)
		mDestConeComponent->ClearInstances();

	// 경로 순번 기록 제거
	mPathArrowOrders.Reset();
	mPathTurnLeftOrders.Reset();
	mPathTurnRightOrders.Reset();

	// 표시 중 경로 없음
	mPathLength = 0;
}

/**
 * @details
 * - 색에 밝기를 곱해서 최종 색을 계산
 * - 밝기만 하한<->상한 사이를 오가고, 화살표는 순서대로 위상차를 줘서 흐르는 느낌이 들게 처리
 */
void ATileMap::RefreshPathPulse()
{
	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	// 칸당 위상 밀림(라디안) — 경로 전체에 고점 mPathFlowCycles개가 흐르도록 길이로 정규화 (길이 무관 일정한 흐름)
	// Span은 시작~도착 타일 간격(mPathLength-1) 기준이라 화살표와 도착 마커가 같은 파동을 공유함
	const int32 Span = FMath::Max(1, mPathLength - 1);
	const float PhasePerTile = 2.0f * PI * mPathFlowCycles / Span;

	// 한 인스턴스에 펄스 색을 기록하는 헬퍼
	auto WriteInstance = [this](UInstancedStaticMeshComponent* Component, int32 InstanceIndex, const FLinearColor& Color, float Wave)
	{
		// 펄스를 상한/하한 값으로 변환해서 색에 곱하면 최종 색이 나옴
		const float Brightness = FMath::Lerp(mPathPulseMinBrightness, mPathPulseMaxBrightness, Wave);
		Component->SetCustomDataValue(InstanceIndex, 0, Color.R * Brightness);
		Component->SetCustomDataValue(InstanceIndex, 1, Color.G * Brightness);
		Component->SetCustomDataValue(InstanceIndex, 2, Color.B * Brightness);
		Component->SetCustomDataValue(InstanceIndex, 3, 1.0f, /*bMarkRenderStateDirty=*/true);
	};

	// 화살표: 경로 순번마다 위상차를 줘 흐르게 (직진/좌회전/우회전 컴포넌트 공통 처리)
	const TPair<UInstancedStaticMeshComponent*, const TArray<int32>*> ArrowGroups[] =
	{
		{ mPathArrowComponent, &mPathArrowOrders },
		{ mPathTurnLeftComponent, &mPathTurnLeftOrders },
		{ mPathTurnRightComponent, &mPathTurnRightOrders },
	};
	for (const auto& Group : ArrowGroups)
	{
		UInstancedStaticMeshComponent* Component = Group.Key;
		const TArray<int32>& Orders = *Group.Value;
		if (Component == nullptr)
			continue;

		const int32 Count = Component->GetInstanceCount();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			// 순번 기록이 없으면(경로 재구성 전 등) 인스턴스 인덱스로 폴백
			const int32 Order = Orders.IsValidIndex(Index) ? Orders[Index] : Index;
			const float Phase = 2.0f * PI * Time / mPathPulsePeriod - Order * PhasePerTile;
			const float Wave = 0.5f - 0.5f * FMath::Cos(Phase);
			WriteInstance(Component, Index, mPathArrowStyle.mColor, Wave);
		}
	}

	// 도착지 마커와 원뿔은 같은 위상을 공유하니까 미리 계산해서 사용
	const int32 EndPhaseIndex = FMath::Max(0, mPathLength - 1);
	const float EndPhase = 2.0f * PI * Time / mPathPulsePeriod - EndPhaseIndex * PhasePerTile;
	const float EndWave = 0.5f - 0.5f * FMath::Cos(EndPhase);

	// 도착 마커: 경로 끝 위상의 펄스 색
	if (mPathEndComponent != nullptr && mPathEndComponent->GetInstanceCount() > 0)
	{
		WriteInstance(mPathEndComponent, 0, mPathEndStyle.mColor, EndWave);
	}

	// 도착지 원뿔: 색은 고정(SetMovePath에서 기록), 높이만 위아래 진동
	if (mDestConeComponent != nullptr && mDestConeComponent->GetInstanceCount() > 0)
	{
		// 기준 높이 위로 0~진동 폭 사이를 부드럽게 오감
		const float Bob = mDestConeBobAmplitude * (0.5f - 0.5f * FMath::Cos(2.0f * PI * Time / mDestConeBobPeriod));
		FTransform ConeTransform;
		mDestConeComponent->GetInstanceTransform(0, ConeTransform, /*bWorldSpace=*/false);
		ConeTransform.SetTranslation(FVector(ConeTransform.GetTranslation().X, ConeTransform.GetTranslation().Y, GetDestConeBaseZ() + Bob));
		mDestConeComponent->UpdateInstanceTransform(0, ConeTransform, /*bWorldSpace=*/false, /*bMarkRenderStateDirty=*/true, /*bTeleport=*/true);
	}
}

float ATileMap::GetDestConeBaseZ() const
{
	// 원뿔 메시(100cm)는 피벗이 중심이라, 절반 높이를 더해야 뒤집힌 꼭짓점이 기준 높이에 온다
	const float ConeScale = (mTileSize / 100.0f) * mDestConeScale;
	return mPathHeightOffset + mDestConeBaseHeight + ConeScale * 50.0f;
}

void ATileMap::AppendWaypointMarker(const FTileIndex& Tile, int32 Number)
{
	// 숫자별로 디지털 숫자의 어떤 세그먼트들로 구성되는 지 미리 테이블로 만들어 둠
	static const TArray<EMarkerBar> DigitSegments[10] =
	{
		/* 0 */ { EMarkerBar::DigitTop, EMarkerBar::DigitTopLeft, EMarkerBar::DigitTopRight, EMarkerBar::DigitBottomLeft, EMarkerBar::DigitBottomRight, EMarkerBar::DigitBottom },
		/* 1 */ { EMarkerBar::DigitTopRight, EMarkerBar::DigitBottomRight },
		/* 2 */ { EMarkerBar::DigitTop, EMarkerBar::DigitTopRight, EMarkerBar::DigitMiddle, EMarkerBar::DigitBottomLeft, EMarkerBar::DigitBottom },
		/* 3 */ { EMarkerBar::DigitTop, EMarkerBar::DigitTopRight, EMarkerBar::DigitMiddle, EMarkerBar::DigitBottomRight, EMarkerBar::DigitBottom },
		/* 4 */ { EMarkerBar::DigitTopLeft, EMarkerBar::DigitTopRight, EMarkerBar::DigitMiddle, EMarkerBar::DigitBottomRight },
		/* 5 */ { EMarkerBar::DigitTop, EMarkerBar::DigitTopLeft, EMarkerBar::DigitMiddle, EMarkerBar::DigitBottomRight, EMarkerBar::DigitBottom },
		/* 6 */ { EMarkerBar::DigitTop, EMarkerBar::DigitTopLeft, EMarkerBar::DigitMiddle, EMarkerBar::DigitBottomLeft, EMarkerBar::DigitBottomRight, EMarkerBar::DigitBottom },
		/* 7 */ { EMarkerBar::DigitTop, EMarkerBar::DigitTopRight, EMarkerBar::DigitBottomRight },
		/* 8 */ { EMarkerBar::DigitTop, EMarkerBar::DigitTopLeft, EMarkerBar::DigitTopRight, EMarkerBar::DigitMiddle, EMarkerBar::DigitBottomLeft, EMarkerBar::DigitBottomRight, EMarkerBar::DigitBottom },
		/* 9 */ { EMarkerBar::DigitTop, EMarkerBar::DigitTopLeft, EMarkerBar::DigitTopRight, EMarkerBar::DigitMiddle, EMarkerBar::DigitBottomRight, EMarkerBar::DigitBottom },
	};

	const FVector TileCenter(Tile.mX * mTileSize, Tile.mY * mTileSize, 0.0f);

	// 사각형 테두리 4변
	AppendMarkerBar(TileCenter, EMarkerBar::FrameTop);
	AppendMarkerBar(TileCenter, EMarkerBar::FrameBottom);
	AppendMarkerBar(TileCenter, EMarkerBar::FrameLeft);
	AppendMarkerBar(TileCenter, EMarkerBar::FrameRight);

	// 순번 숫자 (1~9), 초과하면 '-' 표시
	if (Number >= 1 && Number <= 9)
	{
		for (EMarkerBar Segment : DigitSegments[Number])
			AppendMarkerBar(TileCenter, Segment);
	}
	else
	{
		AppendMarkerBar(TileCenter, EMarkerBar::DigitMiddle);
	}
}

void ATileMap::AppendMarkerBar(const FVector& TileCenter, EMarkerBar Bar)
{
	if (mWaypointComponent == nullptr)
		return;

	// 마커 치수: 테두리 바깥 크기와 막대 굵기는 프로퍼티, 숫자 크기는 테두리 안쪽에 맞춤
	const float FrameSize = mTileSize * mWaypointFrameScale;
	const float Thickness = mWaypointBarThickness;
	const float DigitHeight = (FrameSize - 2.0f * Thickness) * 0.55f;
	const float DigitWidth = DigitHeight * 0.55f;

	// 세그먼트 길이: 이음새마다 살짝 띄워 실제 디지털 숫자처럼 보이게 함
	const float SegmentLengthH = DigitWidth - Thickness * 1.2f;
	const float SegmentLengthV = DigitHeight * 0.5f - Thickness * 1.2f;

	// 막대 종류별 중심 위치(u=가로, v=세로)와 크기 결정
	const float FrameHalf = (FrameSize - Thickness) * 0.5f;
	float CenterU = 0.0f, CenterV = 0.0f, SizeU = 0.0f, SizeV = 0.0f;
	switch (Bar)
	{
	case EMarkerBar::FrameTop:			CenterV = +FrameHalf;				SizeU = FrameSize;			SizeV = Thickness;								break;
	case EMarkerBar::FrameBottom:		CenterV = -FrameHalf;				SizeU = FrameSize;			SizeV = Thickness;								break;
	case EMarkerBar::FrameLeft:			CenterU = -FrameHalf;				SizeU = Thickness;			SizeV = FrameSize - 2.0f * Thickness;			break;
	case EMarkerBar::FrameRight:		CenterU = +FrameHalf;				SizeU = Thickness;			SizeV = FrameSize - 2.0f * Thickness;			break;
	case EMarkerBar::DigitTop:			CenterV = +DigitHeight * 0.5f;		SizeU = SegmentLengthH;		SizeV = Thickness;								break;
	case EMarkerBar::DigitMiddle:															SizeU = SegmentLengthH;		SizeV = Thickness;								break;
	case EMarkerBar::DigitBottom:		CenterV = -DigitHeight * 0.5f;		SizeU = SegmentLengthH;		SizeV = Thickness;								break;
	case EMarkerBar::DigitTopLeft:		CenterU = -DigitWidth * 0.5f;		CenterV = +DigitHeight * 0.25f;		SizeU = Thickness;		SizeV = SegmentLengthV;	break;
	case EMarkerBar::DigitTopRight:		CenterU = +DigitWidth * 0.5f;		CenterV = +DigitHeight * 0.25f;		SizeU = Thickness;		SizeV = SegmentLengthV;	break;
	case EMarkerBar::DigitBottomLeft:	CenterU = -DigitWidth * 0.5f;		CenterV = -DigitHeight * 0.25f;		SizeU = Thickness;		SizeV = SegmentLengthV;	break;
	case EMarkerBar::DigitBottomRight:	CenterU = +DigitWidth * 0.5f;		CenterV = -DigitHeight * 0.25f;		SizeU = Thickness;		SizeV = SegmentLengthV;	break;
	default:							return;
	}

	// 마커 좌표(u,v)를 타일 로컬로 변환: v는 +X(숫자 위쪽), u는 +Y(숫자 오른쪽), yaw로 카메라 방향 보정
	const FRotator Rotation(0.0f, mWaypointYaw, 0.0f);
	const FVector Offset = Rotation.RotateVector(FVector(CenterV, CenterU, 0.0f));
	const FVector Location = TileCenter + Offset + FVector(0.0f, 0.0f, mPathHeightOffset + mWaypointBarHeight * 0.5f);

	// 막대 메시(100cm)를 막대 크기로 스케일 (X=세로, Y=가로, Z=높이)
	const FVector Scale(SizeV / 100.0f, SizeU / 100.0f, mWaypointBarHeight / 100.0f);
	const int32 InstanceIndex = mWaypointComponent->AddInstance(FTransform(Rotation, Location, Scale), /*bWorldSpace=*/false);

	// 경유지 마커 색 기록 (펄스 없는 고정 색)
	mWaypointComponent->SetCustomDataValue(InstanceIndex, 0, mWaypointStyle.mColor.R);
	mWaypointComponent->SetCustomDataValue(InstanceIndex, 1, mWaypointStyle.mColor.G);
	mWaypointComponent->SetCustomDataValue(InstanceIndex, 2, mWaypointStyle.mColor.B);
	mWaypointComponent->SetCustomDataValue(InstanceIndex, 3, 1.0f, /*bMarkRenderStateDirty=*/true);
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

void ATileMap::DebugWaypointTest()
{
	// 경유지 2개를 지나는 ㄹ자 경로를 직접 구성 (경유지 마커, 화살표 생략, 좌/우회전, 도착 마커, 원뿔 확인)
	// 다른 디버그 표시와 겹치지 않게 x=6~8 영역 사용
	TArray<FMovePathTile> PathTiles;
	PathTiles.Emplace(FTileIndex(6, 1));
	PathTiles.Emplace(FTileIndex(7, 1));
	PathTiles.Emplace(FTileIndex(8, 1), /*bInWaypoint=*/true);
	PathTiles.Emplace(FTileIndex(8, 2));
	PathTiles.Emplace(FTileIndex(8, 3));
	PathTiles.Emplace(FTileIndex(7, 3));
	PathTiles.Emplace(FTileIndex(6, 3), /*bInWaypoint=*/true);
	PathTiles.Emplace(FTileIndex(6, 4));
	PathTiles.Emplace(FTileIndex(6, 5));
	PathTiles.Emplace(FTileIndex(7, 5));
	PathTiles.Emplace(FTileIndex(8, 5));
	SetMovePath(PathTiles);
}
#endif

