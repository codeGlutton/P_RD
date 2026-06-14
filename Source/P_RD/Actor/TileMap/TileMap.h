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
#include "Actor/TileMap/Tile.h"
#include "Actor/TileMap/TileLayer.h"
#include "Actor/TileMap/TileHighlight.h"
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

	/* 강조 표시 */
	/**
	 * @brief 지정한 타일들에 강조 상태를 설정 (해당 플래그 비트에 한해 치환)
	 * @details
	 * Flag의 각 비트에 대해, 그 비트를 가진 기존 타일에서 끄고 Tiles에만 켠다.
	 * 다른 플래그 비트는 보존하므로, Aim 위에 Select를 칠해도 Aim은 유지된다.
	 *
	 * @param[in] Tiles : 강조할 타일 목록 (맵 밖 좌표는 무시)
	 * @param[in] Flag  : 설정할 강조 상태
	 */
	void SetTileHighlight(const TArray<FTileIndex>& Tiles, ETileHighlightFlag Flag);

	/**
	 * @brief 지정한 강조 상태를 모든 타일에서 해제
	 * @details OR로 조합한 여러 비트를 한 번에 끌 수 있다 (예: Select | Effect).
	 * @param[in] Flag : 해제할 강조 상태 (비트 조합 가능)
	 */
	void ClearTileHighlight(ETileHighlightFlag Flag);

	/**
	 * 진입 액터가 해당 타일에 막히는지 검사하는 함수
	 * @param TileIndex 검사할 타일 인덱스
	 * @param Incoming 진입하려는 액터
	 * @return 막힘 여부 (맵 범위 밖은 막힘으로 간주)
	 */
	bool IsBlocked(const FTileIndex& TileIndex, const ITileActor* Incoming) const;

	/**
	 * 액터 움직임 시작 함수
	 * 일반 배치 함수와 달리, 움직임 애니메이션 처리 완료 타이밍에 따라서 CompleteActorMovement를 호출해줄 필요가 있음
	 * @param NextTransform 다음 타일 트랜스폼
	 * @param Actor 움직일 액터
	 * @pre NextTransform.mIndex가 맵 범위 안의 유효한 좌표여야 한다
	 * @pre 해당 타일이 Actor 레이어에 대해 막혀있지 않아야 한다 (호출 전 IsBlocked으로 확인)
	 * @warning 사전조건 위반 시 checkf로 중단됨
	 */
	void StartActorMovement(const FTileTransform& NextTransform, ITileActor* Actor);
	void CompleteActorMovement(ITileActor* Actor);

	/**
	 * 액터 초기 배치 함수
	 * @param NextTransform 다음 타일 트랜스폼
	 * @param Actor 배치할 액터
	 * @pre NextTransform.mIndex가 맵 범위 안의 유효한 좌표여야 한다
	 * @pre 해당 타일이 Actor 레이어에 대해 막혀있지 않아야 한다 (호출 전 IsBlocked으로 확인)
	 * @warning 사전조건 위반 시 checkf로 중단됨
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

	/* 타일 액터 조회 */
	/**
	 * @brief 타일에 있는 액터들을 레이어로 필터해 반환 (코어)
	 * @param[in] TileIndex   : 조회할 타일
	 * @param[in] LayerFilter : 포함할 레이어 (All이면 전체)
	 * @return TArray<TScriptInterface<ITileActor>> : 해당 타일의 액터 목록 (범위 밖이면 빈 배열)
	 */
	TArray<TScriptInterface<ITileActor>> GetActorsOnTile(const FTileIndex& TileIndex, ETileLayerFlag LayerFilter = ETileLayerFlag::All) const;

	/**
	 * @brief 코어의 타입 지정 버전 — T로 캐스트되는 액터만 배열로 반환
	 * @details T가 반환 타입에만 등장해 추론 불가 → 호출 시 <T> 명시해야 이 버전이 선택됨
	 * @param[in] TileIndex   : 조회할 타일
	 * @param[in] LayerFilter : 포함할 레이어 (기본 All — 타입 캐스트가 1차 필터라 보통 생략)
	 * @return TArray<T*> : T로 캐스트된 액터 목록
	 */
	template<typename T>
	TArray<T*> GetActorsOnTile(const FTileIndex& TileIndex, ETileLayerFlag LayerFilter = ETileLayerFlag::All) const
	{
		TArray<T*> Result;
		// 코어(비템플릿) 결과를 타입캐스트해 수집
		for (const TScriptInterface<ITileActor>& Actor : GetActorsOnTile(TileIndex, LayerFilter))
		{
			if (T* Typed = Cast<T>(Actor.GetObject()))
			{
				Result.Add(Typed);
			}
		}
		return Result;
	}

	/**
	 * @brief 타일 위 T 타입 액터 하나 반환 (없으면 nullptr)
	 * @details 타일당 1개 보장되는 액터(예: 유닛)용. 여러 개 가능하면 배열 버전 사용.
	 * @param[in] TileIndex   : 조회할 타일
	 * @param[in] LayerFilter : 포함할 레이어 (기본 All)
	 * @return T* : T로 캐스트된 첫 액터 또는 nullptr
	 */
	template<typename T>
	T* GetActorOnTile(const FTileIndex& TileIndex, ETileLayerFlag LayerFilter = ETileLayerFlag::All) const
	{
		// 코어 결과 중 T로 캐스트되는 첫 액터 반환
		for (const TScriptInterface<ITileActor>& Actor : GetActorsOnTile(TileIndex, LayerFilter))
		{
			if (T* Typed = Cast<T>(Actor.GetObject()))
			{
				return Typed;
			}
		}
		return nullptr;
	}

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

	/* 강조 표시 */
	// @brief 조준 범위 스타일 (우선순위 최하, 바닥에 깔림)
	UPROPERTY(EditAnywhere, Category = "TileMap|Highlight", meta = (DisplayName = "Aim Style"))
	FTileHighlightStyle mAimStyle;

	// @brief 선택 타일 스타일 (Aim 위에 덮어씀)
	UPROPERTY(EditAnywhere, Category = "TileMap|Highlight", meta = (DisplayName = "Select Style"))
	FTileHighlightStyle mSelectStyle;

	// @brief 영향 범위 스타일 (우선순위 최상, 펄스로 알파 변조)
	UPROPERTY(EditAnywhere, Category = "TileMap|Highlight", meta = (DisplayName = "Effect Style"))
	FTileHighlightStyle mEffectStyle;

	// @brief 펄스 강도 (Effect 알파에 곱하는 진동의 진폭, 0~1)
	UPROPERTY(EditAnywhere, Category = "TileMap|Highlight", meta = (DisplayName = "Pulse Intensity", ClampMin = "0.0", ClampMax = "1.0"))
	float mPulseIntensity = 0.5f;

	// @brief 펄스 주기 (초)
	UPROPERTY(EditAnywhere, Category = "TileMap|Highlight", meta = (DisplayName = "Pulse Period", ClampMin = "0.01"))
	float mPulsePeriod = 1.0f;

