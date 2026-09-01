/*****************************************************************//**
 * @file   TileMapModel.h
 * @brief  타일맵 데이터 모델 클래스 정의 헤더
 * @author 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/TileMap/TileLayer.h"
#include "Actor/TileMap/TileHighlight.h"
#include "TileMapModel.generated.h"

class UBoardActorModel;
class UStaticUnitSkillData;
struct FPresentationBarrier;

/**
 * @brief  SRPG의 타일
 */
USTRUCT()
struct FTile
{
	GENERATED_BODY()

	/**
	 * @brief 타일맵 뷰/모델이 타일 내부 액터 목록/블로킹 정보에 접근 (전환기 동안 양쪽)
	 */
	friend class ATileMap;
	friend class UTileMapModel;

protected:
	/**
	 * @brief 배치된 보드 액터 (소유권은 외부 컨텍스트/팩토리, 약참조)
	 */
	UPROPERTY(Category = BoardActor, VisibleAnywhere, meta = (DisplayName = "BoardActors"))
	TArray<TWeakObjectPtr<UBoardActorModel>> mBoardActors;
};

/**
 * @brief 이동경로 표시용 타일정보
 * @details 타일좌표와 경유지 여부로 구성
 */
USTRUCT(BlueprintType)
struct FMovePathTile
{
	GENERATED_BODY()

public:
	FMovePathTile() = default;
	FMovePathTile(const FTileIndex& InIndex, bool bInWaypoint = false)
		: mIndex(InIndex), mIsWaypoint(bInWaypoint) {}

