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
	 * @param[in] bIndirect : 곡사 여부 (장애물/유닛 너머 조준 가능한지)
	 * @param[in] Incoming : 교체할 액터. 교체가 없을 경우는 nullptr
	 * @return TArray<FTileIndex> : 조준 가능한 타일 좌표 목록 (맵 밖 좌표 제외)
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

	/**
	 * 진입 액터를 해당 타일에 배치할 수 있는지 검사하는 함수
	 * @details 막히지 않았거나, 막혔어도 기존 액터를 교체할 수 있으면 배치 가능
	 * @param TileIndex 검사할 타일 인덱스
	 * @param Incoming 진입하려는 액터
	 * @return 배치 가능 여부 (맵 범위 밖은 불가)
	 */
	bool CanPlace(const FTileIndex& TileIndex, const ITileActor* Incoming) const;

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
	/**
	 * @brief 타일맵 가로 길이 (X 방향 타일 개수)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG", meta = (DisplayName = "Width", ClampMin = "1"))
	int32 mWidth = 9;

	/**
	 * @brief 타일맵 세로 길이 (Y 방향 타일 개수)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG", meta = (DisplayName = "Height", ClampMin = "1"))
	int32 mHeight = 9;

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
	 * @brief 조준 범위 스타일 (우선순위 최하, 바닥에 깔림)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Highlight", meta = (DisplayName = "Aim Style"))
	FTileHighlightStyle mAimStyle;

	/**
	 * @brief 선택 타일 스타일 (Aim 위에 덮어씀)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Highlight", meta = (DisplayName = "Select Style"))
	FTileHighlightStyle mSelectStyle;

	/**
	 * @brief 영향 범위 스타일 (우선순위 최상, 펄스로 알파 변조)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Highlight", meta = (DisplayName = "Effect Style"))
	FTileHighlightStyle mEffectStyle;

	/**
	 * @brief 펄스 강도 (Effect 알파에 곱하는 진동의 진폭, 0~1)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Highlight", meta = (DisplayName = "Pulse Intensity", ClampMin = "0.0", ClampMax = "1.0"))
	float mPulseIntensity = 0.8f;

	/**
	 * @brief 펄스 주기 (초)
	 */
	UPROPERTY(EditAnywhere, Category = "SRPG|Highlight", meta = (DisplayName = "Pulse Period", ClampMin = "0.01"))
	float mPulsePeriod = 1.0f;

