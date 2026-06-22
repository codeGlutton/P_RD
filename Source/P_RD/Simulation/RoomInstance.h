/*****************************************************************//**
 * @file   RoomInstance.h
 * @brief  방 내 루트 객체가 되는 객체 헤더
 * @author 모호재
 * @date   2026-06-18
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "RoomInstance.generated.h"

class UObjectModel;

/**
 * @brief  복제본을 위해 만들어지는 일종의 수집 데이터
 */
USTRUCT()
struct FRoomCopyData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FRandomStream mCopiedEventStream;
};

/**
 * @brief  방 내 루트 객체가 되는 객체 헤더
 */
UCLASS()
class P_RD_API URoomInstance : public UObject
{
	GENERATED_BODY()

public:
	void CollectGameDatas();
	void CollectSimulationDatas();

	/* 실제 및 시뮬 데이터 */
public:
	// @brief 살아있는 월드 시스템 모델 맵핑 테이블
	UPROPERTY()
	TMap<TSubclassOf<UObjectModel>, TObjectPtr<UObjectModel>> mAliveSubsystemModels;

	// @brief 살아있는 월드 모델들 강참조
	UPROPERTY()
	TArray<TObjectPtr<UObjectModel>> mAliveWorldModels;

	// @brief 최대 ID 값
	UPROPERTY()
	int32 mBoardActorMaxId;

public:
	// @brief 이벤트 스트림 값 참조
	const FRandomStream* mEventStreamPtr;

	/* 복제본 데이터 */
private:
	// @brief 복제본을 위한 데이터
	TInstancedStruct<FRoomCopyData> mCopiedData;
};