private:
	/**
	 * @brief ITileActor 포인터를 GC 추적용 TScriptInterface로 변환
	 * @note TScriptInterface는 UObject 핸들이 필요하므로 _getUObject로 변환한다
	 */
	static TScriptInterface<ITileActor> ToTileActorInterface(ITileActor* Actor);

	/**
	 * @brief 타일에 액터 등록 (mActors에 추가)
	 */
	void RegisterActorToTile(FTile* Tile, ITileActor* Actor);

	/**
	 * @brief 타일에서 액터 해제 (mActors에서 제거)
	 */
	void UnregisterActorFromTile(FTile* Tile, ITileActor* Actor);

	/**
	 * @brief 같은 타일의 다른 액터들과 양방향 OnBeginTileOverlap 통지 (자기 제외)
	 */
	void NotifyBeginOverlap(FTile* Tile, ITileActor* Actor);

	/**
	 * @brief 같은 타일의 다른 액터들과 양방향 OnEndTileOverlap 통지 (자기 제외)
	 */
	void NotifyEndOverlap(FTile* Tile, ITileActor* Actor);

	/**
	 * @brief 현재 Width/Height/TileSize에 맞춰 타일 인스턴스를 모두 재생성
	 */
	void RebuildTileInstances();

	/**
	 * @brief 타일 2차원 인덱스를 저장소/인스턴스의 1차원 인덱스로 변환 (y*Width + x)
	 * @return int32 : 유효 좌표면 1차원 인덱스, 범위 밖이면 INDEX_NONE
	 */
	int32 TileIndexToLinearIndex(const FTileIndex& TileIndex) const;

	/**
	 * @brief 인덱스로 타일 조회
	 * @return const FTile* : 범위 밖이면 nullptr
	 */
	const FTile* GetTile(const FTileIndex& TileIndex) const;

	/**
	 * @brief 인덱스로 타일 조회 (쓰기용 non-const 버전)
	 * @return FTile* : 범위 밖이면 nullptr
	 */
	FTile* GetTile(const FTileIndex& TileIndex);

	// @brief 타일 저장소 (크기 Width*Height, 인덱스 = y*Width + x)
	// FTile이 TScriptInterface<ITileActor>(UObject 참조)를 들고 있어 GC 추적용 UPROPERTY() 필수 (제거 금지)
	UPROPERTY()
	TArray<FTile> mTiles;
};
