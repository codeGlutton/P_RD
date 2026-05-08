#include "Singleton/InstanceSubsystem/SRPGRoomTransitionSubsystem.h"

#include "Engine/AssetManager.h"
#include "DataAsset/StaticRoomSpawnData.h"

DEFINE_LOG_CATEGORY(LogSRPGTransition)

void USRPGRoomTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void USRPGRoomTransitionSubsystem::PreloadRoomAsync(const FPrimaryAssetId& RoomId, bool IsAutoTransition)
{
    PreloadRoomAsync(RoomId, TArray<FPrimaryAssetId>(), IsAutoTransition);
}

void USRPGRoomTransitionSubsystem::PreloadRoomAsync(const FPrimaryAssetId& RoomId, TArray<FPrimaryAssetId>&& AdditionalAssetIds, bool IsAutoTransition)
{
    if (mTransitionState != ESRPGRoomTransitionState::None)
    {
        UE_LOG(LogSRPGTransition, Log, TEXT("Can't load. Other room data is being loaded"));
        return;
    }
    mTransitionState = ESRPGRoomTransitionState::Loading;
    mIsAutoTransition = IsAutoTransition;
    mNextRoomId = RoomId;

    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    checkf(AssetManager != nullptr, TEXT("None Asset Manager"));

    TArray<FName> Bundles = { "World", "Actor" };
    FAssetManagerLoadParams LoadParams;
    LoadParams.OnComplete.BindUObject(this, &USRPGRoomTransitionSubsystem::OnLoadNextRoom);

    AdditionalAssetIds.Add(RoomId);
    mPreloadHandle = AssetManager->PreloadPrimaryAssets(AdditionalAssetIds, Bundles, true, MoveTemp(LoadParams));
}

void USRPGRoomTransitionSubsystem::TransitLoadedRoomAsync()
{
    if (mTransitionState == ESRPGRoomTransitionState::None)
    {
        UE_LOG(LogSRPGTransition, Log, TEXT("Can't transit. Room data must be loaded first"));
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

void USRPGRoomTransitionSubsystem::OnLoadNextRoom(TSharedPtr<FStreamableHandle> AssetHandle)
{
    mTransitionState = ESRPGRoomTransitionState::Loaded;

    // 자동 맵 전환 비활성화 시
    if (mIsAutoTransition == false)
    {
        return;
    }

    OnTransitNextRoom();
}

void USRPGRoomTransitionSubsystem::OnTransitNextRoom()
{
    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    checkf(AssetManager != nullptr, TEXT("None Asset Manager"));

    UStaticRoomSpawnData* StaticSpawnData = AssetManager->GetPrimaryAssetObject<UStaticRoomSpawnData>(mNextRoomId);
    checkf(StaticSpawnData != nullptr, TEXT("None Asset"));

    mTransitionState = ESRPGRoomTransitionState::None;
    mIsAutoTransition = false;

    UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), StaticSpawnData->mBackgroundMap);
}
