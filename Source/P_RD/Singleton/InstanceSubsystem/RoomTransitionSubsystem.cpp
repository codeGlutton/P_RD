#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"

#include "Engine/AssetManager.h"
#include "DataAsset/StaticRoomSpawnData.h"

DEFINE_LOG_CATEGORY(LogTransition)

void URoomTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URoomTransitionSubsystem::PreloadRoomAsync(const FPrimaryAssetId& RoomId, bool IsAutoTransition)
{
    PreloadRoomAsync(RoomId, TArray<FPrimaryAssetId>(), IsAutoTransition);
}

void URoomTransitionSubsystem::PreloadRoomAsync(const FPrimaryAssetId& RoomId, TArray<FPrimaryAssetId>&& AdditionalAssetIds, bool IsAutoTransition)
{
    if (mTransitionState != ESRPGRoomTransitionState::None)
    {
        UE_LOG(LogTransition, Log, TEXT("다른 룸 데이터 처리 중으로 Preload 불가"));
        return;
    }
    mTransitionState = ESRPGRoomTransitionState::Loading;
    mIsAutoTransition = IsAutoTransition;
    mNextRoomId = RoomId;

    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

    TArray<FName> Bundles = { "World", "Actor" };
    FAssetManagerLoadParams LoadParams;
    LoadParams.OnComplete.BindUObject(this, &URoomTransitionSubsystem::OnLoadNextRoom);

    AdditionalAssetIds.Add(RoomId);
    mPreloadHandle = AssetManager->PreloadPrimaryAssets(AdditionalAssetIds, Bundles, true, MoveTemp(LoadParams));
}

void URoomTransitionSubsystem::TransitLoadedRoomAsync()
{
    if (mTransitionState == ESRPGRoomTransitionState::None)
    {
        UE_LOG(LogTransition, Log, TEXT("전환 불가. 방 데이터 로드 필요"));
        return;
    }

    // 에셋 로드가 아직 완료되지 않은 경우
    if (mTransitionState == ESRPGRoomTransitionState::Loading)
    {
        // 자동 맵 전환 예약
        mIsAutoTransition = true;
        return;
    }

    // 에셋 로드가 완료된 경우
    OnTransitNextRoom();
}

void URoomTransitionSubsystem::OnLoadNextRoom(TSharedPtr<FStreamableHandle> AssetHandle)
{
    mTransitionState = ESRPGRoomTransitionState::Loaded;

    // 자동 맵 전환 비활성화 시
    if (mIsAutoTransition == false)
    {
        return;
    }

    OnTransitNextRoom();
}

void URoomTransitionSubsystem::OnTransitNextRoom()
{
    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

    UStaticRoomSpawnData* StaticRoomData = AssetManager->GetPrimaryAssetObject<UStaticRoomSpawnData>(mNextRoomId);
    checkf(StaticRoomData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));

    mTransitionState = ESRPGRoomTransitionState::None;
    mIsAutoTransition = false;

    UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), StaticRoomData->mBackgroundMap);
}
