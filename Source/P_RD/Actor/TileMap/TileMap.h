/*****************************************************************//**
 * @file   TileMap.h
 * @brief  타일맵 액터 정의 헤더
 * @author 이문환
 * @date   2026-05-26
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/ActorView.h"
#include "GameFramework/Actor.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Actor/TileMap/TileLayer.h"
#include "Actor/TileMap/TileHighlight.h"
#include "TileMap.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class ITileActor;

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

	// @brief 게임 시작 처리 (에디터 전용 디버그 경로 표시 토글 포함)
	virtual void BeginPlay() override;

	// @brief 매 프레임 Effect 하이라이트 펄스 갱신
	virtual void Tick(float DeltaSeconds) override;

	/* IActorView 상속 — 팩토리(UGameObjectModelFactory::OnPostCreateNewModel)가 스폰 후 호출하여
	   런타임 전투 모델(UTileMapModel)을 이 뷰 액터에 바인딩한다. 바인딩되어야 GetView<ATileMap>()가
	   유효해지고(CombatUIAdapter의 타일 하이라이트), 모델 좌표로 그리드 인스턴스를 재생성한다. */
public:
	void BindModel(UObjectModel* Model) override;
	void UnbindModel(UObjectModel* Model) override;

protected:
	UObjectModel* GetModel_Internal() const override;

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
	 * @brief 이동경로를 화살표로 표시 (경로 타일 배열을 방향 화살표 + 도착 마커로 시각화)
	 * @details
	 * UTileMapModel::FindPath가 돌려준 타일 배열(시작→목표 순서)을 받는다.
	 * 마지막을 제외한 각 타일에 '다음 타일을 향하는' 화살표를, 마지막 타일엔 도착 마커를 배치한다.
	 * 기존 경로 표시는 먼저 지운다. 빈 배열이면 표시를 모두 해제하는 것과 같다.
	 * @param[in] PathTiles : 경로 타일 목록 (FindPath 결과, 양 끝 포함)
	 */
	void SetMovePath(const TArray<FTileIndex>& PathTiles);

	/**
	 * @brief 이동경로 표시 해제 (화살표·도착 마커 인스턴스 모두 제거)
	 */
	void ClearMovePath();

#if WITH_EDITOR
	/**
	 * @brief [에디터 전용] 하이라이트 테스트 패턴을 칠해 시각 확인 (단독/겹침)
	 * @details 디테일 패널 버튼으로 호출. Aim 한 줄 + Select 한 칸 + Effect 몇 칸을 칠하되,
	 *          일부 칸은 겹치게 해서 단독/겹침 합성을 한 번에 확인한다.
	 */
	UFUNCTION(CallInEditor, Category = "SRPG")
	void DebugPaintTest();

	/**
	 * @brief [에디터 전용] 이동경로를 칠해 화살표/도착 마커 시각 확인
	 * @details 디테일 패널 버튼으로 호출. 모델 경유(FindPath→델리깃→SetMovePath). 펄스는 틱이 도는 PIE에서 보인다.
	 */
	UFUNCTION(CallInEditor, Category = "SRPG")
	void DebugPathTest();
#endif

