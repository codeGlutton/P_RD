/*****************************************************************//**
 * @file   RoomEditor.h
 * @brief  에디터 상에서 전투 방 배치 데이터(Combat Room DA)를 시각화 및 테스트하는 액터 클래스 헤더
 * @author 모호재
 * @date   2026-09-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomEditor.generated.h"

class UChildActorComponent;
class UStaticCombatRoomSpawnData;
class ATileMap;
class APlayerUnit;

/**
 * @brief 에디터 상에서 전투 방 배치 데이터(Combat Room DA)를 기반으로 타일맵 위 요소 배치를 테스트하는 액터
 */
UCLASS()
class P_RD_API ARoomEditor : public AActor
{
	GENERATED_BODY()

public:
	ARoomEditor();

public:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/* 방 배치 갱신 */
public:
	/**
	 * @brief 디테일 창 버튼 또는 에디터 프로퍼티 변경 시 전투 방 데이터(mRoomSpawnData) 기반으로 스폰 액터를 리프레쉬
	 */
	UFUNCTION(CallInEditor, Category = "RoomEditor", meta = (DisplayName = "Refresh Room"))
	void RefreshRoom();

	/* 스폰 액터 정돈 */
public:
	/**
	 * @brief 이전에 생성한 배치 액터들을 제거
	 */
	void CleanUpSpawnedActors();

protected:
	/**
	 * @brief 타일맵 자식 액터 컴포넌트
	 */
	UPROPERTY(Category = "RoomEditor", VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TileMapComponent"))
	TObjectPtr<UChildActorComponent> mTileMapComponent;

	/**
	 * @brief 전투 방 배치 데이터 (Room DA)
	 */
	UPROPERTY(Category = "RoomEditor", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RoomSpawnData"))
	TObjectPtr<UStaticCombatRoomSpawnData> mRoomSpawnData;

	/**
	 * @brief 전투 방 테스트용 배치 플레이어 캐릭터
	 */
	UPROPERTY(Category = "RoomEditor", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TestPlayerUnit"))
	TSubclassOf<APlayerUnit> mTestPlayerUnit;

	/**
	 * @brief 스폰되어 추적 중인 배치 액터 목록
	 */
	UPROPERTY(Category = "RoomEditor", VisibleAnywhere, Transient, meta = (DisplayName = "SpawnedActors"))
	TArray<TObjectPtr<AActor>> mSpawnedActors;
};
