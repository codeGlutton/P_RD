/*****************************************************************//**
 * @file   RoomTransitionSubsystem.h
 * @brief  방 전환 처리를 돕는 Subsystem 구현 헤더
 * @author 모호재
 * @date   2026-04-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"

#include "RoomTransitionSubsystem.generated.h"

// Transition 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogTransition, Log, All)

DECLARE_DELEGATE_TwoParams(FOnPreTransitNextRoom, const FPrimaryAssetId& /*StageId*/, const FPrimaryAssetId& /*RoomId*/)
DECLARE_EVENT(URoomTransitionSubsystem, FOnPreTransitNextRoom_Internal)

struct FRoom;

/**
 * @brief  방 전환 상태 플래그
 */
enum class ERoomTransitionStateFlag : uint8
{
	None = 0,

	NewStageRequested = 1 << 0,
	PreLoadRequested = 1 << 1,

	AutoTransition = 1 << 2,

	StageLoaded = 1 << 3,
	RoomLoaded = 1 << 4,

	AllTaskRequested = NewStageRequested | PreLoadRequested,
	ReadyToTransition = PreLoadRequested | StageLoaded | RoomLoaded,
};
ENUM_CLASS_FLAGS(ERoomTransitionStateFlag);

/**
 * @brief  SRPG 방 전환 처리를 돕는 Subsystem
 */
UCLASS()
class P_RD_API URoomTransitionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/* UGameInstanceSubsystem 상속 */
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	/**
	 * 다음 방에 등장할 에셋들을 Preload 해두는 함수
	 * @param StageId			스테이지에 대한 정적 에셋 및 데이터
	 * @param RoomId			방에 대한 정적 에셋 및 데이터
	 * @param Callback			Transition 전에 호출될 대리자
	 * @param IsAutoTransition	Preload 완료 시 자동 Transition 여부
	 */
	void PreloadRoomAsync(const FPrimaryAssetId& StageId, const FPrimaryAssetId& RoomId, FOnPreTransitNextRoom Callback = FOnPreTransitNextRoom(), bool IsAutoTransition = false);
	void PreloadRoomAsync(const FPrimaryAssetId& StageId, const FPrimaryAssetId& RoomId, TArray<FPrimaryAssetId>&& AdditionalAssetIds, FOnPreTransitNextRoom Callback = FOnPreTransitNextRoom(), bool IsAutoTransition = false);
	
	/**
	 * 비동기로 새로운 Stage를 만든 다음, 자동으로 Stage의 첫 방에 대하여 Preload 해두는 함수
	 * @param StageLevel		대상 스테이지 레벨
	 * @param Callback			Transition 전에 호출될 대리자
	 * @param IsAutoTransition	Preload 완료 시 자동 Transition 여부
	 */
	void MakeStageAndPreloadRoomAsync(EStageLevelType StageLevel, FOnPreTransitNextRoom Callback = FOnPreTransitNextRoom(), bool IsAutoTransition = false);
	
	/**
	 * Preload된 데이터들이 있을 경우, Transition 시작. 아직 Preload 중이라면 비동기 대기 후 전환
	 */
	void TransitLoadedRoomAsync();

protected:
	void OnLoadNextStage(TSharedPtr<FStreamableHandle> AssetHandle);
	void OnLoadNextRoom(TSharedPtr<FStreamableHandle> AssetHandle);

protected:
	void OnTransitNextRoom();

	/* 유지 데이터 */
private:
	ERoomTransitionStateFlag mTransitionState = ERoomTransitionStateFlag::None;
	TSharedPtr<FStreamableHandle> mStagePreloadHandle = nullptr;
	TSharedPtr<FStreamableHandle> mRoomPreloadHandle = nullptr;

	/* 일시 데이터 */
private:
	FPrimaryAssetId mNextRoomId;
	FOnPreTransitNextRoom_Internal OnPreTransitNextRoom_Internal;
};