private:
	/**
	 * 진입 액터가 해당 타일에 막히는지 검사하는 함수
	 * @param TileIndex 검사할 타일 인덱스
	 * @param Incoming 진입하려는 액터
	 * @return 막힘 여부 (맵 범위 밖은 막힘으로 간주)
	 */
	bool IsBlocked(const FTileIndex& TileIndex, const ITileActor* Incoming) const;

	/**
	 * 진입 액터가 덮어쓸(교체할) 기존 액터들을 반환하는 함수
	 * @details 기존 액터가 ReplaceLayerFlags로 진입자 레이어를 교체 허용하고, 진입자의 Overlay 우선순위가 같거나 더 높은 경우만 수집한다.
	 * @param TileIndex 검사할 타일 인덱스
	 * @param Incoming 진입하려는 액터
	 * @return 교체 대상 액터 목록 (빈 배열이면 교체 대상 없음)
	 */
	TArray<TScriptInterface<ITileActor>> GetReplaceableActors(const FTileIndex& TileIndex, const ITileActor* Incoming) const;

	/**
	 * @brief 코어의 타입 지정 버전 — T로 캐스트되는 교체 대상만 배열로 반환
	 * @details T가 반환 타입에만 등장해 추론 불가 → 호출 시 <T> 명시해야 이 버전이 선택됨
	 * @param[in] TileIndex 검사할 타일 인덱스
	 * @param[in] Incoming  진입하려는 액터
	 * @return TArray<T*> : T로 캐스트된 교체 대상 목록
	 */
	template<typename T>
	TArray<T*> GetReplaceableActors(const FTileIndex& TileIndex, const ITileActor* Incoming) const
	{
		TArray<T*> Result;
		// 코어(비템플릿) 결과를 타입캐스트해 수집
		for (const TScriptInterface<ITileActor>& Actor : GetReplaceableActors(TileIndex, Incoming))
		{
			if (T* Typed = Cast<T>(Actor.GetObject()))
			{
				Result.Add(Typed);
			}
		}
		return Result;
	}

	/* 범위 계산 헬퍼 */
	/**
	 * @brief 원점에서 특정 방향으로 Range만큼 뻗는 직선에 포함되는 타일을 수집
	 * @details
	 * 한 칸씩 전진하며 Out에 누적하고, 맵 밖으로 나가면 그 방향을 종료한다. 원점 자신은 포함하지 않는다.
	 * Cross(4방향)·Star(8방향)처럼 여러 방향을 각각 호출해 같은 배열에 누적하는 방식이다.
	 * @param[in] Origin 시작 좌표
	 * @param[in] Step   한 칸 전진 방향 (예: (1,0)=오른쪽, (-1,1)=좌하단 대각)
	 * @param[in] Range  뻗을 칸 수 (0 이하이면 아무것도 추가하지 않음)
	 * @param[in,out] Out 결과를 누적할 배열
	 */
	void AppendRayTiles(const FTileIndex& Origin, const FTileIndex& Step, int32 Range, TArray<FTileIndex>& Out) const;

	/**
	 * @brief 원점에서 특정 방향으로 Range만큼 뻗되, 관통하지 않으면 점유 칸에서 멈추는 직선을 수집
	 * @details
	 * AppendRayTiles와 달리, bPenetrate가 false면 장애물/유닛(IsOccupied) 칸을 만났을 때
	 * 그 칸까지 포함한 뒤 더 진행하지 않는다(맞고 멈춤). true면 점유와 무관하게 Range 한도까지 진행한다.
	 * 영향범위 Cross/Star/Beam 패턴용. 원점 자신은 포함하지 않는다.
	 * @param[in] Origin 시작 좌표
	 * @param[in] Step   한 칸 전진 방향 (예: (1,0)=오른쪽, (-1,1)=좌하단 대각)
	 * @param[in] Range  뻗을 칸 수 (0 이하이면 아무것도 추가하지 않음)
	 * @param[in] bPenetrate 관통 여부 (false면 점유 칸에서 멈춤)
	 * @param[in,out] Out 결과를 누적할 배열
	 */
	void AppendBlockableRay(const FTileIndex& Origin, const FTileIndex& Step, int32 Range, bool bPenetrate, TArray<FTileIndex>& Out) const;

	/**
	 * @brief 타일에 장애물 또는 유닛이 있는 지 검사
	 * @details
	 * 이동범위(통과·도착 차단)와 영향범위(직선 차단) 계산에 쓰인다.
	 * CanPlace()와 달리 비교하는 액터가 없으므로 교체가 없으며 단순 물리적인 방해만 판정한다.
	 * @param[in] TileIndex 검사할 좌표
	 * @return 장애물/유닛이 있으면 true, 없거나 맵 밖이면 false
	 */
	bool IsOccupied(const FTileIndex& TileIndex) const;

	/**
	 * @brief 중심 기준 반지름 이내의 모든 타일 수집
	 * @details
	 * 체비셰프 방식으로 계산
	 * 중심 포함 여부는 호출하는쪽에서 처리하므로(조준:제외, 영향:포함) 헬퍼는 중심을 제외하고 계산
	 * @param[in] Center 중심 좌표
	 * @param[in] Radius 반경 (체비셰프 거리, 0 이하이면 아무것도 추가하지 않음)
	 * @param[in,out] Out 결과를 누적할 배열
	 */
	void AppendSquareTiles(const FTileIndex& Center, int32 Radius, TArray<FTileIndex>& Out) const;

	/**
	 * @brief Bresenham 알고리즘으로 두 칸 사이 직선이 지나는 타일을 수집
	 * @details
	 * 양 끝(From, To)을 모두 포함하며, From을 첫 원소·To를 마지막 원소로 채운다.
	 * 내부에서 정규화된 한 방향으로만 그리므로 From/To를 바꿔 호출해도 같은 칸 집합이 나온다 (순서만 반대).
	 * @param[in] From 시작 좌표
	 * @param[in] To   끝 좌표
	 * @param[in,out] Out 결과를 누적할 배열
	 */
	void BresenhamLine(const FTileIndex& From, const FTileIndex& To, TArray<FTileIndex>& Out) const;

	/**
	 * @brief 두 칸 사이 직선을 래스터화해 지나는 타일을 수집 (래스터화 방식의 교체 지점)
	 * @details
	 * 현재는 Bresenham을 사용한다. Supercover 등 다른 방식으로 바꾸려면 이 함수의 내부 호출만 교체하면 된다.
	 * 호출자가 양 끝을 인덱스로 구분하므로, From을 첫 원소·To를 마지막 원소로 채우는 계약을 지켜야 한다.
	 * @param[in] From 시작 좌표
	 * @param[in] To   끝 좌표
	 * @param[in,out] Out 결과를 누적할 배열
	 */
	void RasterizeLine(const FTileIndex& From, const FTileIndex& To, TArray<FTileIndex>& Out) const;

	/**
	 * @brief From에서 To까지 시야(직선)가 막히지 않는지 판정
	 * @details
	 * RasterizeLine으로 경로 칸을 구한 뒤, 양 끝(From, To)을 제외한 중간 칸에
	 * 시야를 막는 액터(Obstacle 또는 Unit)가 하나라도 있으면 막힌 것으로 본다.
	 * @param[in] From 시작 좌표
	 * @param[in] To   목표 좌표
	 * @return 시야가 확보되면 true, 중간이 막히면 false
	 */
	bool HasLineOfSight(const FTileIndex& From, const FTileIndex& To) const;

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

	/**
	 * @brief 한 타일의 강조 상태(mHighlights)를 스타일로 합성해 ISM custom data에 기록
	 * @details
	 * 활성 레이어를 priority 오름차순으로 blend(Mix=알파 over, Overwrite=덮어쓰기)하고,
	 * Effect는 알파에 펄스 계수를 곱한다. 슬롯 0~2=알파 곱해진(프리멀티) RGB, 3=커버리지 알파.
	 * @param[in] LinearIndex 타일/인스턴스 1차원 인덱스
	 */
	void RefreshTileCustomData(int32 LinearIndex);

	/**
	 * @brief 타일 저장소 (크기 Width*Height, 인덱스 = y*Width + x)
	 * @warning FTile이 TScriptInterface<ITileActor>(UObject 참조)를 들고 있어 GC 추적용 UPROPERTY() 필수 (제거 금지)
	 */
	UPROPERTY()
	TArray<FTile> mTiles;

	/**
	 * @brief 타일별 강조 표시 상태 (시각 전용 — 전투/시뮬레이션과 무관, FTile과 분리)
	 * @details mTiles와 같은 1차원 인덱싱(y*Width+x), 크기 Width*Height. ISM custom data로 화면에 반영된다.
	 */
	UPROPERTY()
	TArray<ETileHighlightFlag> mHighlights;
};
