/*****************************************************************//**
 * @file   TileMap.h
 * @brief  타일맵 액터 정의 헤더
 * @author 이문환
 * @date   2026-05-26
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Actor/ActorView.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Actor/TileMap/TileLayer.h"
#include "Actor/TileMap/TileHighlight.h"
#include "TileMap.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * @brief 경로 화살표 인스턴스의 종류 (직진, 좌회전, 우회전, 도착지)
 */
enum class EPathArrowKind : uint8
{
	Straight,	// 직진 화살표
	TurnLeft,	// 좌회전 화살표
	TurnRight,	// 우회전 화살표
	End,		// 도착(끝) 타일 마커
};

/**
 * @brief 경로 화살표 표시 세트 — 경로 한 종류(이동/밀치기)를 그리는 데 필요한 재료 묶음
 * @details 컴포넌트는 소유 액터가 생성자에서 만들어 주입하고, 세트는 그 참조와 표시 상태를 가짐
 */
USTRUCT()
struct FPathArrowSet
{
	GENERATED_BODY()

	/**
	 * @brief 직진 화살표 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, meta = (DisplayName = "Straight Component"))
	TObjectPtr<UInstancedStaticMeshComponent> mStraightComponent;

	/**
	 * @brief 좌회전 화살표 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, meta = (DisplayName = "Turn Left Component"))
	TObjectPtr<UInstancedStaticMeshComponent> mTurnLeftComponent;

	/**
	 * @brief 우회전 화살표 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, meta = (DisplayName = "Turn Right Component"))
	TObjectPtr<UInstancedStaticMeshComponent> mTurnRightComponent;

	/**
	 * @brief 도착(끝) 타일 마커 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, meta = (DisplayName = "End Component"))
	TObjectPtr<UInstancedStaticMeshComponent> mEndComponent;

	/**
	 * @brief 직진 화살표 메시 (+X를 가리키는 형상 기준)
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Straight Mesh"))
	TObjectPtr<UStaticMesh> mStraightMesh;

	/**
	 * @brief 좌회전 화살표 메시 (+X 진입 기준, null이면 직진 화살표로 폴백)
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Turn Left Mesh"))
	TObjectPtr<UStaticMesh> mTurnLeftMesh;

	/**
	 * @brief 우회전 화살표 메시 (+X 진입 기준, null이면 직진 화살표로 폴백)
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Turn Right Mesh"))
	TObjectPtr<UStaticMesh> mTurnRightMesh;

	/**
	 * @brief 도착(끝) 타일 마커 메시
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "End Mesh"))
	TObjectPtr<UStaticMesh> mEndMesh;

	/**
	 * @brief 화살표/마커에 덮어쓸 머티리얼 (custom data RGBA 색을 읽는 발광 머티리얼 권장, null이면 메시 기본)
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Material"))
	TObjectPtr<UMaterialInterface> mMaterial;

	/**
	 * @brief 화살표 색 (불투명이라 알파값은 사용하지 않음)
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Arrow Style"))
	FTileHighlightStyle mArrowStyle;

	/**
	 * @brief 도착지 마커 색 (불투명이라 알파값은 사용하지 않음)
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "End Style"))
	FTileHighlightStyle mEndStyle;

	/**
	 * @brief 펄스 주기 (초)
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Pulse Period", ClampMin = "0.01"))
	float mPulsePeriod = 1.0f;

	/**
	 * @brief 경로 전체에 동시에 흐르는 펄스 고점 개수 (1=한 줄기, 경로 길이에 무관하게 일정한 흐름. 0=전체 동시 펄스)
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Flow Cycles", ClampMin = "0.0"))
	float mFlowCycles = 1.0f;

	/**
	 * @brief 펄스 밝기 하한
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Pulse Min Brightness", ClampMin = "0.0", ClampMax = "1.0"))
	float mPulseMinBrightness = 0.1f;

	/**
	 * @brief 펄스 밝기 상한
	 */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Pulse Max Brightness", ClampMin = "0.0", ClampMax = "1.0"))
	float mPulseMaxBrightness = 1.0f;