#if WITH_EDITORONLY_DATA
	/**
	 * @brief [에디터 전용] PIE 시작 시 디버그 경로를 그릴지 여부 (펄스 검증용, 인스턴스별 토글)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Debug", meta = (DisplayName = "Debug Draw Path On Begin Play"))
	bool mDebugDrawPathOnBeginPlay = false;

	/**
	 * @brief [에디터 전용] 디버그 경로 시작 타일
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Debug", meta = (DisplayName = "Debug Path Start", EditCondition = "mDebugDrawPathOnBeginPlay"))
	FTileIndex mDebugPathStart = FTileIndex(1, 3);

	/**
	 * @brief [에디터 전용] 디버그 경로 목표 타일
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Debug", meta = (DisplayName = "Debug Path Goal", EditCondition = "mDebugDrawPathOnBeginPlay"))
	FTileIndex mDebugPathGoal = FTileIndex(4, 5);
#endif

	/* 타일맵 정보 */
	/**
	 * @brief 타일 한 칸의 월드 크기 (cm): 기본값은 100cm
	 */
	float GetTileSize() const;

	/* 옛 ITileActor 인터페이스 호환 (전환기) */
	/**
	 * @details
	 * 타일맵 로직이 UTileMapModel로 이전되면서 기존 호출부(SRPGCombatSubsystem / SRPGSkillBuildAction /
	 * SRPGAction)가 쓰던 옛 시그니처가 사라졌다. 호출부를 모델 기반으로 이관하기 전까지 빌드를 살리는 호환층이다.
	 * 좌표/타일 계산(Aim/Effect)은 모델로 위임해 실제 동작하고, 액터 배치·조회는 ITileActor↔UBoardActorModel
	 * 매핑이 없어 껍데기 스텁이다. 호출부 이관 후 이 묶음은 제거한다.
	 */

	/**
	 * @brief 기준 좌표에서 조준 가능한 타일 목록 반환 (모델 GetAimableTiles로 위임)
	 * @details Incoming(ITileActor)은 모델의 UBoardActorModel로 매핑할 수 없어 무시(nullptr 전달)한다.
	 */
	TArray<FTileIndex> GetAimableTiles(
		const FTileIndex& Origin,
		int32 Range,
		EAimPattern Pattern,
		bool bIncludeOccupied,
		bool bIndirect,
		const ITileActor* Incoming = nullptr
	) const;

	/**
	 * @brief 스킬 발동 시 영향받는 타일 목록 반환 (모델 GetEffectTiles로 위임)
	 */
	TArray<FTileIndex> GetEffectTiles(
		const FTileIndex& Caster,
		const FTileIndex& Target,
		EEffectPattern Pattern,
		int32 Size,
		bool bPenetrate
	) const;

	/**
	 * @brief 기준 좌표에서 이동 가능한 타일 목록 반환 (모델 GetReachableTiles로 위임, BFS 경로 기반)
	 */
	TArray<FTileIndex> GetReachableTiles(const FTileIndex& Origin, int32 MoveDistance) const;

	/**
	 * @brief [스텁] 진입 액터 배치 가능 여부 — ITileActor↔모델 매핑 전까지 항상 true
	 */
	bool CanPlace(const FTileIndex& TileIndex, const ITileActor* Incoming) const;

	/**
	 * @brief [스텁] 액터 배치 — ITileActor↔모델 매핑 전까지 동작 없음
	 */
	void PlaceActor(const FTileTransform& NextTransform, ITileActor* Actor);

	/**
	 * @brief [스텁] 타일 위 액터 조회 — ITileActor↔모델 매핑 전까지 빈 목록
	 */
	TArray<TScriptInterface<ITileActor>> GetActorsOnTile(const FTileIndex& TileIndex, ETileLayerFlag LayerFilter = ETileLayerFlag::All) const;

	/**
	 * @brief 타일 인덱스가 맵 범위 안 유효 좌표인지 검사 (모델 IsValidIndex로 위임)
	 */
	bool IsValidIndex(const FTileIndex& TileIndex) const;

	/**
	 * @brief [스텁] 액터 이동 시작 — ITileActor↔모델 매핑 전까지 동작 없음
	 */
	void StartActorMovement(const FTileTransform& NextTransform, ITileActor* Actor);

	/**
	 * @brief [스텁] 액터 이동 완료 — ITileActor↔모델 매핑 전까지 동작 없음
	 */
	void CompleteActorMovement(ITileActor* Actor);

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
	float mTileSize = 100.0f;

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
	float mTileVisualScale = 0.95f;

	/**
	 * @brief 경로 중간 화살표를 그리는 인스턴스드 메시 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, Category = "SRPG|Visual", meta = (DisplayName = "Path Arrow Component"))
	TObjectPtr<UInstancedStaticMeshComponent> mPathArrowComponent;

	/**
	 * @brief 도착(끝) 타일 마커를 그리는 인스턴스드 메시 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, Category = "SRPG|Visual", meta = (DisplayName = "Path End Component"))
	TObjectPtr<UInstancedStaticMeshComponent> mPathEndComponent;

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

	/* 이동 경로 표시 */

	/**
	 * @brief 경로 중간 화살표 메시 (기본: Kenney SM_Kenney_FactoryKit_Arrow, +X를 가리키는 형상이어야 방향이 맞음)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path Arrow Mesh"))
	TObjectPtr<UStaticMesh> mPathArrowMesh;

	/**
	 * @brief 화살표에 덮어쓸 머티리얼 (custom data RGBA 색을 읽는 하이라이트 머티리얼 권장, null이면 메시 기본)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path Arrow Material"))
	TObjectPtr<UMaterialInterface> mPathArrowMaterial;

	/**
	 * @brief 도착(끝) 타일 마커 메시 (기본: Kenney SM_Kenney_FactoryKit_IndicatorSpecialArrow)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path End Mesh"))
	TObjectPtr<UStaticMesh> mPathEndMesh;

	/**
	 * @brief 도착 마커에 덮어쓸 머티리얼 (null이면 메시 기본)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path End Material"))
	TObjectPtr<UMaterialInterface> mPathEndMaterial;

	/**
	 * @brief 경로 화살표 색·알파 (알파는 펄스 고점 기준)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path Arrow Style"))
	FTileHighlightStyle mPathArrowStyle;

	/**
	 * @brief 도착 마커 색·알파 (알파는 펄스 고점 기준)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path End Style"))
	FTileHighlightStyle mPathEndStyle;

	/**
	 * @brief 경로 펄스 주기 (초)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path Pulse Period", ClampMin = "0.01"))
	float mPathPulsePeriod = 1.0f;

	/**
	 * @brief 경로 전체에 동시에 흐르는 펄스 고점 개수 (1=한 줄기, 경로 길이에 무관하게 일정한 흐름. 0=전체 동시 펄스)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path Flow Cycles", ClampMin = "0.0"))
	float mPathFlowCycles = 1.0f;

	/**
	 * @brief 화살표/마커 시각 크기 비율 (타일 크기 기준)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path Arrow Scale", ClampMin = "0.01"))
	float mPathArrowScale = 0.5f;

	/**
	 * @brief 화살표/마커를 타일 위로 띄우는 Z 오프셋 (cm, z-fighting 방지)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Path", meta = (DisplayName = "Path Height Offset"))
	float mPathHeightOffset = 1.0f;

private:
	/**
	 * @brief 현재 Width/Height/TileSize에 맞춰 타일 인스턴스를 모두 재생성
	 */
	void RebuildTileInstances();

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
	 * @brief 현재 표시 중인 경로 타일 수 (틱 펄스 갱신 대상 판단 + 도착 마커 위상 인덱스용)
	 * @note PIE에 복제해서 볼 수 있도록 UPROPERTY 추가. 단, 저장할 필요는 없으므로 Transient 속성 부여
	 */
    UPROPERTY(Transient)
	int32 mPathLength = 0;

	/**
	 * @brief 경로 화살표/도착 마커의 펄스 알파를 매 프레임 재계산해 custom data에 기록
	 * @details 화살표는 인스턴스 순서(=경로 순서)에 위상차를 줘 경로를 따라 흐르게 한다.
	 */
	void RefreshPathPulse();

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
};
