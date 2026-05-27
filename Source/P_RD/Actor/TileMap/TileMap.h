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

/**
 * @brief  타일맵 액터
 */
UCLASS()
class P_RD_API ATileMap : public AActor
{
	GENERATED_BODY()
	
public:	
	ATileMap();

public:
	/**
	 * 타일의 해당 지점의 FTransform 정보를 가져오는 함수
	 * @param Transform 타일 좌표계에서의 좌표 값
	 * @return 언리얼의 좌표계에서의 좌표 값
	 */
	const FTransform& GetTileTransform(const FTileTransform& Transform) const;
	const FTransform& GetTileTransform(ETileRotation Direction, int32 Row, int32 Column) const;
	const FVector& GetTilePosition(int32 Row, int32 Column) const;

public:
	/**
	 * 배치 가능한지 체크하는 함수
	 * @param Transform 배치할 지점 좌표
	 * @param LayerFlag 배치 액터 Layer
	 * @return 가능 여부
	 */
	bool IsBlocking(const FTileTransform& Transform, ETileLayerFlag LayerFlag) const;

	/**
	 * 액터 움직임 시작 함수
	 * 일반 배치 함수와 달리, 움직임 애니메이션 처리 완료 타이밍에 따라서 FinishActorMovement를 호출해줄 필요가 있음
	 * @param NextTransform 다음 타일 트랜스폼
	 * @param Actor 움직일 액터
	 */
	void BeginActorMovement(const FTileTransform& NextTransform, ITileActor* Actor);
	void FinishActorMovement(ITileActor* Actor);

	/**
	 * 액터 초기 배치 함수
	 * @param NextTransform 다음 타일 트랜스폼
	 * @param Actor 배치할 액터
	 */
	void PlaceActor(const FTileTransform& NextTransform, ITileActor* Actor);
	void RemoveActor(ITileActor* Actor);
};