	/**
	 * @brief 현재 표시 중인 경로 개수 (틱 펄스 갱신 대상 판단용)
	 */
	UPROPERTY(Transient)
	int32 mPathCount = 0;

	/**
	 * @brief 인스턴스별 위상 오프셋(라디안 단위)
	 * @details
	 * - 경로를 구성하는 인스턴스들에 대해서 순서대로 위상차를 설정
	 * - 예를 들어, 경로에 4개의 인스턴스가 있으면 각각 0, 0.5파이, 1파이, 1.5파이 위상차를 설정
	 * - Tick에서는 시간과 위상차를 사용해서 밝기 결정
	 * - 시간에 따라 밝기가 변하는데 어두움<->밝음 사이를 위상차를 두고 진동하니까 마치 흐르는 것 같은 느낌을 줌
	 * - 파도타기 응원을 생각하면 쉬움
	 */

	/**
	 * @brief 직진 화살표 인스턴스별 위상 오프셋
	 */
	TArray<float> mStraightPhaseOffsets;

	/**
	 * @brief 좌회전 화살표 인스턴스별 위상 오프셋
	 */
	TArray<float> mTurnLeftPhaseOffsets;

	/**
	 * @brief 우회전 화살표 인스턴스별 위상 오프셋
	 */
	TArray<float> mTurnRightPhaseOffsets;

	/**
	 * @brief 도착 마커 인스턴스별 위상 오프셋 (경로마다 마커 1개, 자기 경로 끝 칸의 위상)
	 */
	TArray<float> mEndPhaseOffsets;

	/**
	 * @brief 타일→(화살표 종류, 인스턴스 번호) 역방향 조회
	 * @details
	 * - 같은 타일에 나중 경로가 오면 기존 표시를 덮어쓰기 하는 용도
	 */
	TMap<FTileIndex, TPair<EPathArrowKind, int32>> mTileInstances;
};

/**
 * @brief  타일맵 액터
 */
UCLASS()
class P_RD_API ATileMap : public AActor, public IActorView
{
	GENERATED_BODY()

public:
	ATileMap();

	// @brief 에디터 배치/스폰 시 타일 그리드 인스턴스 재생성
	virtual void OnConstruction(const FTransform& Transform) override;

	// @brief 매 프레임 Effect 하이라이트 펄스 갱신
	virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR
	// @brief [에디터 전용] 디테일 편집 즉시 반영 — 배치 계열은 그리드 재생성+액터 재배치, 색/테두리 계열은 표시만 갱신
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/* IActorView 상속 */
	// @brief 런타임 모델을 뷰에 바인딩 (모델 교체 + 델리깃 재바인딩 + 그리드 재생성)
	virtual void BindModel(UObjectModel* Model) override;

	// @brief 모델 바인딩 해제 (델리깃 정리)
	virtual void UnbindModel(UObjectModel* Model) override;

protected:
	// @brief Blueprint/native 기본 서브오브젝트 인스턴싱 완료 후 경로 컴포넌트 참조를 복구
	virtual void PostInitializeComponents() override;

    // @brief 게임 시작 처리 (에디터 전용 디버그 경로 표시 토글 포함)
    virtual void BeginPlay() override;

	// @brief 보유 모델 반환 (IObjectView::GetModel 용)
	virtual UObjectModel* GetModel_Internal() const override;

public:
	/* 타일 → 월드 변환 */
	/**
	 * @brief 타일 트랜스폼(인덱스+방향)을 월드 트랜스폼으로 변환
	 * @param[in] TileTransform : 타일 좌표 + 방향
	 * @return FTransform : 해당 타일 중심의 월드 트랜스폼 (위치+회전+스케일)
	 */
	FTransform TileToWorldTransform(const FTileTransform& TileTransform) const;

	/**
	 * @brief 타일 인덱스를 월드 위치로 변환
	 * @param[in] TileIndex : 타일 좌표
	 * @return FVector : 해당 타일 중심의 월드 위치
	 */
	FVector TileToWorldLocation(const FTileIndex& TileIndex) const;

	/**
	 * @brief 타일 인덱스를 월드 위치로 변환
	 * @param[in] Direction : 타일 방향
	 * @return FVector : 해당 타일 중심의 월드 회전
	 */
	FRotator TileToWorldRotation(ETileActorDirection Direction) const;

