/*****************************************************************//**
 * @file   RoomTransitionSubsystem.h
 * @brief  방 전환 처리를 돕는 Subsystem 구현 헤더
 * @author 모호재
 * @date   2026-04-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentDataWriter.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"

#include "RoomTransitionSubsystem.generated.h"

// Transition 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogTransition, Log, All)

DECLARE_DELEGATE_TwoParams(FOnReadyToTransition, int32 /*RoomRowIndex*/, int32 /*RoomColumnIndex*/)
DECLARE_DELEGATE_TwoParams(FOnPreTransitNextRoom, int32 /*RoomRowIndex*/, int32 /*RoomColumnIndex*/)

struct FRoom;

/**
 * @brief  방 전환 상태 플래그
 */
enum class ERoomTransitionStateFlag : uint8
{
	None = 0,

	NewStageRequested = 1 << 0,
	PreLoadRequested = 1 << 1,

	ExternalReady = 1 << 2,

	PlayerLoaded = 1 << 3,
	StageLoaded = 1 << 4,
	RoomLoaded = 1 << 5,

	AutoTransition = 1 << 6,

	AllTaskRequested = NewStageRequested | PreLoadRequested,
	ReadyToTransition = PreLoadRequested | PlayerLoaded | StageLoaded | RoomLoaded | ExternalReady,
	InTransition = ReadyToTransition | AutoTransition,
};
ENUM_CLASS_FLAGS(ERoomTransitionStateFlag);

USTRUCT()
struct FRoomTransitionRequest
{
	GENERATED_BODY()

public:
	bool mChangePersistentData = true;
	int32 mRoomRowIndex = -1;
	int32 mRoomColumnIndex = -1;

	FOnReadyToTransition OnReadyToTransition;
	FOnPreTransitNextRoom OnPreTransitNextRoom;
};

/**
 * @brief  SRPG 방 전환 처리를 돕는 Subsystem
 */
UCLASS()
class P_RD_API URoomTransitionSubsystem : public UGameInstanceSubsystem, public IRunDataWriter
{
	GENERATED_BODY()

	/* UGameInstanceSubsystem 상속 */
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	/**
	 * 타이틀 방에 등장할 에셋들을 Preload 해두는 함수
	 * @param ReadyToTransitionCallback	Transition 준비 완료 시 호출될 대리자
	 * @param PreTransitionCallback		Transition 전에 호출될 대리자
	 * @param RequireExternalReady		외부의 준비 상태를 대기할 필요가 있는지 여부
	 * @param IsAutoTransition			모든 준비 완료 시 자동 Transition 여부
	 */
	bool PreloadFrontendRoomAsync(FOnReadyToTransition ReadyToTransitionCallback = FOnReadyToTransition(), FOnPreTransitNextRoom PreTransitionCallback = FOnPreTransitNextRoom(), bool RequireExternalReady = true, bool IsAutoTransition = false);

	/**
	 * 다음 방에 등장할 에셋들을 Preload 해두는 함수
	 * @param RoomRowIndex				다음 방 행 인덱스
	 * @param RoomColumnIndex			다음 방 열 인덱스
	 * @param ReadyToTransitionCallback	Transition 준비 완료 시 호출될 대리자
	 * @param PreTransitionCallback		Transition 전에 호출될 대리자
	 * @param RequireExternalReady		외부의 준비 상태를 대기할 필요가 있는지 여부
	 * @param IsAutoTransition			모든 준비 완료 시 자동 Transition 여부
	 */
	bool PreloadRoomAsync(int32 RoomRowIndex, int32 RoomColumnIndex, FOnReadyToTransition ReadyToTransitionCallback = FOnReadyToTransition(), FOnPreTransitNextRoom PreTransitionCallback = FOnPreTransitNextRoom(), bool RequireExternalReady = true, bool IsAutoTransition = false);
	bool PreloadRoomAsync(FRoomTransitionRequest Request, bool RequireExternalReady = true, bool IsAutoTransition = false);
	
	/**
	 * 비동기로 새로운 Stage를 만든 다음, 자동으로 Stage의 첫 방에 대하여 Preload 해두는 함수
	 * @param StageLevel				대상 스테이지 레벨
	 * @param ReadyToTransitionCallback	Transition 준비 완료 시 호출될 대리자
	 * @param PreTransitionCallback		Transition 전에 호출될 대리자
	 * @param RequireExternalReady		외부의 준비 상태를 대기할 필요가 있는지 여부
	 * @param IsAutoTransition			모든 준비 완료 시 자동 Transition 여부
	 */
	bool MakeStageAndPreloadRoomAsync(EStageLevelType StageLevel, FOnReadyToTransition ReadyToTransitionCallback = FOnReadyToTransition(), FOnPreTransitNextRoom PreTransitionCallback = FOnPreTransitNextRoom(), bool RequireExternalReady = true, bool IsAutoTransition = false);
	
	/**
	 * 외부에서도 준비가 완료됨을 알림
	 */
	bool MarkExternalReady();

	/**
	 * @brief AutoTransition=false로 대기 중인 전환을 실제로 시작한다.
	 *
	 * @details
	 * ReadyToTransition 상태가 된 뒤 GameMode가 로딩 UI를 닫고 호출하는 수동 전환 진입점이다.
	 * 준비가 끝나지 않았다면 false를 반환하고 레벨 이동을 시작하지 않는다.
	 */
	bool TransitLoadedRoom();

protected:
	void OnLoadNextPlayer(TSharedPtr<FStreamableHandle> AssetHandle);
	void OnLoadNextStage(TSharedPtr<FStreamableHandle> AssetHandle);
	void OnLoadNextRoom(TSharedPtr<FStreamableHandle> AssetHandle);

protected:
	void OnReadyToTransition();
	void OnTransitNextRoom();

	/* 유지 데이터 */
private:
	ERoomTransitionStateFlag mTransitionState = ERoomTransitionStateFlag::None;
	TSharedPtr<FStreamableHandle> mPlayerPreloadHandle = nullptr;
	TSharedPtr<FStreamableHandle> mStagePreloadHandle = nullptr;
	TSharedPtr<FStreamableHandle> mRoomPreloadHandle = nullptr;

	/* 일시 데이터 */
private:
	FRoomTransitionRequest mRequest;
};

