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
	TArray<TWeakObjectPtr<UBoardActorModel>> mBoardActors;
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
DECLARE_DELEGATE_OneParam(FSetMovePathDelegate, const TArray<FTileIndex>&);

/**
 * @brief 모델이 뷰(ATileMap)에 타일 강조(하이라이트) 표시/해제를 요청하는 델리깃
 * @details 경로 델리깃과 동일하게 non-dynamic — 라이브에서 뷰가 SetTileHighlight/ClearTileHighlight로
 *          바인딩하고, 미바인딩(심 복제본)이면 표시되지 않는다.
 */
DECLARE_DELEGATE_TwoParams(FSetTileHighlightDelegate, const TArray<FTileIndex>&, ETileHighlightFlag);
DECLARE_DELEGATE_OneParam(FClearTileHighlightDelegate, ETileHighlightFlag);

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
	 * @brief 시작→목표 최단 이동경로 계산 (장애물/유닛 회피)
	 * @details
	 * GetReachableTiles와 동일한 4방향(직교) BFS 규칙을 쓴다 — 도달 가능성과 경로 존재가 일치하도록.
	 * 장애물·유닛(IsOccupied) 칸은 통과·도착 불가. 여러 최단경로 중 이웃 탐색 순서(직교 4방향)로 하나를 고른다.
	 * 시작 칸은 점유돼 있어도(자기 유닛이 선 칸) 출발점으로 허용한다.
	 *
	 * @param[in] Start : 시작 좌표
	 * @param[in] Goal  : 목표 좌표
	 * @return TArray<FTileIndex> : Start부터 Goal까지 순서대로의 타일 목록(양 끝 포함). 경로가 없으면 빈 배열
	 */
	TArray<FTileIndex> FindPath(const FTileIndex& Start, const FTileIndex& Goal) const;

	/**
	 * @brief 시작→목표 경로를 계산해 뷰에 표시 요청 (FindPath + 뷰 표시 델리깃 호출)
	 * @details 미바인딩(심 복제본)이면 아무 일도 하지 않는다. 경로가 없으면 빈 경로라 표시 해제와 같다.
	 * @param[in] Start : 시작 좌표
	 * @param[in] Goal  : 목표 좌표
	 */
	void SetMovePath(const FTileIndex& Start, const FTileIndex& Goal);

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
		const UBoardActorModel* Incoming = nullptr
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
	 * @brief 밀치기 시 밀려나는 액터의 최종 도착 좌표 계산 (순수 계산, 뷰 비의존)
	 * @details
	 * Pusher→Pushed 방향을 각 축 부호로 8방향 단위 스텝화해(대각 포함) Pushed에서부터 한 칸씩 전진시킨다.
	 * 다음 칸이 맵 밖이거나 장애물/유닛(IsOccupied)이면 더 밀리지 않고 직전 칸에서 멈춘다.
	 * 코너 컷(대각 틈새 통과)은 시야 판정과 동일하게 허용한다 — 목적지 칸만 검사한다.
	 * 첫 칸부터 막히면(또는 Pusher==Pushed로 방향이 없으면) Pushed 그대로 반환한다.
	 * @param[in] Pusher : 미는 쪽 좌표 (방향 계산용)
	 * @param[in] Pushed : 밀리는 쪽 좌표 (전진 시작점)
	 * @param[in] MaxDistance : 최대 밀치기 칸 수 (0 이하이면 Pushed 그대로)
	 * @return FTileIndex : 밀려난 최종 좌표 (못 밀리면 Pushed)
	 */
	FTileIndex GetPushDestination(const FTileIndex& Pusher, const FTileIndex& Pushed, int32 MaxDistance) const;

	/**
	 * 진입 액터를 해당 타일에 배치할 수 있는지 검사하는 함수
	 * @details 막히지 않았거나, 막혔어도 기존 액터를 교체할 수 있으면 배치 가능
	 * @param TileIndex 검사할 타일 인덱스
	 * @param Incoming 진입하려는 액터
	 * @return 배치 가능 여부 (맵 범위 밖은 불가)
	 */
	bool CanPlace(const FTileIndex& TileIndex, const UBoardActorModel* Incoming) const;

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
