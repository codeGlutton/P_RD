#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"

#include "Engine/AssetManager.h"
#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"

DEFINE_LOG_CATEGORY(LogTransition)

void URoomTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URoomTransitionSubsystem::PreloadRoomAsync(const FPrimaryAssetId& StageId, const FPrimaryAssetId& RoomId, bool IsAutoTransition)
{
    PreloadRoomAsync(StageId, RoomId, TArray<FPrimaryAssetId>(), IsAutoTransition);
}

void URoomTransitionSubsystem::PreloadRoomAsync(const FPrimaryAssetId& StageId, const FPrimaryAssetId& RoomId, TArray<FPrimaryAssetId>&& AdditionalAssetIds, bool IsAutoTransition)
{
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::PreLoadRequested) == true)
    {
        UE_LOG(LogTransition, Log, TEXT("다른 룸 데이터 처리 중으로 Preload 불가"));
        return;
    }

    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::PreLoadRequested);
    if (IsAutoTransition == true)
    {
        EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::AutoTransition);
    }
    mNextRoomId = RoomId;

    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

    {
        TArray<FName> Bundles = { "UI" };
        FAssetManagerLoadParams LoadParams;
        LoadParams.OnComplete.BindUObject(this, &URoomTransitionSubsystem::OnLoadNextStage);

        TSharedPtr<FStreamableHandle> NewPreloadHandle = AssetManager->PreloadPrimaryAssets({ StageId }, Bundles, true, MoveTemp(LoadParams));
        mStagePreloadHandle = NewPreloadHandle;
    }

    {
        TArray<FName> Bundles = { "World", "Actor" };
        FAssetManagerLoadParams LoadParams;
        LoadParams.OnComplete.BindUObject(this, &URoomTransitionSubsystem::OnLoadNextRoom);

        AdditionalAssetIds.Add(RoomId);
        TSharedPtr<FStreamableHandle> NewPreloadHandle = mRoomPreloadHandle = AssetManager->PreloadPrimaryAssets(AdditionalAssetIds, Bundles, true, MoveTemp(LoadParams));
        mRoomPreloadHandle = NewPreloadHandle;
    }
}

void URoomTransitionSubsystem::TransitLoadedRoomAsync()
{
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::PreLoadRequested) == false)
    {
        UE_LOG(LogTransition, Log, TEXT("전환 불가. 먼저 방 데이터 로드 요청 필요"));
        return;
    }

    // 에셋 로드가 아직 완료되지 않은 경우
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::ReadyToTransition) == false)
    {
        // 자동 맵 전환 예약
        EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::AutoTransition);
        return;
    }

    // 에셋 로드가 완료된 경우
    OnTransitNextRoom();
}

void URoomTransitionSubsystem::OnLoadNextStage(TSharedPtr<FStreamableHandle> AssetHandle)
{
    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::StageLoaded);

    // 에셋 로드가 아직 완료되지 않은 경우
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::ReadyToTransition) == false)
    {
        return;
    }

    // 자동 맵 전환 비활성화 시
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::AutoTransition) == false)
    {
        return;
    }

    OnTransitNextRoom();
}

void URoomTransitionSubsystem::OnLoadNextRoom(TSharedPtr<FStreamableHandle> AssetHandle)
{
    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::RoomLoaded);

    // 에셋 로드가 아직 완료되지 않은 경우
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::ReadyToTransition) == false)
    {
        return;
    }

    // 자동 맵 전환 비활성화 시
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::AutoTransition) == false)
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

    mTransitionState = ERoomTransitionStateFlag::None;

    UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), StaticRoomData->mBackgroundMap);
}