	// @brief 타일 인덱스
	UPROPERTY(Category = "MovePathTile", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Index"))
	FTileIndex mIndex;
	// @brief 경유지 여부
	UPROPERTY(Category = "MovePathTile", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Is Waypoint"))
	bool mIsWaypoint = false;
};

/**
 * @brief 모델이 뷰(ATileMap)의 시각/컴포넌트 정보가 필요한 좌표 변환을 질의하기 위한 델리깃들
 * @details single-cast RetVal — 반환값을 받아야 하므로 멀티캐스트 불가.
 *          non-dynamic이라 UPROPERTY가 아니며 직렬화/네트워크 복제 안 됨(런타임 바인딩 전용).
 */
DECLARE_DELEGATE_RetVal_OneParam(FTransform, FTileToWorldTransformDelegate, const FTileTransform&);
DECLARE_DELEGATE_RetVal_OneParam(FVector, FTileToWorldLocationDelegate, const FTileIndex&);
DECLARE_DELEGATE_RetVal_OneParam(FTileIndex, FWorldToTileIndexDelegate, const FVector&);

/**
 * @brief 모델이 뷰(ATileMap)에 이동경로 표시를 요청하는 델리깃
 * @details 좌표 델리깃과 동일하게 non-dynamic — 라이브에서 뷰가 SetMovePath로 바인딩하고, 미바인딩(심)이면 표시되지 않는다.
 *          빈 배열을 넘기면 표시 해제(SetMovePath의 빈 배열 처리)와 같다.
 */
DECLARE_DELEGATE_OneParam(FSetMovePathDelegate, const TArray<FMovePathTile>&);

/**
 * @brief 모델이 뷰(ATileMap)에 타일 강조(하이라이트) 표시/해제를 요청하는 델리깃
 * @details 경로 델리깃과 동일하게 non-dynamic — 라이브에서 뷰가 SetTileHighlight/ClearTileHighlight로
 *          바인딩하고, 미바인딩(심 복제본)이면 표시되지 않는다.
 */
DECLARE_DELEGATE_TwoParams(FSetTileHighlightDelegate, const TArray<FTileIndex>&, ETileHighlightFlag);
DECLARE_DELEGATE_OneParam(FClearTileHighlightDelegate, ETileHighlightFlag);

/**
 * @brief 모델이 뷰(ATileMap)에 위협 범위 표시/해제를 요청하는 델리깃
 * @details 강조 델리깃과 동일하게 non-dynamic — 라이브에서 뷰가 SetThreatRange/ClearThreatRange로
 *          바인딩하고, 미바인딩(심 복제본)이면 표시되지 않는다.
 */
DECLARE_DELEGATE_TwoParams(FSetThreatRangeDelegate, const TArray<FTileIndex>&, const TArray<FTileIndex>&);
DECLARE_DELEGATE(FClearThreatRangeDelegate);

/**
 * @brief  타일맵 데이터 모델 클래스
 * @details 보드 액터들이 배치되는 타일맵의 데이터 모델이다.
 * @warning UCLASS(EditInlineNew)는 ATileMap이 이 모델을 Instanced UPROPERTY로 인라인 편집(크기 확인·변경)하기 위한 것.
 *          제거하면 액터 디테일 패널에서 모델 내부 프로퍼티 편집이 불가하므로 변경 금지.
 */
UCLASS()
class P_RD_API UTileMapModel : public UObjectModel
{
	GENERATED_BODY()

	/* UObjectModel 상속 */
public:
	/**
	 * @brief 모델 생성 시 타일 저장소(mTiles)를 채운다.
	 * @details mTiles의 소유자는 모델이므로, 뷰(ATileMap) 생성 여부나 시뮬레이션 모드와 무관하게
	 *          생성 시점에 직접 RebuildTiles로 Width*Height 크기의 기본 타일을 채운다.
	 */
	void Initialize() override;

public:
	/**
	 * @brief 타일맵의 가로 길이 (X 방향 타일 개수)
	 */
	int32 GetWidth() const;

	/**
	 * @brief 타일맵의 세로 길이 (Y 방향 타일 개수)
	 */
	int32 GetHeight() const;

	/**
	 * @brief 타일 인덱스가 이 맵 범위 안의 유효한 좌표인지 검사
	 * @param[in] TileIndex : 검사할 타일 좌표
	 * @return bool : 0 <= X < Width && 0 <= Y < Height 이면 true
	 */
	bool IsValidIndex(const FTileIndex& TileIndex) const;

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
	 * @brief 타일 2차원 인덱스를 저장소의 1차원 인덱스로 변환 (y*Width + x)
	 * @details 뷰의 mHighlights 등 같은 인덱싱을 공유하는 병렬 데이터도 이걸로 변환해 일관성 유지
	 * @return int32 : 유효 좌표면 1차원 인덱스, 범위 밖이면 INDEX_NONE
	 */
	int32 TileIndexToLinearIndex(const FTileIndex& TileIndex) const;

	/**
	 * @brief 격자 크기(Width/Height) 설정 후 타일 저장소 재생성
	 * @details 1 미만은 1로 보정(ClampMin=1 정책과 동일). 크기 변경이 곧 저장소 크기 변경이라 RebuildTiles까지 수행.
	 * @param[in] Width  가로 칸 수
	 * @param[in] Height 세로 칸 수
	 */
	void SetDimensions(int32 Width, int32 Height);

	/**
	 * @brief 현재 Width/Height에 맞춰 타일 저장소(mTiles)를 기본 타일로 재생성
	 */
	void RebuildTiles();

	/**
	 * @brief From 타일에서 To 타일을 바라볼 때의 방향을 계산
	 * @details
	 * 이동 스텝마다 유닛이 진행 방향을 바라보게 하는 데 사용.
	 * @param Fallback 제자리거나 방향을 정할 수 없을 때 폴백 방향
	 */
	static ETileActorDirection TileDeltaToDirection(const FTileIndex& From, const FTileIndex& To, ETileActorDirection Fallback);

	/**
	 * @brief From->To 벡터 방향으로 하나 더 나아갈 때 8방향(직각/대각)에 가까운 방향으로 양자화 해서 전진
	 * @details
	 * 벡터가 직각/대각이 아닌 타일에서 한 개 더 나아갈 때 어떤 타일이 되는 지 계산
	 * 미는 방향이 8방향이 아니므로 가까운 직각/대각 선분 방향으로 미는 걸로 처리
	 * 예) (0,0)->(3,1) 타일에서 한 단계 더 나가면 (4,1)인가? (4,2)인가?
	 *   고민하지 말고, 벡터((0,0)->(3.1))은 약 18도니까 직각 선분과 가까움
	 *   따라서 직각의 진행방향을 따라해서 다음 타일은 (4,1)이 됨
	 * @return FTileIndex 단위 스텝. 최종 위치가 아님
	 *   양자화 된 단위니까 이걸 기존 타일인덱스에 더해주면 다음 스텝의 위치가 됨
	 */
	static FTileIndex TileDeltaToStep(const FTileIndex& From, const FTileIndex& To);

	/**
	 * @brief 방향 enum을 타일 한 칸 스텝으로 변환
	 * @details 함정처럼 미는 쪽과 밀리는 쪽이 같은 타일이면 델타 벡터로 방향을 계산할 수 없으므로,
	 *          고정 방향 enum에서 스텝을 얻을 때 사용
	 */
	static FTileIndex DirectionToTileStep(ETileActorDirection Direction);

	/* 좌표 변환 (뷰 질의) */
	/**
	 * @brief 타일 트랜스폼(인덱스+방향)을 월드 트랜스폼으로 변환 — 뷰에 질의
	 * @details 시각/컴포넌트 정보가 필요해 바인딩된 뷰 델리깃에 위임한다.
	 *          미바인딩(심 복제본 등)이면 FTransform::Identity 반환.
	 */
	FTransform TileToWorldTransform(const FTileTransform& TileTransform) const;

	/**
	 * @brief 타일 인덱스를 월드 위치로 변환 — 뷰에 질의 (미바인딩이면 FVector::ZeroVector)
	 */
	FVector TileToWorldLocation(const FTileIndex& TileIndex) const;

	/**
	 * @brief 월드 위치를 타일 인덱스로 변환 — 뷰에 질의 (미바인딩이면 FTileIndex::Invalid)
	 */
	FTileIndex WorldToTileIndex(const FVector& WorldLocation) const;

	/**
	 * @brief 좌표 변환 질의 델리깃 (라이브에서 ATileMap이 자기 함수로 바인딩)
	 * @details non-dynamic이라 직렬화/복제되지 않음 — 심 복제본에선 미바인딩으로 남아 시각 질의가 무력화된다.
	 */
	FTileToWorldTransformDelegate mTileToWorldTransformDelegate;
	FTileToWorldLocationDelegate mTileToWorldLocationDelegate;
	FWorldToTileIndexDelegate mWorldToTileIndexDelegate;

	/**
	 * @brief 이동경로 표시 요청 델리깃 (라이브에서 ATileMap이 SetMovePath로 바인딩, 미바인딩=심이면 표시 없음)
	 */
	FSetMovePathDelegate mSetMovePathDelegate;

	/**
	 * @brief 타일 강조 표시/해제 요청 델리깃 (라이브에서 ATileMap이 SetTileHighlight/ClearTileHighlight로 바인딩, 미바인딩=심이면 표시 없음)
	 */
	FSetTileHighlightDelegate mSetTileHighlightDelegate;
	FClearTileHighlightDelegate mClearTileHighlightDelegate;

	/**
	 * @brief 위협 범위 표시/해제 요청 델리깃 (라이브에서 ATileMap이 SetThreatRange/ClearThreatRange로 바인딩, 미바인딩=심이면 표시 없음)
	 */
	FSetThreatRangeDelegate mSetThreatRangeDelegate;
	FClearThreatRangeDelegate mClearThreatRangeDelegate;

	/* 이동 / 범위 조회 */
	/**
	 * @brief 기준 좌표에서 이동 가능한 타일 목록 반환
	 * @details
	 * 경로 기반 도달성을 계산한다. 장애물·유닛으로 막힌 경로는 통과 불가.
	 * 거리 계산은 타일맵 내부 방식을 따른다. 점유된 타일은 도착 불가로 처리.
	 *
	 * @param[in] Origin : 기준 좌표
	 * @param[in] MoveDistance : 이동 거리 (1=인접 칸)
	 * @param[in] IgnoreBlocker : 점유 판정에서 제외할 액터 (이동 빌드 중 자기 출발 타일 복귀 허용 등). 없으면 nullptr
	 * @return TArray<FTileIndex> : 도달 가능한 타일 좌표 목록 (Origin 제외, 맵 밖 제외)
	 */
	TArray<FTileIndex> GetReachableTiles(const FTileIndex& Origin, int32 MoveDistance, const UBoardActorModel* IgnoreBlocker = nullptr) const;

	/**
	 * @brief 시작→목표 최단 이동경로 계산 (장애물/유닛 회피)
	 * @details
	 * 도달 범위 계산과 동일한 4방향(직교) BFS 규칙 사용
	 * 도착지에 점유하는 게 있으면 도착 불가하지만 파라미터로 무시할 수 있음
	 * 시작 칸은 유닛 존재 허용
	 * @param[in] Start : 시작 좌표
	 * @param[in] Goal  : 목표 좌표
	 * @param[in] IgnoreBlocker : 점유 판정에서 제외할 액터 (이동 빌드 중 자기 출발 타일 복귀 허용 등). 없으면 nullptr
	 * @param[in] bAllowOccupiedGoal : 도착지에 점유하는 게 있어도 도착 허용할지 여부
	 * @return TArray<FTileIndex> : Start부터 Goal까지 순서대로의 타일 목록(양 끝 포함). 경로가 없으면 빈 배열
	 */
	TArray<FTileIndex> FindPath(const FTileIndex& Start, const FTileIndex& Goal, const UBoardActorModel* IgnoreBlocker = nullptr, bool bAllowOccupiedGoal = false) const;

	/**
	 * @brief 목표지점을 기준으로 모든 타일의 경로거리를 계산
	 * @details
	 * 장애물이나 유닛 등으로 인해 스킬 사용이 안될 시 플레이어에게 접근해야 하는데,
	 * 단순 도달 범위로는 잡히지 않는 우회하는 비용도 계산해서 표를 작성한 후,
	 * 가장 비용이 적은 타일로 이동
	 * @param[in] Target : 거리 기준이 되는 목표 좌표
	 * @param[in] IgnoreBlocker : 통과 판정에서 제외할 액터 (자리를 비울 예정인 유닛 등). 없으면 nullptr
	 * @return TArray<int32> : 칸별 최단 거리 (크기 Width*Height, 인덱스는 TileIndexToLinearIndex 규칙, 도달 불가면 -1)
	 */
	TArray<int32> GetDistanceField(const FTileIndex& Target, const UBoardActorModel* IgnoreBlocker = nullptr) const;

	/**
	 * @brief 시작→목표 경로를 계산해 뷰에 표시 요청 (FindPath + 뷰 표시 델리깃 호출)
	 * @details 미바인딩(심 복제본)이면 아무 일도 하지 않는다. 경로가 없으면 빈 경로라 표시 해제와 같다.
	 * @param[in] Start : 시작 좌표
	 * @param[in] Goal  : 목표 좌표
	 */
	void SetMovePath(const FTileIndex& Start, const FTileIndex& Goal);

	/**
	 * @brief 경유지 여부가 포함된 표시용 경로를 뷰에 그대로 전달
	 * @details 호출자가 경로와 경유지 플래그를 직접 구성하는 경우 사용 (경유지 이동 빌드)
	 * @param[in] PathTiles : 표시용 경로 타일 목록
	 */
	void SetMovePath(const TArray<FMovePathTile>& PathTiles);

	/**
	 * @brief 이동경로 표시 해제 요청 (빈 경로를 뷰에 전달)
	 */
	void ClearMovePath();

	/* 타일 강조 */
	/**
	 * @brief 지정 타일들을 강조 상태로 표시 요청 (뷰의 SetTileHighlight로 위임)
	 * @details 미바인딩(심 복제본)이면 아무 일도 하지 않는다.
	 * @param[in] Tiles : 강조할 타일 목록
	 * @param[in] Flag  : 설정할 강조 상태(Aim/Select/Effect)
	 */
	void SetTileHighlight(const TArray<FTileIndex>& Tiles, ETileHighlightFlag Flag);

	/**
	 * @brief 지정한 강조 상태 해제 요청 (뷰의 ClearTileHighlight로 위임)
	 * @param[in] Flag : 해제할 강조 상태 (비트 조합 가능)
	 */
	void ClearTileHighlight(ETileHighlightFlag Flag);

	/* 위협 범위 표시 */
	/**
	 * @brief 적 위협 범위 표시 요청 (뷰의 SetThreatRange로 위임)
	 * @details 미바인딩(심 복제본)이면 아무 일도 하지 않는다. 빈 배열일 경우 그리지 않음
	 * @param[in] MoveTiles : 최대 이동 범위 타일 목록
	 * @param[in] AttackTiles : 최대 공격 범위 타일 목록
	 */
	void SetThreatRange(const TArray<FTileIndex>& MoveTiles, const TArray<FTileIndex>& AttackTiles);

	/**
	 * @brief 적 위협 범위 표시 해제 요청 (뷰의 ClearThreatRange로 위임)
	 */
	void ClearThreatRange();

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
	 * @param[in] BlockerLayers : 조준 시야를 막는 레이어 (None이면 아무것도 조준을 막지 않음)
	 * @param[in] Incoming : 교체할 액터. 교체가 없을 경우는 nullptr
	 * @param[in] IgnoreBlocker : 시야 차폐에서 제외할 액터 (이동 후 위치 평가처럼 자리를 비울 예정인 유닛). 없으면 nullptr
	 * @return TArray<FTileIndex> : 조준 가능한 타일 좌표 목록 (맵 밖 좌표 제외)
	 */
	TArray<FTileIndex> GetAimableTiles(
		const FTileIndex& Origin,
		int32 Range,
		EAimPattern Pattern,
		bool bIncludeOccupied,
		ETileLayerFlag BlockerLayers,
		const UBoardActorModel* Incoming = nullptr,
		const UBoardActorModel* IgnoreBlocker = nullptr
	) const;

	/**
	 * @brief Origin 타일에서 Target 타일을 조준 가능한 지 판정
	 * @param[in] Origin : 기준 좌표
	 * @param[in] Target : 조준 대상 좌표
	 * @param[in] Range : 사거리 (1=인접 칸, Single은 무시)
	 * @param[in] Pattern : 조준 패턴
	 * @param[in] bIncludeOccupied : 점유된 타일(장애물/유닛)을 조준 가능으로 포함할지
	 * @param[in] BlockerLayers : 조준 시야를 막는 레이어 (None이면 아무것도 조준을 막지 않음)
	 * @param[in] Incoming : 교체할 액터. 교체가 없을 경우는 nullptr
	 * @param[in] IgnoreBlocker : 시야 차폐에서 제외할 액터 (이동 후 위치 평가처럼 자리를 비울 예정인 유닛). 없으면 nullptr
	 * @return bool : 조준 가능하면 true
	 */
	bool CanAim(
		const FTileIndex& Origin,
		const FTileIndex& Target,
		int32 Range,
		EAimPattern Pattern,
		bool bIncludeOccupied,
		ETileLayerFlag BlockerLayers,
		const UBoardActorModel* Incoming = nullptr,
		const UBoardActorModel* IgnoreBlocker = nullptr
	) const;

	/**
	 * @brief 시전 타일에서부터 조준 타일로 가면서 타겟 패턴을 사용해서 타겟이 되는 타일들을 수집
	 * @details
	 * 수집된 타일들은 각각 영향 범위의 중심 타일이 됨. 전체 영향 범위를 계산할 때 중복은 호출하는 쪽에서 처리.
	 * TargetOnly 패턴은 조준 타일 한 칸만 포함.
	 * LineToTarget 패턴은 시전자→조준 타일 직선이 지나는 칸들을 시전자 제외,
	 * 가까운 순으로 포함. 조준 타일은 어떤 패턴에서든 항상 포함.
	 *
	 * @param[in] Caster : 시전자 좌표
	 * @param[in] Target : 조준 타일 좌표
	 * @param[in] Pattern : 타겟 범위 패턴
	 * @return TArray<FTileIndex> : 타겟 타일 좌표 목록 (시전자에서 가까운 순)
	 */
	TArray<FTileIndex> GetTargetTiles(
		const FTileIndex& Caster,
		const FTileIndex& Target,
		ETargetPattern Pattern
	) const;

	/**
	 * @brief 스킬 발동 시 영향받는 타일 목록 반환
	 * @details
	 * Target을 중심으로 패턴에 따라 영향 타일을 계산한다.
	 * Size가 0이면 Target 한 칸만 반환. Single 패턴은 Size 무시.
	 *
	 * @param[in] Target : 영향 범위 중심 좌표
	 * @param[in] Pattern : 영향 범위 패턴
	 * @param[in] Size : 범위 크기 (0=점, 이후 확장)
	 * @param[in] BlockerLayers : 영향 확산을 막는 레이어 (None이면 아무것도 확산을 막지 않음)
	 * @param[in] IgnoreBlocker : 확산 차단에서 제외할 액터 (이동 후 위치 평가처럼 자리를 비울 예정인 유닛). 없으면 nullptr
	 * @return TArray<FTileIndex> : 영향받는 타일 좌표 목록 (맵 밖 좌표 제외)
	 */
	TArray<FTileIndex> GetEffectTiles(
		const FTileIndex& Target,
		EEffectPattern Pattern,
		int32 Size,
		ETileLayerFlag BlockerLayers,
		const UBoardActorModel* IgnoreBlocker = nullptr
	) const;

	/**
	 * @brief 적 위협 범위(최대이동범위/최대공격범위) 계산
	 * @details
	 * 최대이동범위: 행동력 전부를 이동에 쓸 때 도달 가능한 타일 + 원점
	 * 최대공격범위: "이동비용 + 스킬 시전비용 ≤ 행동력"인 타일에서 각 스킬로 조준 가능한 타일의 합집합
	 * 조준 가능한 타일만 포함하며 이펙트 확산은 포함하지 않음
	 * @param[in] Origin : 적의 현재 타일
	 * @param[in] ActionPoint : 이동과 스킬 시전이 나눠 쓰는 행동력
	 * @param[in] Skills : 스킬 데이터 목록 (빈 슬롯 nullptr 허용, 쿨다운 제외는 호출부 책임)
	 * @param[in] Self : 계획 주체 (이동으로 자리를 비울 예정이므로 통과/차폐 판정에서 제외)
	 * @param[out] MoveTiles : 최대 이동 범위 타일 목록
	 * @param[out] AttackTiles : 최대 공격 범위 타일 목록
	 */
	void GetThreatRanges(
		const FTileIndex& Origin,
		int32 ActionPoint,
		const TArray<const UStaticUnitSkillData*>& Skills,
		const UBoardActorModel* Self,
		OUT TArray<FTileIndex>& MoveTiles,
		OUT TArray<FTileIndex>& AttackTiles
	) const;

	/**
	 * @brief 밀치기 시 밀려나는 경로 계산
	 * @details
	 * - Pusher와 Pushed를 밀었을 때 밀려나는 경로를 가까운곳부터 먼곳 순서로 배열로 저장
	 * @note
	 * - Pusher->Pushed 벡터 방향으로 미는 게 원칙
	 * - 만약, 직각/대각 방향이 아닌 경우 가까운 직각/대각 방향을 사용해서 민다.
	 * - Pusher->Pushed 사이에 장애물이 시야를 가리면 밀지 못함 (유닛은 관통)
	 * - Pushed 뒤에 장애물/유닛이 있으면 밀리지 않음
	 * @param[in] Pusher 미는 쪽 좌표 (방향 계산용)
	 * @param[in] Pushed 밀리는 쪽 좌표 (전진 시작점)
	 * @param[in] MaxDistance 최대 밀치기 칸 수
	 * @return TArray<FTileIndex> 밀리는 위치부터 도착까지의 경로. 밀리지 않으면 Pushed 위치 한 개
	 */
	TArray<FTileIndex> GetPushPath(const FTileIndex& Pusher, const FTileIndex& Pushed, int32 MaxDistance) const;

	/**
	 * @brief 고정 방향 밀치기 경로 계산 (함정/기믹처럼 미는 유닛 없이 방향만 있는 경우)
	 * @details
	 * - 함정은 밀리는 유닛과 같은 타일이라 시야(LOS) 검사 없음
	 * - 막히는 규칙은 기존 GetPushPath와 동일 (뒤가 막히면 직전까지, 맵 밖으로는 안 밀림)
	 * @param[in] Pushed 밀리는 쪽 좌표 (전진 시작점)
	 * @param[in] Direction 밀치는 방향 (함정 데이터의 고정 방향)
	 * @param[in] MaxDistance 최대 밀치기 칸 수
	 * @return TArray<FTileIndex> 밀리는 위치부터 도착까지의 경로. 밀리지 않으면 Pushed 위치 한 개
	 */
	TArray<FTileIndex> GetPushPath(const FTileIndex& Pushed, ETileActorDirection Direction, int32 MaxDistance) const;

	/**
	 * @brief 당기기 경로 계산 (대상을 시전자 쪽으로 붙을 때까지 끌어옴)
	 * @details
	 * - 경로는 밀치기와 같은 8방향 직선 (Pulled->Puller 방향을 가까운 직각/대각으로 양자화)
	 * - Puller와 Pulled 사이에 장애물이 시야를 가리면 당기지 못함 (유닛은 관통)
	 * - 정지 조건: 다음 칸이 막힘 / 현재 칸이 Puller와 붙음 / 다음 칸이 Puller를 지나침
	 *   (비스듬한 위치라 직선으로는 면에 못 붙는 경우 대각선 옆에서 멈춤)
	 * - 붙음 판정은 IsPullAdjacent가 결정
	 * @param[in] Puller 당기는 쪽 좌표 (방향 계산 및 정지 기준)
	 * @param[in] Pulled 당겨지는 쪽 좌표 (전진 시작점)
	 * @return TArray<FTileIndex> 당겨지는 위치부터 도착까지의 경로. 당겨지지 않으면 Pulled 위치 한 개
	 */
	TArray<FTileIndex> GetPullPath(const FTileIndex& Puller, const FTileIndex& Pulled) const;

	/**
	 * @brief 당기기 정지 판정 - 대상 칸이 당기는 쪽과 붙었는지
	 * @details 현재 규칙은 4방향 면 접촉 (대각선은 붙은 것으로 보지 않음). 규칙이 바뀌면 이 함수만 수정
	 * @param[in] Tile 검사할 칸 (당겨지는 쪽의 현재 위치)
	 * @param[in] Puller 당기는 쪽 좌표
	 * @return 붙었으면 true
	 */
	static bool IsPullAdjacent(const FTileIndex& Tile, const FTileIndex& Puller);

	/**
	 * 진입 액터를 해당 타일에 배치할 수 있는지 검사하는 함수
	 * @details 막히지 않았거나, 막혔어도 기존 액터를 교체할 수 있으면 배치 가능
	 * @param TileIndex 검사할 타일 인덱스
	 * @param Incoming 진입하려는 액터
	 * @return 배치 가능 여부 (맵 범위 밖은 불가)
	 */
	bool CanPlace(const FTileIndex& TileIndex, const UBoardActorModel* Incoming) const;

	/**
	 * 액터가 바라보는 방향 변경 함수
	 * @details
	 * 이동 없이 방향만 바꾼다.
	 * 논리 방향은 즉시 갱신되고, 뷰에는 OnRotate로 회전 연출을 요청.
	 * @param Direction 바꿀 방향 (타일맵 기준 4방향)
	 * @param Actor 방향을 바꿀 액터
	 * @param Barrier 회전 연출 완료를 기다릴 배리어. (nullptr이면 즉각 완료)
	 * @pre Actor가 타일맵에 배치되어 있어야 한다
	 */
	void RotateActor(ETileActorDirection Direction, UBoardActorModel* Actor, TSharedPtr<FPresentationBarrier> Barrier = nullptr);

	/**
	 * 액터 움직임 시작 함수
	 * 일반 배치 함수와 달리, 움직임 애니메이션 처리 완료 타이밍에 따라서 CompleteActorMovement를 호출해줄 필요가 있음
	 * @param NextTransform 다음 타일 트랜스폼
	 * @param Actor 움직일 액터
	 * @pre NextTransform.mIndex가 맵 범위 안의 유효한 좌표여야 한다
	 * @pre 해당 타일이 Actor 레이어에 대해 막혀있지 않아야 한다 (호출 전 IsBlocked으로 확인)
	 * @warning 사전조건 위반 시 checkf로 중단됨
	 */
	void StartActorMovement(const FTileTransform& NextTransform, UBoardActorModel* Actor);
	void CompleteActorMovement(UBoardActorModel* Actor);

	/**
	 * 액터 초기 배치 함수
	 * @param NextTransform 다음 타일 트랜스폼
	 * @param Actor 배치할 액터
	 * @pre NextTransform.mIndex가 맵 범위 안의 유효한 좌표여야 한다
	 * @pre 해당 타일이 Actor 레이어에 대해 막혀있지 않아야 한다 (호출 전 IsBlocked으로 확인)
	 * @warning 사전조건 위반 시 checkf로 중단됨
	 */
	void PlaceActor(const FTileTransform& NextTransform, UBoardActorModel* Actor);
	void RemoveActor(UBoardActorModel* Actor);

	/**
	 * @brief 배치된 모든 보드 액터의 월드 트랜스폼을 뷰에 재통지
	 * @details 타일 크기 등 뷰의 좌표 기준이 바뀌었을 때 현재 논리 좌표를 다시 변환해
	 *          OnPlaceTileTransform으로 방송. 논리 좌표(점유 타일)는 변경 없음.
	 */
	void RefreshActorPlacements();

	/* 타일 액터 조회 */
	/**
	 * @brief 타일에 있는 액터들을 레이어로 필터해 반환 (코어)
	 * @param[in] TileIndex   : 조회할 타일
	 * @param[in] LayerFilter : 포함할 레이어 (All이면 전체)
	 * @return TArray<UBoardActorModel*> : 해당 타일의 액터 목록 (범위 밖이면 빈 배열)
	 */
	TArray<UBoardActorModel*> GetActorsOnTile(const FTileIndex& TileIndex, ETileLayerFlag LayerFilter = ETileLayerFlag::All) const;

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
		for (UBoardActorModel* Actor : GetActorsOnTile(TileIndex, LayerFilter))
		{
			if (T* Typed = Cast<T>(Actor))
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
		for (UBoardActorModel* Actor : GetActorsOnTile(TileIndex, LayerFilter))
		{
			if (T* Typed = Cast<T>(Actor))
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
	 * @brief 타일 저장소 (크기 Width*Height, 인덱스 = y*Width + x)
	 */
	UPROPERTY()
	TArray<FTile> mTiles;

private:
	/**
	 * @brief 밀치기 전진 루프 공용부 (두 GetPushPath가 공유)
	 * @details Pushed에서 Step 방향으로 막힐 때까지 전진하며 지나간 칸을 기록.
	 *          방향 결정과 시야(LOS) 검사는 각 GetPushPath 책임
	 * @param[in] Pushed 밀리는 쪽 좌표 (전진 시작점)
	 * @param[in] Step 한 칸 전진 스텝 (8방향 양자화 또는 방향 enum 변환 결과)
	 * @param[in] MaxDistance 최대 밀치기 칸 수
	 * @return TArray<FTileIndex> 밀리는 위치부터 도착까지의 경로. 밀리지 않으면 Pushed 위치 한 개
	 */
	TArray<FTileIndex> BuildPushPath(const FTileIndex& Pushed, const FTileIndex& Step, int32 MaxDistance) const;

	/**
	 * 진입 액터가 해당 타일에 막히는지 검사하는 함수
	 * @param TileIndex 검사할 타일 인덱스
	 * @param Incoming 진입하려는 액터
	 * @return 막힘 여부 (맵 범위 밖은 막힘으로 간주)
	 */
	bool IsBlocked(const FTileIndex& TileIndex, const UBoardActorModel* Incoming) const;

	/**
	 * 진입 액터가 덮어쓸(교체할) 기존 액터들을 반환하는 함수
	 * @details 기존 액터가 ReplaceLayerFlags로 진입자 레이어를 교체 허용하고, 진입자의 Overlay 우선순위가 같거나 더 높은 경우만 수집한다.
	 * @param TileIndex 검사할 타일 인덱스
	 * @param Incoming 진입하려는 액터
	 * @return 교체 대상 액터 목록 (빈 배열이면 교체 대상 없음)
	 */
	TArray<UBoardActorModel*> GetReplaceableActors(const FTileIndex& TileIndex, const UBoardActorModel* Incoming) const;

	/**
	 * @brief 코어의 타입 지정 버전 — T로 캐스트되는 교체 대상만 배열로 반환
	 * @details T가 반환 타입에만 등장해 추론 불가 → 호출 시 <T> 명시해야 이 버전이 선택됨
	 * @param[in] TileIndex 검사할 타일 인덱스
	 * @param[in] Incoming  진입하려는 액터
	 * @return TArray<T*> : T로 캐스트된 교체 대상 목록
	 */
	template<typename T>
	TArray<T*> GetReplaceableActors(const FTileIndex& TileIndex, const UBoardActorModel* Incoming) const
	{
		TArray<T*> Result;
		// 코어(비템플릿) 결과를 타입캐스트해 수집
		for (UBoardActorModel* Actor : GetReplaceableActors(TileIndex, Incoming))
		{
			if (T* Typed = Cast<T>(Actor))
			{
				Result.Add(Typed);
			}
		}
		return Result;
	}

	/* 범위 계산 헬퍼 */
	/**
	 * @brief Target 타일이 조준 패턴의 기하 범위 안에 있는 지 판정
	 * @param[in] Origin : 기준 좌표
	 * @param[in] Target : 검사할 좌표
	 * @param[in] Range : 사거리 (1=인접 칸, Single은 무시)
	 * @param[in] Pattern : 조준 패턴
	 * @return bool : 패턴 범위 안이면 true
	 */
	bool IsInAimPattern(const FTileIndex& Origin, const FTileIndex& Target, int32 Range, EAimPattern Pattern) const;

	/**
	 * @brief 시야 차폐 또는 도착 타일 점유로 조준이 막혔는 지 판정
	 * @param[in] Origin : 기준 좌표
	 * @param[in] Target : 조준 대상 좌표
	 * @param[in] Pattern : 조준 패턴 (직선 패턴이면서 직사일 때만 시야 검사)
	 * @param[in] bIncludeOccupied : 점유된 타일(장애물/유닛)을 조준 가능으로 포함할지
	 * @param[in] BlockerLayers : 조준 시야를 막는 레이어 (None이면 아무것도 조준을 막지 않음)
	 * @param[in] Incoming : 교체할 액터. 교체가 없을 경우는 nullptr
	 * @param[in] IgnoreBlocker : 시야 차폐에서 제외할 액터. 없으면 nullptr
	 * @return bool : 막혔으면 true
	 */
	bool IsAimBlocked(
		const FTileIndex& Origin,
		const FTileIndex& Target,
		EAimPattern Pattern,
		bool bIncludeOccupied,
		ETileLayerFlag BlockerLayers,
		const UBoardActorModel* Incoming,
		const UBoardActorModel* IgnoreBlocker
	) const;

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
	 * @brief 원점에서 특정 방향으로 Range만큼 뻗되, 차단 레이어의 액터가 있는 칸에서 멈추는 직선을 수집
	 * @details
	 * AppendRayTiles와 달리, 차단 레이어의 액터가 있는 칸을 만났을 때
	 * 그 칸까지 포함한 뒤 더 진행하지 않는다(맞고 멈춤). None이면 점유와 무관하게 Range 한도까지 진행한다.
	 * 영향범위 Cross/Star 패턴용. 원점 자신은 포함하지 않는다.
	 * @param[in] Origin 시작 좌표
	 * @param[in] Step   한 칸 전진 방향 (예: (1,0)=오른쪽, (-1,1)=좌하단 대각)
	 * @param[in] Range  뻗을 칸 수 (0 이하이면 아무것도 추가하지 않음)
	 * @param[in] BlockerLayers 확산을 막는 레이어 (None이면 아무것도 확산을 막지 않음)
	 * @param[in,out] Out 결과를 누적할 배열
	 * @param[in] IgnoreBlocker 차단 판정에서 제외할 액터 (자리를 비울 예정인 유닛 등). 없으면 nullptr
	 */
	void AppendBlockableRay(const FTileIndex& Origin, const FTileIndex& Step, int32 Range, ETileLayerFlag BlockerLayers, TArray<FTileIndex>& Out, const UBoardActorModel* IgnoreBlocker = nullptr) const;

	/**
	 * @brief 타일에 장애물 또는 유닛이 있는 지 검사
	 * @details
	 * 이동범위(통과/도착 차단)와 영향범위(직선 차단) 계산에 쓰인다.
	 * CanPlace()와 달리 비교하는 액터가 없으므로 교체가 없으며 단순 물리적인 방해만 판정한다.
	 * @param[in] TileIndex 검사할 좌표
	 * @param[in] IgnoreBlocker 점유 판정에서 제외할 액터 (자리를 비울 예정인 유닛 등). 없으면 nullptr
	 * @return 장애물/유닛이 있으면 true, 없거나 맵 밖이면 false
	 */
	bool IsOccupied(const FTileIndex& TileIndex, const UBoardActorModel* IgnoreBlocker = nullptr) const;

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
	 * 시야를 막는 액터(BlockerLayers 레이어)가 하나라도 있으면 막힌 것으로 본다.
	 * @param[in] From 시작 좌표
	 * @param[in] To   목표 좌표
	 * @param[in] IgnoreBlocker 차폐 판정에서 제외할 액터 (자리를 비울 예정인 유닛 등). 없으면 nullptr
	 * @param[in] BlockerLayers 차폐물로 볼 레이어 (기본: 장애물+유닛. 밀치기는 장애물만 차폐)
	 * @return 시야가 확보되면 true, 중간이 막히면 false
	 */
	bool HasLineOfSight(const FTileIndex& From, const FTileIndex& To, const UBoardActorModel* IgnoreBlocker = nullptr,
		ETileLayerFlag BlockerLayers = ETileLayerFlag::Obstacle | ETileLayerFlag::Unit) const;

	/**
	 * @brief 타일에 액터 등록 (mBoardActors에 추가)
	 */
	void RegisterActorToTile(FTile* Tile, UBoardActorModel* Actor);

	/**
	 * @brief 타일에서 액터 해제 (mBoardActors에서 제거)
	 */
	void UnregisterActorFromTile(FTile* Tile, UBoardActorModel* Actor);

	/**
	 * @brief 같은 타일의 다른 액터들과 양방향 OnBeginTileOverlap 통지 (자기 제외)
	 */
	void NotifyBeginOverlap(FTile* Tile, UBoardActorModel* Actor);

	/**
	 * @brief 같은 타일의 다른 액터들과 양방향 OnEndTileOverlap 통지 (자기 제외)
	 */
	void NotifyEndOverlap(FTile* Tile, UBoardActorModel* Actor);
};
