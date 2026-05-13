/*****************************************************************//**
 * @file   RoomTransitionSubsystem.h
 * @brief  방 전환 처리를 돕는 Subsystem 구현 헤더
 * @author 모호재
 * @date   2026-04-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "RoomTransitionSubsystem.generated.h"

// Transition 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogTransition, Log, All)

struct FRoom;

/**
 * @brief  방 전환 상태 플래그
 */
enum class ERoomTransitionStateFlag : uint8
{
	None = 0,

	PreLoadRequested = 1 << 0,
	AutoTransition = 1 << 1,
	StageLoaded = 1 << 2,
	RoomLoaded = 1 << 3,

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
	void PreloadRoomAsync(const FPrimaryAssetId& StageId, const FPrimaryAssetId& RoomId, bool IsAutoTransition = false);
	void PreloadRoomAsync(const FPrimaryAssetId& StageId, const FPrimaryAssetId& RoomId, TArray<FPrimaryAssetId>&& AdditionalAssetIds, bool IsAutoTransition = false);
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
};