	/* 월드 → 타일 변환 */
	/**
	 * @brief 월드 위치를 타일 인덱스로 변환
	 * @param[in] WorldLocation : 월드 위치
	 * @return FTileIndex : 가장 가까운 타일 인덱스 (범위 밖 처리 정책은 구현 시 결정)
	 */
	FTileIndex WorldToTileIndex(const FVector& WorldLocation) const;

	/* 강조 표시 */
	/**
	 * @brief 지정한 타일들에 강조 상태를 설정 (해당 플래그 비트에 한해 치환 + 하위 의존 레이어 클리어)
	 * @details
	 * Flag 비트를 가진 기존 타일에서 끄고 Tiles에만 켠다. 상위/무관 플래그 비트는 보존하므로,
	 * Aim 위에 Select를 칠해도 Aim은 유지된다.
	 *
	 * 드릴다운 의존성(Aim → Select → Effect): 상위 레이어를 새로 설정하면 하위(더 구체적)
	 * 레이어는 무효가 되어 모든 타일에서 자동 클리어된다.
	 * - Aim 설정    → Select, Effect 클리어
	 * - Select 설정 → Effect 클리어 (Aim은 맥락으로 유지)
	 * - Effect 설정 → 클리어 없음
	 *
	 * @param[in] Tiles : 강조할 타일 목록 (맵 밖 좌표는 무시)
	 * @param[in] Flag  : 설정할 강조 상태
	 */
	void SetTileHighlight(const TArray<FTileIndex>& Tiles, ETileHighlightFlag Flag);

	/**
	 * @brief 지정한 강조 상태를 모든 타일에서 해제
	 * @details
	 * OR로 조합한 여러 비트를 한 번에 끌 수 있다 (예: Select | Effect).
	 * Set과 달리 하위 의존 레이어 캐스케이드는 하지 않는다 — 지정한 비트만 끈다.
	 * (캐스케이드 무효화가 필요하면 해당 상위 레이어를 다시 Set하면 된다.)
	 * @param[in] Flag : 해제할 강조 상태 (비트 조합 가능)
	 */
	void ClearTileHighlight(ETileHighlightFlag Flag);

	/* 이동 경로 */
	/**
	 * @brief 이동경로를 화살표와 경유지마커로 표시
	 * @details
	 * 경로를 구성하는 타일배열을 받아 각 타일에 '다음 타일로 향하는' 화살표 배치
	 * 경유지 타일에는 화살표 대신 경유지 표시(사각형 테두리 안에 경유지 순번), 도착지에는 도착지 마커와 원역뿔을 배치
	 * 같은 타일이 여러 번 경유지면 마지막 순번 마커만 남고(덮어쓰기), 도착 타일과 겹친 경유지 마커는 생략
	 * 기존 경로 표시는 먼저 지운다. 빈 배열이면 표시를 모두 해제하는 것과 같다.
	 * @param[in] PathTiles : 경로 타일 목록 (양 끝 포함)
	 */
	void SetMovePath(const TArray<FMovePathTile>& PathTiles);

	/**
	 * @brief 이동경로 표시 해제 (화살표, 마커, 원뿔 모두 제거)
	 */
	void ClearMovePath();

	/**
	 * @brief 밀치기 경로를 기존 표시에 추가
	 * @details
	 * 광역 밀치기처럼 여러 몹이 밀릴 때 몹마다 GetPushPath 결과를 쌓는 용도.
	 * 밀리지 못하는 경로(타일 1개 이하)는 무시
	 * 같은 타일에 여러 표시가 중복되면 나중 표시가 덮어씀
	 * @param[in] PathTiles 경로 타일 목록 (밀리는 몹의 타일부터 도착까지)
	 */
	void AddPushPath(const TArray<FTileIndex>& PathTiles);

