/*****************************************************************//**
 * @file   SRPGRoomTransitionSubsystem.h
 * @brief  SRPG 방 전환 처리를 돕는 Subsystem 구현 헤더
 * @author 모호재
 * @date   2026-04-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "SRPGFramework/SRPGFrameworkType.h"

#include "SRPGRoomTransitionSubsystem.generated.h"

// RD 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogSRPGTransition, Log, All)

/**
 * @brief  SRPG 방 전환 상태 열거형
 */
enum class ESRPGRoomTransitionState : uint8
{
	None,
	Loading,
	Loaded,
};

/**
 * @brief  SRPG 방 전환 처리를 돕는 Subsystem
 */
UCLASS()
class P_RD_API USRPGRoomTransitionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/* UGameInstanceSubsystem 상속 */
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	void PreloadRoomAsync(const FPrimaryAssetId& RoomId, bool IsAutoTransition = false);
	void PreloadRoomAsync(const FPrimaryAssetId& RoomId, TArray<FPrimaryAssetId>&& AdditionalAssetIds, bool IsAutoTransition = false);
	void TransitLoadedRoomAsync();

protected:
	void OnLoadNextRoom(TSharedPtr<FStreamableHandle> AssetHandle);

protected:
	void OnTransitNextRoom();

	/* 유지 데이터 */
private:
	ESRPGRoomTransitionState mTransitionState = ESRPGRoomTransitionState::None;
	TSharedPtr<FStreamableHandle> mPreloadHandle;

	/* 일시 데이터 */
private:
	bool mIsAutoTransition = false;
	FPrimaryAssetId mNextRoomId;
};

