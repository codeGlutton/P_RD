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
#include "Actor/TileMap/TileLayer.h"
#include "TileMap.generated.h"

class ITileActor;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

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

	/* 이동 / 범위 조회 */
	/**
	 * @brief 기준 좌표에서 이동 가능한 타일 목록 반환
	 * @details
	 * 경로 기반 도달성을 계산한다. 장애물·유닛으로 막힌 경로는 통과 불가.
	 * 거리 계산은 타일맵 내부 방식을 따른다. 점유된 타일은 도착 불가로 처리.
	 *
	 * @param[in] Origin : 기준 좌표
	 * @param[in] MoveDistance : 이동 거리 (1=인접 칸)
	 * @return TArray<FTileIndex> : 도달 가능한 타일 좌표 목록 (Origin 제외, 맵 밖 제외)
	 */
	TArray<FTileIndex> GetReachableTiles(const FTileIndex& Origin, int32 MoveDistance) const;

	/**
	 * @brief 기준 좌표에서 조준 가능한 타일 목록 반환
	 * @details
	 * Single 패턴은 기준 타일만 포함, 그 외 패턴은 기준 타일 제외.
	 * Range가 0이면 Single을 제외한 패턴은 빈 배열 반환.
	 *
	 * @param[in] Origin : 기준 좌표
	 * @param[in] Range : 사거리 (1=인접 칸, Single은 무시)
	 * @param[in] Pattern : 조준 패턴
	 * @param[in] bIncludeOccupied : 점유된 타일(장애물/유닛)을 조준 가능으로 포함할지
	 * @param[in] bIndirect : 곡사 여부 (장애물 너머 조준 가능한지)
	 * @return TArray<FTileIndex> : 조준 가능한 타일 좌표 목록 (맵 밖 좌표 제외)
	 */
	TArray<FTileIndex> GetAimableTiles(
		const FTileIndex& Origin,
		int32 Range,
		EAimPattern Pattern,
		bool bIncludeOccupied,
		bool bIndirect
	) const;

	/**
	 * @brief 스킬 발동 시 영향받는 타일 목록 반환
	 * @details
	 * Target을 중심으로 패턴에 따라 영향 타일을 계산한다.
	 * Beam 패턴은 Caster→Target 벡터로 방향을 결정한다.
	 * Size가 0이면 Target 한 칸만 반환. Single 패턴은 Size 무시.
	 *
	 * @param[in] Caster : 시전자 좌표 (Beam 패턴의 방향 계산용)
	 * @param[in] Target : 영향 범위 중심 좌표
	 * @param[in] Pattern : 영향 범위 패턴
	 * @param[in] Size : 범위 크기 (0=점, 이후 확장)
	 * @param[in] bPenetrate : 관통 여부 (장애물 너머 타일도 포함)
	 * @return TArray<FTileIndex> : 영향받는 타일 좌표 목록 (맵 밖 좌표 제외)
	 */
	TArray<FTileIndex> GetEffectTiles(
		const FTileIndex& Caster,
		const FTileIndex& Target,
		EEffectPattern Pattern,
		int32 Size,
		bool bPenetrate
	) const;

	/**
	 * 배치 가능한지 체크하는 함수
	 * @param TileIndex 배치할 지점 인덱스
	 * @param LayerFlag 배치 액터 Layer
	 * @return 가능 여부
	 */
	bool IsBlocking(const FTileIndex& TileIndex, ETileLayerFlag LayerFlag) const;

	/**
	 * 액터 움직임 시작 함수
	 * 일반 배치 함수와 달리, 움직임 애니메이션 처리 완료 타이밍에 따라서 CompleteActorMovement를 호출해줄 필요가 있음
	 * @param NextTransform 다음 타일 트랜스폼
	 * @param Actor 움직일 액터
	 */
	void StartActorMovement(const FTileTransform& NextTransform, ITileActor* Actor);
	void CompleteActorMovement(ITileActor* Actor);

	/**
	 * 액터 초기 배치 함수
	 * @param NextTransform 다음 타일 트랜스폼
	 * @param Actor 배치할 액터
	 */
	void PlaceActor(const FTileTransform& NextTransform, ITileActor* Actor);
	void RemoveActor(ITileActor* Actor);

	/* 타일맵 정보 */
	/**
	 * @brief 타일맵의 가로 길이 (X 방향 타일 개수)
	 */
	int32 GetWidth() const;

	/**
	 * @brief 타일맵의 세로 길이 (Y 방향 타일 개수)
	 */
	int32 GetHeight() const;

	/**
	 * @brief 타일 한 칸의 월드 크기 (cm): 기본값은 100cm
	 */
	float GetTileSize() const;

	/**
	 * @brief 타일 인덱스가 이 맵 범위 안의 유효한 좌표인지 검사
	 * @param[in] TileIndex : 검사할 타일 좌표
	 * @return bool : 0 <= X < Width && 0 <= Y < Height 이면 true
	 */
	bool IsValidIndex(const FTileIndex& TileIndex) const;

protected:
	// @brief 타일맵 가로 길이 (X 방향 타일 개수)
	UPROPERTY(EditAnywhere, Category = "TileMap", meta = (DisplayName = "Width", ClampMin = "1"))
	int32 mWidth = 9;

	// @brief 타일맵 세로 길이 (Y 방향 타일 개수)
	UPROPERTY(EditAnywhere, Category = "TileMap", meta = (DisplayName = "Height", ClampMin = "1"))
	int32 mHeight = 9;

	// @brief 타일 한 칸의 월드 크기 (cm)
	UPROPERTY(EditAnywhere, Category = "TileMap", meta = (DisplayName = "Tile Size", ClampMin = "1.0"))
	float mTileSize = 100.0f;

	/* 시각화 */
	// @brief 타일 그리드를 그리는 인스턴스드 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "TileMap|Visual", meta = (DisplayName = "Tile Mesh Component"))
	TObjectPtr<UInstancedStaticMeshComponent> mTileMeshComponent;

	// @brief 타일 한 칸에 사용할 메시 (기본: 엔진 Plane)
	UPROPERTY(EditAnywhere, Category = "TileMap|Visual", meta = (DisplayName = "Tile Mesh"))
	TObjectPtr<UStaticMesh> mTileMesh;

	// @brief 타일 메시에 덮어쓸 머티리얼 (null이면 메시 기본 머티리얼)
	UPROPERTY(EditAnywhere, Category = "TileMap|Visual", meta = (DisplayName = "Tile Material"))
	TObjectPtr<UMaterialInterface> mTileMaterial;

	// @brief 타일 시각 크기 비율 (1.0 미만이면 타일 사이에 틈이 생겨 격자선처럼 보임)
	UPROPERTY(EditAnywhere, Category = "TileMap|Visual", meta = (DisplayName = "Tile Visual Scale", ClampMin = "0.1", ClampMax = "1.0"))
	float mTileVisualScale = 0.95f;

private:
	/**
	 * @brief 현재 Width/Height/TileSize에 맞춰 타일 인스턴스를 모두 재생성
	 */
	void RebuildTileInstances();
};