	/**
	 * @brief 밀치기 경로 표시 해제 (화살표·도착 마커 인스턴스 모두 제거)
	 */
	void ClearPushPath();

#if WITH_EDITOR
	/**
	 * @brief [에디터 전용] 하이라이트 테스트 패턴을 칠해 시각 확인 (단독/겹침)
	 * @details 디테일 패널 버튼으로 호출. Aim 한 줄 + Select 한 칸 + Effect 몇 칸을 칠하되,
	 *          일부 칸은 겹치게 해서 단독/겹침 합성을 한 번에 확인한다.
	 */
	UFUNCTION(CallInEditor, Category = "SRPG")
	void DebugPaintTest();

	/**
	 * @brief [에디터 전용] 이동경로 표시 시각 확인 (화살표, 경유지 마커, 도착 마커, 원뿔)
	 * @details 디테일 패널 버튼으로 호출. 경유지 2개를 지나는 경로를 그린다. 진동과 펄스는 틱이 도는 PIE에서 보인다.
	 */
	UFUNCTION(CallInEditor, Category = "SRPG")
	void DebugWaypointTest();

	/**
	 * @brief [에디터 전용] 밀치기 다중 경로를 칠해 rounded 화살표/도착 마커/덮어쓰기 시각 확인
	 * @details 디테일 패널 버튼으로 호출. 일렬 연쇄(먼 몹 먼저) + 길이 다른 경로 + 안 밀리는 몹(표시 없음)을 고정 경로로 확인.
	 */
	UFUNCTION(CallInEditor, Category = "SRPG")
	void DebugPushTest();
#endif

#if WITH_EDITORONLY_DATA
	/**
	 * @brief [에디터 전용] 에디터 뷰포트와 PIE 시작 시 경유지 디버그 경로를 그릴지 여부 (인스턴스별 토글)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Debug", meta = (DisplayName = "Debug Draw Waypoint Path"))
	bool mDebugDrawWaypointPath = false;
#endif

	/* 타일맵 정보 */
	/**
	 * @brief 타일 한 칸의 월드 크기 (cm): 기본값은 100cm
	 */
	float GetTileSize() const;

	/* 좌표 유효성 (모델 위임) */
	/**
	 * @brief 타일 인덱스가 맵 범위 안 유효 좌표인지 검사 (모델 IsValidIndex로 위임)
	 */
	bool IsValidIndex(const FTileIndex& TileIndex) const;

protected:
	/**
	 * @brief 타일맵 데이터 모델 (데이터·계산 담당, 뷰와 분리)
	 * @details Instanced로 액터가 소유·직렬화하며, ShowOnlyInnerProperties로 내부 프로퍼티를 디테일 패널 최상위에 펼쳐 노출
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "SRPG", meta = (DisplayName = "Model", ShowOnlyInnerProperties))
	TObjectPtr<UTileMapModel> mModel;

	/**
	 * @brief 타일 한 칸의 월드 크기 (cm)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG", meta = (DisplayName = "Tile Size", ClampMin = "1.0"))
	float mTileSize = 250.0f;

	/* 시각화 */

	/**
	 * @brief 타일 그리드를 그리는 인스턴스드 메시 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, Category = "SRPG|Visual", meta = (DisplayName = "Tile Mesh Component"))
	TObjectPtr<UInstancedStaticMeshComponent> mTileMeshComponent;

	/**
	 * @brief 타일 한 칸에 사용할 메시 (기본: 엔진 Plane)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Visual", meta = (DisplayName = "Tile Mesh"))
	TObjectPtr<UStaticMesh> mTileMesh;

	/**
	 * @brief 타일 메시에 덮어쓸 머티리얼 (null이면 메시 기본 머티리얼)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Visual", meta = (DisplayName = "Tile Material"))
	TObjectPtr<UMaterialInterface> mTileMaterial;

	/**
	 * @brief 타일 시각 크기 비율 (1.0 미만이면 타일 사이에 틈이 생겨 격자선처럼 보임)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Visual", meta = (DisplayName = "Tile Visual Scale", ClampMin = "0.1", ClampMax = "1.0"))
	float mTileVisualScale = 0.9f;

	/**
	 * @brief 하이라이트 없는 기본 타일에 얹는 구분색 (머티리얼 바탕 위에 알파만큼 Mix, 알파 0이면 바탕만 보임)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Visual", meta = (DisplayName = "Tile Base Style"))
	FTileHighlightStyle mTileBaseStyle;

	/**
	 * @brief 타일 테두리 색
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Visual", meta = (DisplayName = "Tile Border Style"))
	FTileHighlightStyle mTileBorderStyle;

	/**
	 * @brief 타일 테두리 굵기 (cm 단위)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Visual", meta = (DisplayName = "Tile Border Width", ClampMin = "0.0"))
	float mTileBorderWidth = 3.0f;

	/**
	 * @brief 경유지 마커 메시 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, Category = "SRPG|Visual", meta = (DisplayName = "Waypoint Component"))
	TObjectPtr<UInstancedStaticMeshComponent> mWaypointComponent;

	/**
	 * @brief 도착지 원뿔 메시 컴포넌트 (기존 바닥 메시와 별개로 머리 위에서 위아래로 진동)
	 */
	UPROPERTY(VisibleAnywhere, Category = "SRPG|Visual", meta = (DisplayName = "Dest Cone Component"))
	TObjectPtr<UInstancedStaticMeshComponent> mDestConeComponent;

