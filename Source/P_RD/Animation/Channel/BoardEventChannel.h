/*****************************************************************//**
 * @file   BoardEventChannel.h
 * @brief  보드 이벤트 시퀀스 세션 내 키 프레임 및 채널 정의 헤더
 * @author 모호재
 * @date   2026-08-17
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Channels/MovieSceneChannel.h"
#include "Channels/MovieSceneChannelData.h"
#include "Channels/MovieSceneChannelTraits.h"
#include "Animation/Notify/EventTriggerPayload.h"
#include "BoardEventChannel.generated.h"

/**
 * @brief 이벤트 구조체
 */
USTRUCT(BlueprintType)
struct FBoardSceneEvent
{
	GENERATED_BODY()

public:
	FBoardSceneEvent() = default;
	FBoardSceneEvent(FGameplayTag EventTag);

public:
	friend bool operator==(const FBoardSceneEvent& Lhs, const FBoardSceneEvent& Rhs)
	{
		return Lhs.mEventTag == Rhs.mEventTag && Lhs.mEventPayload == Rhs.mEventPayload;
	}

	friend bool operator!=(const FBoardSceneEvent& Lhs, const FBoardSceneEvent& Rhs)
	{
		return Lhs.mEventTag != Rhs.mEventTag || Lhs.mEventPayload != Rhs.mEventPayload;
	}

public:
	UPROPERTY(Category = "Sequencer|Event", EditAnywhere, meta = (DisplayName = "EventTag"))
	FGameplayTag mEventTag;

	UPROPERTY(Category = "Sequencer|Event", EditAnywhere, meta = (DisplayName = "EventPayload"))
	TInstancedStruct<FEventTriggerPayloadBase> mEventPayload;
};

/**
 * @brief 이벤트 구조체
 */
USTRUCT(BlueprintType)
struct FBoardEventTriggerData
{
	GENERATED_BODY()

public:
	FBoardEventTriggerData() = default;
	FBoardEventTriggerData(FGameplayTag EventTag);

public:
	friend bool operator==(const FBoardEventTriggerData& Lhs, const FBoardEventTriggerData& Rhs)
	{
		return Lhs.mEventTag == Rhs.mEventTag && Lhs.mEventPayload == Rhs.mEventPayload;
	}

	friend bool operator!=(const FBoardEventTriggerData& Lhs, const FBoardEventTriggerData& Rhs)
	{
		return Lhs.mEventTag != Rhs.mEventTag || Lhs.mEventPayload != Rhs.mEventPayload;
	}

public:
	UPROPERTY(Category = "Sequencer|Event", EditAnywhere, meta = (DisplayName = "EventTag"))
	FGameplayTag mEventTag;

	UPROPERTY(Category = "Sequencer|Event", EditAnywhere, meta = (DisplayName = "EventPayload", ExcludeBaseStruct))
	TInstancedStruct<FEventTriggerPayload> mEventPayload;
};

/**
 * @brief 이벤트 구조체
 */
USTRUCT(BlueprintType)
struct FBoardEventDurationData
{
	GENERATED_BODY()

public:
	FBoardEventDurationData() = default;
	FBoardEventDurationData(FGameplayTag EventTag);

public:
	friend bool operator==(const FBoardEventDurationData& Lhs, const FBoardEventDurationData& Rhs)
	{
		return Lhs.mEventTag == Rhs.mEventTag && Lhs.mEventPayload == Rhs.mEventPayload;
	}

	friend bool operator!=(const FBoardEventDurationData& Lhs, const FBoardEventDurationData& Rhs)
	{
		return Lhs.mEventTag != Rhs.mEventTag || Lhs.mEventPayload != Rhs.mEventPayload;
	}

public:
	UPROPERTY(Category = "Sequencer|Event", EditAnywhere, meta = (DisplayName = "EventTag"))
	FGameplayTag mEventTag;

	UPROPERTY(Category = "Sequencer|Event", EditAnywhere, meta = (DisplayName = "EventPayload", ExcludeBaseStruct))
	TInstancedStruct<FDurationEventTriggerPayload> mEventPayload;
};

/**
 * @brief 단발성 보드 이벤트 채널
 */
USTRUCT()
struct FBoardEventTriggerChannel : public FMovieSceneChannel
{
	GENERATED_BODY()

	typedef FBoardSceneEvent CurveValueType;

public:
	FBoardEventTriggerChannel() = default;

	/* FMovieSceneChannel 상속 */
public:
	void GetKeys(const TRange<FFrameNumber>& WithinRange, TArray<FFrameNumber>* OutKeyTimes, TArray<FKeyHandle>* OutKeyHandles) override;
	void GetKeyTimes(TArrayView<const FKeyHandle> InHandles, TArrayView<FFrameNumber> OutKeyTimes) override;
	void SetKeyTimes(TArrayView<const FKeyHandle> InHandles, TArrayView<const FFrameNumber> InKeyTimes) override;
	void DuplicateKeys(TArrayView<const FKeyHandle> InHandles, TArrayView<FKeyHandle> OutNewHandles) override;
	void DeleteKeys(TArrayView<const FKeyHandle> InHandles) override;
	void DeleteKeysFrom(FFrameNumber InTime, bool bDeleteForward) override;
	void RemapTimes(const UE::MovieScene::IRetimingInterface& Retimer) override;
	TRange<FFrameNumber> ComputeEffectiveRange() const override;
	int32 GetNumKeys() const override;
	void Reset() override;
	void Offset(FFrameNumber DeltaPosition) override;
	FKeyHandle GetHandle(int32 Index) override;
	int32 GetIndex(FKeyHandle Handle) override;

	/* 채널 데이터 조작 API */
public:
	P_RD_API TMovieSceneChannelData<FBoardEventTriggerData> GetData();
	P_RD_API TMovieSceneChannelData<const FBoardEventTriggerData> GetData() const;

	TArrayView<const FFrameNumber> GetKeyTimes() const
	{
		return mKeyTimes;
	}

	TArrayView<const FBoardEventTriggerData> GetKeyValues() const
	{
		return mKeyValues;
	}

public:
	UPROPERTY(meta = (KeyTimes))
	TArray<FFrameNumber> mKeyTimes;

	UPROPERTY(meta = (KeyValues))
	TArray<FBoardEventTriggerData> mKeyValues;

private:
	UPROPERTY(Transient)
	FMovieSceneKeyHandleMap mKeyHandles;
};

template <>
struct TMovieSceneChannelTraits<FBoardEventTriggerChannel> : TMovieSceneChannelTraitsBase<FBoardEventTriggerChannel>
{
	enum { SupportsDefaults = false };
};

inline bool EvaluateChannel(const FBoardEventTriggerChannel* InChannel, FFrameTime InTime, FBoardSceneEvent& OutValue)
{
	return false;
}

