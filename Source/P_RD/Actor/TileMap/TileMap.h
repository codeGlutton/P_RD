/*****************************************************************//**
 * @file   TileMap.h
 * @brief  타일맵 액터 정의 헤더
 * @author 이문환
 * @date   2026-05-26
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
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
class P_RD_API ATileMap : public AActor
{
	GENERATED_BODY()

public:
	ATileMap();

	// @brief 에디터 배치/스폰 시 타일 그리드 인스턴스 재생성
	virtual void OnConstruction(const FTransform& Transform) override;

	// @brief 매 프레임 Effect 하이라이트 펄스 갱신
	virtual void Tick(float DeltaSeconds) override;

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

#if WITH_EDITOR
	/**
	 * @brief [에디터 전용] 하이라이트 테스트 패턴을 칠해 시각 확인 (단독/겹침)
	 * @details 디테일 패널 버튼으로 호출. Aim 한 줄 + Select 한 칸 + Effect 몇 칸을 칠하되,
	 *          일부 칸은 겹치게 해서 단독/겹침 합성을 한 번에 확인한다.
	 */
	UFUNCTION(CallInEditor, Category = "SRPG")
	void DebugPaintTest();
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
};