	/* 강조 표시 */

	/**
	 * @brief 조준 범위 스타일 (타일 위에 자기 알파로 Mix)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Highlight", meta = (DisplayName = "Aim Style"))
	FTileHighlightStyle mAimStyle;

	/**
	 * @brief 선택 타일 스타일 (겹치면 최우선 — 자기 색만 칠하고 Effect/Aim 무시)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Highlight", meta = (DisplayName = "Select Style"))
	FTileHighlightStyle mSelectStyle;

	/**
	 * @brief 영향 범위 스타일 (아래 레이어[Aim/타일] ↔ 자기 색 사이를 펄스로 오감)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Highlight", meta = (DisplayName = "Effect Style"))
	FTileHighlightStyle mEffectStyle;

	/**
	 * @brief 펄스 주기 (초)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Highlight", meta = (DisplayName = "Pulse Period", ClampMin = "0.01"))
	float mPulsePeriod = 1.0f;

	/* 경로 표시 */

	/**
	 * @brief 이동 경로 표시 세트
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Move Path Set"))
	FPathArrowSet mMovePathSet;

	/**
	 * @brief 밀치기 경로 표시 세트
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Push Path Set"))
	FPathArrowSet mPushPathSet;

	/**
	 * @brief 화살표/마커 시각 크기 비율 (타일 크기 기준)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path Arrow Scale", ClampMin = "0.01"))
	float mPathArrowScale = 0.5f;

	/**
	 * @brief 화살표 Z축(두께) 스케일 비율 (1이면 원본 두께, 도착 마커에는 적용 안 함)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path Arrow Z Scale", ClampMin = "0.05", ClampMax = "1.0"))
	float mPathArrowZScale = 0.5f;

	/**
	 * @brief 화살표/마커를 타일 위로 띄우는 Z 오프셋 (cm, z-fighting 방지)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path Height Offset"))
	float mPathHeightOffset = 1.0f;

	/* 경유지/도착지 원뿔 표시 */

