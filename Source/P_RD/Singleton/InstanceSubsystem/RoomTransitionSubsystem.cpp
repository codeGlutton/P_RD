#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"

#include "Engine/AssetManager.h"
#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"

DEFINE_LOG_CATEGORY(LogTransition)

void URoomTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URoomTransitionSubsystem::PreloadRoomAsync(const FPrimaryAssetId& StageId, const FPrimaryAssetId& RoomId, FOnPreTransitNextRoom Callback, bool IsAutoTransition)
{
    PreloadRoomAsync(StageId, RoomId, TArray<FPrimaryAssetId>(), MoveTemp(Callback), IsAutoTransition);
}

void URoomTransitionSubsystem::PreloadRoomAsync(const FPrimaryAssetId& StageId, const FPrimaryAssetId& RoomId, TArray<FPrimaryAssetId>&& AdditionalAssetIds, FOnPreTransitNextRoom Callback, bool IsAutoTransition)
{
    if (EnumHasAnyFlags(mTransitionState, ERoomTransitionStateFlag::AllTaskRequested) == true)
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
    OnPreTransitNextRoom_Internal.AddLambda([SId = StageId, RId = RoomId, Callback]() {
        Callback.ExecuteIfBound(SId, RId);
        });

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
        TSharedPtr<FStreamableHandle> NewPreloadHandle = AssetManager->PreloadPrimaryAssets(AdditionalAssetIds, Bundles, true, MoveTemp(LoadParams));
        mRoomPreloadHandle = NewPreloadHandle;
    }
}

void URoomTransitionSubsystem::MakeStageAndPreloadRoomAsync(EStageLevelType StageLevel, FOnPreTransitNextRoom Callback, bool IsAutoTransition)
{
    if (EnumHasAnyFlags(mTransitionState, ERoomTransitionStateFlag::AllTaskRequested) == true)
    {
        UE_LOG(LogTransition, Log, TEXT("다른 룸 데이터 처리 중으로 Preload 불가"));
        return;
    }
    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::NewStageRequested);

    UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
    checkf(PersistentDataSubsystem != nullptr, TEXT("에셋 매니저 nullptr"));

    // 스테이지 먼저 생성
    PersistentDataSubsystem->GetRunPersistData()->MakeStageAsync(StageLevel, FOnCreateStage::CreateLambda([this, Callback, IsAutoTransition](const FStage& NewStage) {
        EnumRemoveFlags(mTransitionState, ERoomTransitionStateFlag::NewStageRequested);

        // 이후 스테이지의 배정된 첫 방으로 Preload 시작
        FPrimaryAssetId RoomId;
        TArray<FPrimaryAssetId> AdditionalIds;
        NewStage.GetStartRoom().CollectAssetIds(OUT RoomId, OUT AdditionalIds);
        PreloadRoomAsync(NewStage.mStaticStageSpawnDataId, RoomId, MoveTemp(AdditionalIds), Callback, IsAutoTransition);
        }));
}

void URoomTransitionSubsystem::TransitLoadedRoomAsync()
{
    if (EnumHasAnyFlags(mTransitionState, ERoomTransitionStateFlag::AllTaskRequested) == false)
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

    OnPreTransitNextRoom_Internal.Broadcast();
    OnPreTransitNextRoom_Internal.Clear();
    mTransitionState = ERoomTransitionStateFlag::None;

    UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), StaticRoomData->mBackgroundMap);
}
