/*****************************************************************//**
 * @file   BoardEventTrackInstance.h
 * @brief  트랙 인스턴스를 활용하여 단발성 및 구간성 보드 이벤트를 수집 및 실행하는 클래스 정의 헤더
 * @author 모호재
 * @date   2026-08-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "EntitySystem/TrackInstance/MovieSceneTrackInstance.h"
#include "Animation/Section/BoardEventTriggerSection.h"
#include "Animation/Section/BoardEventDurationSection.h"
#include "BoardEventTrackInstance.generated.h"

// Board Event Track 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogBoardEventTrack, Log, All)

class UBoardActorSequencePlayer;

/**
 * @brief 이벤트 수집 데이터 구조체
 */
USTRUCT()
struct FBoardSceneEventInstanceData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FFrameNumber mKeyTime;

	UPROPERTY()
	FBoardSceneEvent mEvent;
};

/**
 * @brief 이벤트 호출을 위한 컨텍스트
 */
USTRUCT()
struct FBoardSceneEventInstanceContext
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FBoardSceneEventInstanceData> mDatas;

public:
	UE::MovieScene::FInstanceHandle mInstanceHandle;
};

/**
 * @brief BoardEventTrack의 런타임 트랙 인스턴스 클래스
 */
UCLASS(BlueprintType)
class P_RD_API UBoardEventTrackInstance : public UMovieSceneTrackInstance
{
	GENERATED_BODY()

public:
	UBoardEventTrackInstance();

	/* UMovieSceneTrackInstance 상속 */
public:
	virtual void OnInputAdded(const FMovieSceneTrackInstanceInput& InInput) override;
	virtual void OnInputRemoved(const FMovieSceneTrackInstanceInput& InInput) override;
	virtual void OnAnimate() override;

private:
	TRange<FFrameNumber> GetCurrentFrameNumberRange(UE::MovieScene::FInstanceHandle Handle) const;
	UBoardActorSequencePlayer* GetBoardActorSequencePlayer(UE::MovieScene::FInstanceHandle Handle) const;
	void TriggerBoardEvent(const FBoardSceneEvent& Event, UE::MovieScene::FInstanceHandle Handle);
	void TriggerBoardEventEnd(const FBoardSceneEvent& Event, UE::MovieScene::FInstanceHandle Handle);

private:
	/** 수집된 즉발성(Trigger) 이벤트 복제 데이터 맵 */
	UPROPERTY()
	TMap<TObjectPtr<const UMovieSceneSection>, FBoardSceneEventInstanceContext> mActiveTriggerEvents;

	/** 수집된 구간성(Duration) 이벤트 복제 데이터 맵 */
	UPROPERTY()
	TMap<TObjectPtr<const UMovieSceneSection>, FBoardSceneEventInstanceContext> mActiveDurationEvents;
};