	/**
	 * @brief 경유지 마커 막대 메시 (기본: 엔진 Cube)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Waypoint Bar Mesh"))
	TObjectPtr<UStaticMesh> mWaypointBarMesh;

	/**
	 * @brief 경유지 마커 색
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Waypoint Style"))
	FTileHighlightStyle mWaypointStyle;

	/**
	 * @brief 경유지 테두리 크기 비율 (타일 크기 기준)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Waypoint Frame Scale", ClampMin = "0.1", ClampMax = "1.0"))
	float mWaypointFrameScale = 0.6f;

	/**
	 * @brief 마커 막대 굵기 (cm)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Waypoint Bar Thickness", ClampMin = "0.5"))
	float mWaypointBarThickness = 8.0f;

	/**
	 * @brief 마커 막대 높이 (cm)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Waypoint Bar Height", ClampMin = "0.5"))
	float mWaypointBarHeight = 8.0f;

	/**
	 * @brief 숫자 방향 (도, 0이면 숫자 위쪽이 +X — 카메라에 맞춰 조정)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Waypoint Yaw"))
	float mWaypointYaw = 0.0f;

	/**
	 * @brief 도착지 원뿔 메시 (기본: 엔진 Cone, 뒤집어서 배치)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Dest Cone Mesh"))
	TObjectPtr<UStaticMesh> mDestConeMesh;

	/**
	 * @brief 도착지 원뿔 색
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Dest Cone Style"))
	FTileHighlightStyle mDestConeStyle;

	/**
	 * @brief 원뿔 크기 비율 (타일 크기 기준)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Dest Cone Scale", ClampMin = "0.01"))
	float mDestConeScale = 0.25f;

	/**
	 * @brief 원뿔 기준 높이 (cm, 타일 바닥에서 원뿔 꼭짓점까지)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Dest Cone Base Height", ClampMin = "0.0"))
	float mDestConeBaseHeight = 80.0f;

	/**
	 * @brief 원뿔 진동 폭 (cm)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Dest Cone Bob Amplitude", ClampMin = "0.0"))
	float mDestConeBobAmplitude = 30.0f;

	/**
	 * @brief 원뿔 진동 주기 (초)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Waypoint", meta = (DisplayName = "Dest Cone Bob Period", ClampMin = "0.01"))
	float mDestConeBobPeriod = 1.0f;

private:
	/**
	 * @brief 현재 Width/Height/TileSize에 맞춰 타일 저장소와 인스턴스를 모두 재생성
	 * @warning 모델의 타일 저장소를 초기화해 배치(점유) 정보가 사라짐 — 런타임 시각 갱신은 RefreshTileVisuals 사용
	 */
	void RebuildTileInstances();

	/**
	 * @brief 타일 저장소는 건드리지 않고 그리드 인스턴스·머티리얼·타일 색만 재생성 (런타임 시각 갱신용)
	 */
	void RefreshTileVisuals();

	/**
	 * @brief 테두리 색/굵기를 타일 MID 파라미터(BorderColor/BorderWidth)로 푸시 (굵기 cm → UV 비율 환산)
	 */
	void ApplyBorderParameters();

	/**
	 * @brief 타일 그리드 전용 다이나믹 머티리얼 (테두리 파라미터 푸시 통로 — 원본 에셋을 쓰는 경로 화살표/마커는 영향 없음)
	 */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> mTileMID;

	/**
	 * @brief 한 타일의 강조 상태(mHighlights)를 스타일로 합성해 ISM custom data에 기록
	 * @details
	 * Select가 있으면 Select 색만(최우선). 아니면 Effect가 아래 레이어(Aim 있으면 Aim, 없으면 타일)와
	 * 자기 색 사이를 펄스로 보간한다. 둘 다 없으면 Aim 색(또는 타일).
	 * 모든 색은 타일 위에 자기 알파로 Mix(프리멀티). 슬롯 0~2=알파 곱해진 RGB, 3=커버리지 알파.
	 * @param[in] LinearIndex 타일/인스턴스 1차원 인덱스
	 */
	void RefreshTileCustomData(int32 LinearIndex);

	/**
	 * @brief 타일별 강조 표시 상태 (시각 전용 — 전투/시뮬레이션과 무관, FTile과 분리)
	 * @details mTiles와 같은 1차원 인덱싱(y*Width+x), 크기 Width*Height. ISM custom data로 화면에 반영된다.
	 */
	UPROPERTY()
	TArray<ETileHighlightFlag> mHighlights;

	/**
	 * @brief 경로 하나를 세트의 메시/색으로 기존 표시에 추가
	 * @details
	 *   기존 표시는 유지 (전체 제거는 ClearPath 별도 호출)
	 *   인스턴스별 위상 오프셋을 이 경로의 간격 수로 확정해 저장
	 *   같은 타일에 기존 인스턴스가 있으면 제거 후 배치 (나중 경로가 이김)
	 *   경유지 타일에는 화살표 대신 순번 마커 배치 (밀치기 경로는 경유지 없음)
	 *   SetMovePath/AddPushPath 공통 구현
	 * @param[in] Set : 사용할 표시 세트
	 * @param[in] PathTiles : 경로 타일 목록 (양 끝 포함, 경유지 여부 포함)
	 */
	void AppendPath(FPathArrowSet& Set, const TArray<FMovePathTile>& PathTiles);

