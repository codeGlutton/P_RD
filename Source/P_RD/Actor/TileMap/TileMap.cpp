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

	// 경로 화살표/도착 마커 컴포넌트 생성 헬퍼 (장식용 — 타일 트레이스 방해 않도록 충돌 없음)
	auto CreatePathComponent = [this](const TCHAR* Name) -> UInstancedStaticMeshComponent*
	{
		UInstancedStaticMeshComponent* Component = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(RootComponent);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetNumCustomDataFloats(4);
		return Component;
	};

	// 이동/밀치기 경로 표시 세트의 컴포넌트 생성
	mMovePathSet.mStraightComponent = CreatePathComponent(TEXT("MovePathStraight"));
	mMovePathSet.mTurnLeftComponent = CreatePathComponent(TEXT("MovePathTurnLeft"));
	mMovePathSet.mTurnRightComponent = CreatePathComponent(TEXT("MovePathTurnRight"));
	mMovePathSet.mEndComponent = CreatePathComponent(TEXT("MovePathEnd"));
	mPushPathSet.mStraightComponent = CreatePathComponent(TEXT("PushPathStraight"));
	mPushPathSet.mTurnLeftComponent = CreatePathComponent(TEXT("PushPathTurnLeft"));
	mPushPathSet.mTurnRightComponent = CreatePathComponent(TEXT("PushPathTurnRight"));
	mPushPathSet.mEndComponent = CreatePathComponent(TEXT("PushPathEnd"));

	// 메시 로드 헬퍼 (경로가 잘못됐거나 SVN 미갱신이면 null 유지 — 회전 화살표는 직진으로 폴백됨)
	auto FindMesh = [](const TCHAR* Path) -> UStaticMesh*
	{
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	};

	// 이동 경로 기본 메시: 각진 화살표 (+X 진행 기준 형상, 회전은 +X 진입 기준)
	mMovePathSet.mStraightMesh = FindMesh(TEXT("/Game/SVN/OutSideAsset/Kenney/FactoryKit/SM_Kenney_FactoryKit_ArrowStraight.SM_Kenney_FactoryKit_ArrowStraight"));
	mMovePathSet.mTurnLeftMesh = FindMesh(TEXT("/Game/SVN/OutSideAsset/Kenney/FactoryKit/SM_Kenney_FactoryKit_ArrowTurnLeft.SM_Kenney_FactoryKit_ArrowTurnLeft"));
	mMovePathSet.mTurnRightMesh = FindMesh(TEXT("/Game/SVN/OutSideAsset/Kenney/FactoryKit/SM_Kenney_FactoryKit_ArrowTurnRight.SM_Kenney_FactoryKit_ArrowTurnRight"));
	mMovePathSet.mEndMesh = FindMesh(TEXT("/Game/SVN/OutSideAsset/Kenney/FactoryKit/SM_Kenney_FactoryKit_IndicatorSpecialArrow.SM_Kenney_FactoryKit_IndicatorSpecialArrow"));

	// 밀치기 경로 기본 메시: rounded 화살표 (도착 마커는 이동 경로와 공유)
	mPushPathSet.mStraightMesh = FindMesh(TEXT("/Game/SVN/OutSideAsset/Kenney/FactoryKit/SM_Kenney_FactoryKit_ArrowRoundedStraight.SM_Kenney_FactoryKit_ArrowRoundedStraight"));
	mPushPathSet.mTurnLeftMesh = FindMesh(TEXT("/Game/SVN/OutSideAsset/Kenney/FactoryKit/SM_Kenney_FactoryKit_ArrowRoundedTurnLeft.SM_Kenney_FactoryKit_ArrowRoundedTurnLeft"));
	mPushPathSet.mTurnRightMesh = FindMesh(TEXT("/Game/SVN/OutSideAsset/Kenney/FactoryKit/SM_Kenney_FactoryKit_ArrowRoundedTurnRight.SM_Kenney_FactoryKit_ArrowRoundedTurnRight"));
	mPushPathSet.mEndMesh = mMovePathSet.mEndMesh;

	// 컴포넌트에 기본 메시 반영 (에디터 프리뷰용 — 런타임 교체는 AppendPath에서)
	auto ApplySetMeshes = [](FPathArrowSet& Set)
	{
		Set.mStraightComponent->SetStaticMesh(Set.mStraightMesh);
		Set.mTurnLeftComponent->SetStaticMesh(Set.mTurnLeftMesh);
		Set.mTurnRightComponent->SetStaticMesh(Set.mTurnRightMesh);
		Set.mEndComponent->SetStaticMesh(Set.mEndMesh);
	};
	ApplySetMeshes(mMovePathSet);
	ApplySetMeshes(mPushPathSet);

	// 화살표/마커는 전용 발광 머티리얼(custom data RGBA + EmissiveBoost) 사용 — AppendPath에서 컴포넌트에 적용
	// 타일 머티리얼(M_TileTransparent)과 분리해 타일 쪽 테두리 파라미터의 영향 없이 블룸으로 도드라지게 함
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PathIndicatorMatFinder(TEXT("/Game/SVN/InSideAsset/Material/M_PathIndicator.M_PathIndicator"));
	if (PathIndicatorMatFinder.Succeeded())
	{
		mMovePathSet.mMaterial = PathIndicatorMatFinder.Object;
		mPushPathSet.mMaterial = PathIndicatorMatFinder.Object;
	}

	// 경로 화살표와 도착지 마커 색 기본 값 — 이동: 주황 발광, 밀치기: 어두운 적색
	mMovePathSet.mArrowStyle.mColor = FLinearColor(0.9f, 0.45f, 0.0f, 0.95f);
	mMovePathSet.mEndStyle.mColor = FLinearColor(0.9f, 0.45f, 0.0f, 0.95f);
	mPushPathSet.mArrowStyle.mColor = FLinearColor(0.55f, 0.08f, 0.06f, 0.95f);
	mPushPathSet.mEndStyle.mColor = FLinearColor(0.55f, 0.08f, 0.06f, 0.95f);

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
	// [에디터 전용] 에디터 뷰포트에서 디버그 경로 미리보기 — 좌표 변경 즉시 반영 (펄스 애니메이션은 틱이 도는 PIE에서만)
	if (GetWorld() != nullptr && !GetWorld()->IsGameWorld())
	{
		if (mDebugDrawPathOnBeginPlay && mModel != nullptr)
			mModel->SetMovePath(mDebugPathStart, mDebugPathGoal);
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
	// [에디터 전용] 토글이 켜진 인스턴스에서만 PIE 시작 시 디버그 경로를 그려 펄스 검증 (패키징 빌드에선 제거됨)
	if (mDebugDrawPathOnBeginPlay && mModel != nullptr)
		mModel->SetMovePath(mDebugPathStart, mDebugPathGoal);
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

	// 경로 화살표/도착 마커 알파도 펄스 — 표시 중인 세트만 갱신
	if (mMovePathSet.mPathCount > 0)
		RefreshPathPulse(mMovePathSet);
	if (mPushPathSet.mPathCount > 0)
		RefreshPathPulse(mPushPathSet);
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

void ATileMap::SetMovePath(const TArray<FTileIndex>& PathTiles)
{
	// 이동 경로는 한 번에 하나 — 기존 표시 제거 후 이동 세트로 그리기 (공통 구현 위임)
	ClearPath(mMovePathSet);
	AppendPath(mMovePathSet, PathTiles);
}

void ATileMap::ClearMovePath()
{
	ClearPath(mMovePathSet);
}

void ATileMap::AddPushPath(const TArray<FTileIndex>& PathTiles)
{
	// 밀리지 못하는 경로(제자리 한 칸 이하)는 표시하지 않음 — 밀리는 몹만 화살표가 생김
	if (PathTiles.Num() < 2)
		return;

	// 기존 표시를 유지한 채 밀치기 세트에 추가 (공통 구현 위임)
	AppendPath(mPushPathSet, PathTiles);
}

void ATileMap::ClearPushPath()
{
	ClearPath(mPushPathSet);
}

void ATileMap::AppendPath(FPathArrowSet& Set, const TArray<FTileIndex>& PathTiles)
{
	// 경로가 비었으면 추가할 것 없음
	if (PathTiles.Num() == 0)
		return;

	// 에디터에서 교체된 메시/머티리얼 반영
	auto ApplyMeshAndMaterial = [&Set](UInstancedStaticMeshComponent* Component, UStaticMesh* Mesh)
	{
		if (Component == nullptr)
			return;
		Component->SetStaticMesh(Mesh);
		if (Set.mMaterial != nullptr)
			Component->SetMaterial(0, Set.mMaterial);
	};
	ApplyMeshAndMaterial(Set.mStraightComponent, Set.mStraightMesh);
	ApplyMeshAndMaterial(Set.mTurnLeftComponent, Set.mTurnLeftMesh);
	ApplyMeshAndMaterial(Set.mTurnRightComponent, Set.mTurnRightMesh);
	ApplyMeshAndMaterial(Set.mEndComponent, Set.mEndMesh);

	// 화살표와 도착지마커를 타일사이즈에 맞게 스케일 조정
	const float ArrowScale = (mTileSize / 100.0f) * mPathArrowScale;

	// 타일의 위상차 계산:
	// 한 주기 2*PI를 간격의 개수(타일 수 - 1)로 나누면 한 주기에 각 타일들이 가져야할 위상차가 됨
	// 여기에 한 주기에 돌아야 할 사이클 수(기본은 1)를 곱하면 최종 위상차가 됨
	const int32 Span = FMath::Max(1, PathTiles.Num() - 1);
	const float PhasePerTile = 2.0f * PI * Set.mFlowCycles / Span;

	// 도착지 마커인 마지막 타일을 제외하고 화살표 배치
	const int32 LastIndex = PathTiles.Num() - 1;
	for (int32 Index = 0; Index < LastIndex; ++Index)
	{
		const FTileIndex& Tile = PathTiles[Index];
		const FTileIndex& Next = PathTiles[Index + 1];

		// 진출 방향 스텝
		const FTileIndex OutStep = Next - Tile;

		// 기본은 직진 화살표
		EPathArrowKind Kind = EPathArrowKind::Straight;
		float Yaw = StepToYaw(OutStep);

		if (Index > 0)
		{
			// 진입/진출 방향의 벡터 외적 부호로 회전 판별
			// 0: 직진
			// 양수: 우회전
			// 음수: 좌회전
			const FTileIndex& Prev = PathTiles[Index - 1];
			const FTileIndex InStep = Tile - Prev;
			const int32 Cross = InStep.mX * OutStep.mY - InStep.mY * OutStep.mX;

			// 회전 타일이면 회전 화살표로 교체
			// @note 회전 화살표 메시가 없으면 직진 화살표 사용 (회전 화살표 메시만 없는 경우는 없겠지만...)
			if (Cross > 0 && Set.mTurnRightMesh != nullptr)
			{
				Kind = EPathArrowKind::TurnRight;
				Yaw = StepToYaw(InStep);
			}
			else if (Cross < 0 && Set.mTurnLeftMesh != nullptr)
			{
				Kind = EPathArrowKind::TurnLeft;
				Yaw = StepToYaw(InStep);
			}
		}

		// 타일 중심 로컬 위치 + Z 오프셋에 배치 (바닥에서 아주 살짝 뜬 위치)
		const FVector Location(Tile.mX * mTileSize, Tile.mY * mTileSize, mPathHeightOffset);
		const FTransform InstanceTransform(FRotator(0.0f, Yaw, 0.0f), Location, FVector(ArrowScale));
		AddTileInstance(Set, Kind, Tile, InstanceTransform, Index * PhasePerTile);
	}

	// 마지막 타일엔 도착지 마커 배치 (진입 방향과 같은 방향)
	{
		const FTileIndex& EndTile = PathTiles[LastIndex];

		// 진입 방향 = 마지막 스텝(직전 타일 → 도착 타일), 마지막 화살표와 같은 방향을 가리킴
		// 단일 타일 경로(직전 타일 없음)면 진입 방향이 없어 회전 없음(+X)
		FRotator Rotation = FRotator::ZeroRotator;
		if (LastIndex >= 1)
		{
			const FTileIndex& PrevTile = PathTiles[LastIndex - 1];
			const FTileIndex Step = EndTile - PrevTile;
			Rotation = FRotator(0.0f, StepToYaw(Step), 0.0f);
		}

		// 도착지 마커의 위상 오프셋 설정
		const FVector Location(EndTile.mX * mTileSize, EndTile.mY * mTileSize, mPathHeightOffset);
		const FTransform InstanceTransform(Rotation, Location, FVector(ArrowScale));
		AddTileInstance(Set, EPathArrowKind::End, EndTile, InstanceTransform, LastIndex * PhasePerTile);
	}

	// 경로 개수 증가
	++Set.mPathCount;

	// 최초 1회 펄스 색 기록 (이후 틱마다 자동 갱신)
	RefreshPathPulse(Set);
}

void ATileMap::ClearPath(FPathArrowSet& Set)
{
	// 화살표·도착 마커 인스턴스 모두 제거
	if (Set.mStraightComponent != nullptr)
		Set.mStraightComponent->ClearInstances();
	if (Set.mTurnLeftComponent != nullptr)
		Set.mTurnLeftComponent->ClearInstances();
	if (Set.mTurnRightComponent != nullptr)
		Set.mTurnRightComponent->ClearInstances();
	if (Set.mEndComponent != nullptr)
		Set.mEndComponent->ClearInstances();

	// 위상 오프셋 기록 제거
	Set.mStraightPhaseOffsets.Reset();
	Set.mTurnLeftPhaseOffsets.Reset();
	Set.mTurnRightPhaseOffsets.Reset();
	Set.mEndPhaseOffsets.Reset();

	// 타일→인스턴스 기록 제거
	Set.mTileInstances.Reset();

	// 표시 중 경로 없음
	Set.mPathCount = 0;
}

UInstancedStaticMeshComponent* ATileMap::GetComponent(FPathArrowSet& Set, EPathArrowKind Kind)
{
	switch (Kind)
	{
	case EPathArrowKind::TurnLeft:	return Set.mTurnLeftComponent;
	case EPathArrowKind::TurnRight:	return Set.mTurnRightComponent;
	case EPathArrowKind::End:		return Set.mEndComponent;
	default:						return Set.mStraightComponent;
	}
}

TArray<float>& ATileMap::GetPhaseOffsets(FPathArrowSet& Set, EPathArrowKind Kind)
{
	switch (Kind)
	{
	case EPathArrowKind::TurnLeft:	return Set.mTurnLeftPhaseOffsets;
	case EPathArrowKind::TurnRight:	return Set.mTurnRightPhaseOffsets;
	case EPathArrowKind::End:		return Set.mEndPhaseOffsets;
	default:						return Set.mStraightPhaseOffsets;
	}
}

void ATileMap::AddTileInstance(FPathArrowSet& Set, EPathArrowKind Kind,
	const FTileIndex& Tile, const FTransform& InstanceTransform, float PhaseOffset)
{
	UInstancedStaticMeshComponent* Component = GetComponent(Set, Kind);
	if (Component == nullptr)
		return;

	// 타일의 기존 표시는 제거 (나중 표시가 덮어쓰니까)
	RemoveTileInstance(Set, Tile);

	// 컴포넌트에 인스턴스와 오프셋 추가
	const int32 InstanceIndex = Component->AddInstance(InstanceTransform, /*bWorldSpace=*/false);
	GetPhaseOffsets(Set, Kind).Add(PhaseOffset);

	// 타일에 있는 인스턴스와 표시를 매핑해놓은 테이블 갱신 (타일만 주면 어떤 화살표가 있는 지 바로 확인 가능)
	Set.mTileInstances.Add(Tile, TPair<EPathArrowKind, int32>(Kind, InstanceIndex));
}

void ATileMap::RemoveTileInstance(FPathArrowSet& Set, const FTileIndex& Tile)
{
	// 해당 타일에 표시가 있는 지 확인해서 없으면 그냥 리턴
	const TPair<EPathArrowKind, int32>* Found = Set.mTileInstances.Find(Tile);
	if (Found == nullptr)
		return;

	// 삭제할 인스턴스의 종류와 인덱스 저장 (인덱스는 뒤에서 오프셋 삭제할 때 사용)
	const EPathArrowKind Kind = Found->Key;
	const int32 RemovedIndex = Found->Value;

	UInstancedStaticMeshComponent* Component = GetComponent(Set, Kind);
	if (Component == nullptr)
		return;

	// 인스턴스 삭제
	Component->RemoveInstance(RemovedIndex);

	// 오프셋 삭제
	TArray<float>& Offsets = GetPhaseOffsets(Set, Kind);
	if (Offsets.IsValidIndex(RemovedIndex))
		Offsets.RemoveAt(RemovedIndex);

	// 맵에서 해당 타일표시 삭제
	Set.mTileInstances.Remove(Tile);

	// 인스턴스 삭제로 뒤 번호들이 한 칸씩 당겨졌으니, 맵에 남은 같은 종류의 더 큰 번호들도 1씩 당김
	for (TPair<FTileIndex, TPair<EPathArrowKind, int32>>& TileInstancePair : Set.mTileInstances)
	{
		if (TileInstancePair.Value.Key == Kind && TileInstancePair.Value.Value > RemovedIndex)
			--TileInstancePair.Value.Value;
	}
}

/**
 * @details
 * - 색에 밝기를 곱해서 최종 색을 계산
 * - 밝기만 하한<->상한 사이를 오가고, 화살표는 순서대로 위상차를 줘서 흐르는 느낌이 들게 처리
 */
void ATileMap::RefreshPathPulse(FPathArrowSet& Set)
{
	// 게임 시작부터의 누적 시간 (일시정지 시 멈추는 게임 시간)
	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	// 누적시간을 라디안으로 변환.
    // 뒤에서 cos() 함수에 넣어서 0~1 사이의 값으로 만들고,
	// 이 값에서 인스턴스별 오프셋을 빼면 인스턴스의 현재 위상이 나옴.
    // 인스턴스는 '현재 위상'만큼의 밝기로 표현됨.
	const float TimePhase = 2.0f * PI * Time / Set.mPulsePeriod;

	// 한 인스턴스에 펄스 색을 기록하는 헬퍼
	auto WriteInstance = [&Set](UInstancedStaticMeshComponent* Component, int32 InstanceIndex, const FLinearColor& Color, float Wave)
	{
		// 펄스를 상한/하한 값으로 변환해서 색에 곱하면 최종 색이 나옴
		const float Brightness = FMath::Lerp(Set.mPulseMinBrightness, Set.mPulseMaxBrightness, Wave);
		Component->SetCustomDataValue(InstanceIndex, 0, Color.R * Brightness);
		Component->SetCustomDataValue(InstanceIndex, 1, Color.G * Brightness);
		Component->SetCustomDataValue(InstanceIndex, 2, Color.B * Brightness);
		Component->SetCustomDataValue(InstanceIndex, 3, 1.0f, /*bMarkRenderStateDirty=*/true);
	};

	// 종류별로 인스턴스를 돌며 저장된 오프셋으로 위상 평가 (화살표는 화살표 색, 마커는 마커 색)
	const EPathArrowKind Kinds[] = { EPathArrowKind::Straight, EPathArrowKind::TurnLeft, EPathArrowKind::TurnRight, EPathArrowKind::End };
	for (const EPathArrowKind Kind : Kinds)
	{
		UInstancedStaticMeshComponent* Component = GetComponent(Set, Kind);
		if (Component == nullptr)
			continue;

		const TArray<float>& Offsets = GetPhaseOffsets(Set, Kind);
		const FLinearColor& Color = (Kind == EPathArrowKind::End) ? Set.mEndStyle.mColor : Set.mArrowStyle.mColor;

		const int32 Count = Component->GetInstanceCount();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			// 오프셋 기록이 없으면 0으로 폴백 (크래시 대신 동시 펄스로 보이는 안전망)
			const float Offset = Offsets.IsValidIndex(Index) ? Offsets[Index] : 0.0f;

			// 이 인스턴스의 현재 위상(시간항 - 자기 오프셋)을 cos에 넣어서 [-1, +1] 사이 값으로 변환 후,
			// [0, 1] 사이 값으로 최종 변환 -> 이 값이 밝기의 최종 값이 됨
			const float Wave = 0.5f - 0.5f * FMath::Cos(TimePhase - Offset);
			WriteInstance(Component, Index, Color, Wave);
		}
	}
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

void ATileMap::DebugPathTest()
{
	// 모델 경유로 전체 파이프라인 확인: FindPath → 표시 델리깃 → 뷰 SetMovePath
	// (빈 에디터 맵이면 장애물이 없어 시작→목표 계단식 경로가 나온다)
	if (mModel != nullptr)
		mModel->SetMovePath(FTileIndex(1, 3), FTileIndex(4, 5));
}

void ATileMap::DebugPushTest()
{
	// 기존 밀치기 표시 초기화
	ClearPushPath();

	// 일렬 연쇄: 먼 몹 (3,1)→(5,1) 먼저, 가까운 몹 (1,1)→(3,1) 나중
	// 겹치는 타일 (3,1)은 나중 경로의 도착지 마커가 먼저 경로의 출발 화살표를 덮어씀
	AddPushPath({ FTileIndex(3, 1), FTileIndex(4, 1), FTileIndex(5, 1) });
	AddPushPath({ FTileIndex(1, 1), FTileIndex(2, 1), FTileIndex(3, 1) });

	// 길이가 다른 경로 — 경로별 파동 독립(각자 줄기 하나) 확인용
	AddPushPath({ FTileIndex(1, 3), FTileIndex(2, 3), FTileIndex(3, 3), FTileIndex(4, 3) });

	// 밀리지 못하는 몹(제자리 한 칸) — 아무것도 표시되지 않아야 함
	AddPushPath({ FTileIndex(1, 5) });
}
#endif