	/**
	 * @brief 세트의 경로 표시 관련된 데이터 모두 초기화
	 */
	void ClearPath(FPathArrowSet& Set);

	/**
	 * @brief 세트에서 화살표 종류에 해당하는 컴포넌트 반환
	 */
	static UInstancedStaticMeshComponent* GetComponent(FPathArrowSet& Set, EPathArrowKind Kind);

	/**
	 * @brief 세트에서 화살표 종류에 해당하는 위상 오프셋 배열 반환
	 */
	static TArray<float>& GetPhaseOffsets(FPathArrowSet& Set, EPathArrowKind Kind);

	/**
	 * @brief 세트의 컴포넌트에 인스턴스 추가
	 * @note 같은 타일에 기존 표시가 있었다면 나중 표시로 덮어써짐
	 */
	static void AddTileInstance(FPathArrowSet& Set, EPathArrowKind Kind,
		const FTileIndex& Tile, const FTransform& InstanceTransform, float PhaseOffset);

	/**
	 * @brief 세트에서 타일 하나의 표시를 제거
	 */
	static void RemoveTileInstance(FPathArrowSet& Set, const FTileIndex& Tile);

	/**
	 * @brief 세트의 화살표와 도착 마커의 펄스값을 계산해서 내부에 저장
	 * @details 화살표들이 위상차를 갖게 해서 흐르는 느낌을 줌
	 */
	void RefreshPathPulse(FPathArrowSet& Set);

	/**
	 * @brief 도착지 원뿔의 기준 높이 계산
	 */
	float GetDestConeBaseZ() const;

	/**
	 * @brief 도착지 원뿔을 기준 높이 위로 위아래 진동 (틱에서 호출, 색은 고정)
	 */
	void RefreshDestConeBob();

	/**
	 * @brief 마커를 구성하는 막대 종류 (테두리 4변 + 숫자 7세그먼트)
	 */
	enum class EMarkerBar : uint8
	{
		FrameTop,			// 테두리 상단
		FrameBottom,		// 테두리 하단
		FrameLeft,			// 테두리 왼쪽
		FrameRight,			// 테두리 오른쪽
		DigitTop,			// 숫자 상단 가로
		DigitMiddle,		// 숫자 중앙 가로
		DigitBottom,		// 숫자 하단 가로
		DigitTopLeft,		// 숫자 좌상 세로
		DigitTopRight,		// 숫자 우상 세로
		DigitBottomLeft,	// 숫자 좌하 세로
		DigitBottomRight,	// 숫자 우하 세로
	};

	/**
	 * @brief 경유지 마커를 디지털 숫자 형식으로 조립
	 * @param[in] Tile   : 경유지 타일
	 * @param[in] Number : 경유지 순번 (1~9, 초과 시 '-' 표시)
	 */
	void AppendWaypointMarker(const FTileIndex& Tile, int32 Number);

	/**
	 * @brief 마커 막대 1개 추가 (종류별 위치와 크기는 함수 내부에서 결정)
	 * @param[in] TileCenter : 타일 중심 로컬 위치
	 * @param[in] Bar        : 막대 종류
	 */
	void AppendMarkerBar(const FVector& TileCenter, EMarkerBar Bar);

	/**
	 * @brief 방향 스텝(dx,dy)을 +X 기준 yaw(도)로 변환 (메시가 +X를 향한다고 가정)
	 */
	static float StepToYaw(const FTileIndex& Step);

	/**
	 * @brief 모델→뷰 표시 델리깃을 현재 mModel에 바인딩
	 * @details 생성 시 임시로 한 번, 런타임에 모델이 매핑/교체되면 그 자리에서 다시 호출한다.
	 *          싱글캐스트라 재호출 시 기존 바인딩을 덮어쓴다. (좌표 변환 델리깃 바인딩도 추후 이 자리로 합칠 것)
	 */
	void BindModelDelegates();

	/**
	 * @brief 타일맵뷰 트랜스폼 변경 시 유닛뷰 위치도 변경
	 */
	void OnRootTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);
};
