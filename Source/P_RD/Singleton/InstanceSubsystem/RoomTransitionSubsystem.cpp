#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "Engine/AssetManager.h"
#include "DataAsset/BundleType.h"
#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"

#include "Setting/GamePlaySettings.h"

DEFINE_LOG_CATEGORY(LogTransition)

void URoomTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

bool URoomTransitionSubsystem::PreloadFrontendRoomAsync(FOnReadyToTransition ReadyToTransitionCallback, FOnPreTransitNextRoom PreTransitionCallback, bool RequireExternalReady, bool IsAutoTransition)
{
    if (EnumHasAnyFlags(mTransitionState, ERoomTransitionStateFlag::AllTaskRequested) == true)
    {
        UE_LOG(LogTransition, Log, TEXT("다른 룸 데이터 처리 중으로 Preload 불가"));
        return false;
    }

    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::PreLoadRequested);
    if (RequireExternalReady == false)
    {
        EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::ExternalReady);
    }
    if (IsAutoTransition == true)
    {
        EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::AutoTransition);
    }
    mRequest.mNeedInGameRoom = false;
    mRequest.mRoomRowIndex = INDEX_NONE;
    mRequest.mRoomColumnIndex = INDEX_NONE;
    mRequest.OnReadyToTransition = ReadyToTransitionCallback;
    mRequest.OnPreTransitNextRoom = PreTransitionCallback;

    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

    {
        OnLoadNextPlayer(nullptr);
        mPlayerPreloadHandle = nullptr;
    }

    {
        OnLoadNextStage(nullptr);
        mStagePreloadHandle = nullptr;
    }

    {
        TArray<FName> Bundles = { BULDLE_ALL };
        FAssetManagerLoadParams LoadParams;
        LoadParams.OnComplete.BindUObject(this, &URoomTransitionSubsystem::OnLoadNextRoom);

        const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
        TSharedPtr<FStreamableHandle> NewPreloadHandle = AssetManager->PreloadPrimaryAssets({ GamePlaySettings->mFrontendRoomId }, Bundles, true, MoveTemp(LoadParams));
        mRoomPreloadHandle = NewPreloadHandle;
    }

    return true;
}

/**
 * @brief 방 좌표를 받아 방 전환 준비를 시작한다.
 *
 * @details
 * 이 함수는 방 좌표로 FRoomTransitionRequest를 만든 뒤 실제 처리 함수로 넘깁니다.
 * 여기서 중요한 점은 RequireExternalReady와 IsAutoTransition을 둘 다 그대로 넘기는 것입니다.
 * 하나가 빠지면 "외부 준비를 기다릴지"와 "자동으로 이동할지"가 서로 잘못 해석될 수 있습니다.
 *
 * @param RoomRowIndex 전환할 방의 행 index.
 * @param RoomColumnIndex 전환할 방의 열 index.
 * @param ReadyToTransitionCallback 전환 준비 완료 시 호출할 콜백.
 * @param PreTransitionCallback 실제 전환 직전 호출할 콜백.
 * @param RequireExternalReady 에셋 로드 후 외부 준비 신호까지 기다릴지 여부.
 * @param IsAutoTransition 준비 완료 후 자동으로 전환을 시작할지 여부.
 * @return 전환 준비 요청을 받았으면 true, 이미 다른 전환 중이면 false.
 */
bool URoomTransitionSubsystem::PreloadRoomAsync(int32 RoomRowIndex, int32 RoomColumnIndex, FOnReadyToTransition ReadyToTransitionCallback, FOnPreTransitNextRoom PreTransitionCallback, bool RequireExternalReady, bool IsAutoTransition)
{
    FRoomTransitionRequest Request;
    Request.mNeedInGameRoom = true;
    Request.mRoomRowIndex = RoomRowIndex;
    Request.mRoomColumnIndex = RoomColumnIndex;
    Request.OnReadyToTransition = ReadyToTransitionCallback;
    Request.OnPreTransitNextRoom = PreTransitionCallback;

    // 두 옵션을 모두 넘겨야 "기다릴지/자동 이동할지"가 바뀌지 않는다.
    return PreloadRoomAsync(MoveTemp(Request), RequireExternalReady, IsAutoTransition);
}

bool URoomTransitionSubsystem::PreloadRoomAsync(FRoomTransitionRequest Request, bool RequireExternalReady, bool IsAutoTransition)
{
    if (EnumHasAnyFlags(mTransitionState, ERoomTransitionStateFlag::AllTaskRequested) == true)
    {
        UE_LOG(LogTransition, Log, TEXT("다른 룸 데이터 처리 중으로 Preload 불가"));
        return false;
    }

    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::PreLoadRequested);
    if (RequireExternalReady == false)
    {
        EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::ExternalReady);
    }
    if (IsAutoTransition == true)
    {
        EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::AutoTransition);
    }

    mRequest = Request;

    TArray<FPrimaryAssetId> PlayerIds;
    FPrimaryAssetId StageId;
    FPrimaryAssetId RoomId;
    TArray<FPrimaryAssetId> AdditionalIds;
    GetRunMutableData()->CollectAssetIds(mRequest.mRoomRowIndex, mRequest.mRoomColumnIndex, OUT PlayerIds, OUT StageId, OUT RoomId, OUT AdditionalIds);

    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

    {
        TArray<FName> Bundles = { BULDLE_ALL };
        FAssetManagerLoadParams LoadParams;
        LoadParams.OnComplete.BindUObject(this, &URoomTransitionSubsystem::OnLoadNextPlayer);

        TSharedPtr<FStreamableHandle> NewPreloadHandle = AssetManager->PreloadPrimaryAssets(PlayerIds, Bundles, true, MoveTemp(LoadParams));
        mPlayerPreloadHandle = NewPreloadHandle;
    }

    {
        TArray<FName> Bundles = { BUNDLE_PAD, BUNDLE_UI };
        FAssetManagerLoadParams LoadParams;
        LoadParams.OnComplete.BindUObject(this, &URoomTransitionSubsystem::OnLoadNextStage);

        TSharedPtr<FStreamableHandle> NewPreloadHandle = AssetManager->PreloadPrimaryAssets({ StageId }, Bundles, true, MoveTemp(LoadParams));
        mStagePreloadHandle = NewPreloadHandle;
    }

    {
        TArray<FName> Bundles = { BULDLE_ALL };
        FAssetManagerLoadParams LoadParams;
        LoadParams.OnComplete.BindUObject(this, &URoomTransitionSubsystem::OnLoadNextRoom);

        AdditionalIds.Add(RoomId);
        TSharedPtr<FStreamableHandle> NewPreloadHandle = AssetManager->PreloadPrimaryAssets(AdditionalIds, Bundles, true, MoveTemp(LoadParams));
        mRoomPreloadHandle = NewPreloadHandle;
    }

    return true;
}

bool URoomTransitionSubsystem::MakeStageAndPreloadRoomAsync(EStageLevelType StageLevel, FOnReadyToTransition ReadyToTransitionCallback, FOnPreTransitNextRoom PreTransitionCallback, bool RequireExternalReady, bool IsAutoTransition)
{
    if (EnumHasAnyFlags(mTransitionState, ERoomTransitionStateFlag::AllTaskRequested) == true)
    {
        UE_LOG(LogTransition, Log, TEXT("다른 룸 데이터 처리 중으로 Preload 불가"));
        return false;
    }
    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::NewStageRequested);

    // 스테이지 먼저 생성
    GetRunMutableData()->MakeStageAsync(StageLevel, FOnCreateStage::CreateLambda([this, ReadyToTransitionCallback, PreTransitionCallback, RequireExternalReady, IsAutoTransition](const FStage& NewStage) {
        EnumRemoveFlags(mTransitionState, ERoomTransitionStateFlag::NewStageRequested);

        // 이후 스테이지의 배정된 첫 방으로 Preload 시작
        const FRoom& StartRoom = NewStage.GetStartRoom();
        const bool bPreloadRequested = PreloadRoomAsync(StartRoom.mRow, StartRoom.mColumn, ReadyToTransitionCallback, PreTransitionCallback, RequireExternalReady, IsAutoTransition);
        checkf(bPreloadRequested == true, TEXT("만들어진 Stage로 Preload 시도 실패"));
        if (bPreloadRequested == false)
        {
            UE_LOG(LogTransition, Error, TEXT("만들어진 Stage로 Preload 시도 실패"));
        }
        }));

    return true;
}

bool URoomTransitionSubsystem::MarkExternalReady()
{
    if (EnumHasAnyFlags(mTransitionState, ERoomTransitionStateFlag::AllTaskRequested) == false)
    {
        UE_LOG(LogTransition, Log, TEXT("전환 불가. 먼저 방 데이터 로드 요청 필요"));
        return false;
    }

    if (EnumHasAnyFlags(mTransitionState, ERoomTransitionStateFlag::ExternalReady) == true)
    {
        UE_LOG(LogTransition, Log, TEXT("이미 마크 완료"));
        return false;
    }
    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::ExternalReady);

    // 에셋 로드가 아직 완료되지 않은 경우
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::ReadyToTransition) == false)
    {
        return true;
    }

    // 모든 준비가 완료된 경우
    OnReadyToTransition();
    return true;
}

/**
 * @brief 준비만 끝난 방 전환을 실제로 시작한다.
 *
 * @details
 * AutoTransition=false로 준비한 방은 에셋 로드가 끝나도 바로 이동하지 않습니다.
 * GameMode가 로딩 UI나 닫힘 애니메이션을 끝낸 뒤 이 함수를 호출해 실제 이동을 시작합니다.
 *
 * ReadyToTransition이 없으면 아직 이동할 방 정보가 완성되지 않은 상태라서 이동하지 않습니다.
 *
 * @return 실제 이동을 시작했으면 true, 아직 준비 전이거나 이미 이동 중이면 false.
 */
bool URoomTransitionSubsystem::TransitLoadedRoom()
{
    // 아직 준비가 끝나지 않았으면 OpenLevel을 시작하면 안 된다.
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::ReadyToTransition) == false)
    {
        UE_LOG(LogTransition, Log, TEXT("전환 불가. 먼저 전환 준비 필요"));
        return false;
    }

    if (EnumHasAnyFlags(mTransitionState, ERoomTransitionStateFlag::AutoTransition) == true)
    {
        UE_LOG(LogTransition, Log, TEXT("이미 전환 중"));
        return false;
    }
    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::AutoTransition);

    // 전환 시작
    OnTransitNextRoom();
    return true;
}

void URoomTransitionSubsystem::OnLoadNextPlayer(TSharedPtr<FStreamableHandle> AssetHandle)
{
    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::PlayerLoaded);

    // 에셋 로드가 아직 완료되지 않은 경우
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::ReadyToTransition) == false)
    {
        return;
    }

    // 모든 준비가 완료된 경우
    OnReadyToTransition();
}

void URoomTransitionSubsystem::OnLoadNextStage(TSharedPtr<FStreamableHandle> AssetHandle)
{
    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::StageLoaded);

    // 에셋 로드가 아직 완료되지 않은 경우
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::ReadyToTransition) == false)
    {
        return;
    }

    // 모든 준비가 완료된 경우
    OnReadyToTransition();
}

void URoomTransitionSubsystem::OnLoadNextRoom(TSharedPtr<FStreamableHandle> AssetHandle)
{
    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::RoomLoaded);

    // 에셋 로드가 아직 완료되지 않은 경우
    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::ReadyToTransition) == false)
    {
        return;
    }

    // 모든 준비가 완료된 경우
    OnReadyToTransition();
}

void URoomTransitionSubsystem::OnReadyToTransition()
{
    // 준비 완료 콜백
    if (mRequest.OnReadyToTransition.IsBound() == true)
    {
        mRequest.OnReadyToTransition.Execute(mRequest.mRoomRowIndex, mRequest.mRoomColumnIndex);
        mRequest.OnReadyToTransition.Unbind();
    }

    if (EnumHasAllFlags(mTransitionState, ERoomTransitionStateFlag::InTransition) == false)
    {
        return;
    }

    // 전환 시작
    OnTransitNextRoom();
}

void URoomTransitionSubsystem::OnTransitNextRoom()
{
    // 전환 전 콜백
    if (mRequest.OnPreTransitNextRoom.IsBound() == true)
    {
        mRequest.OnPreTransitNextRoom.Execute(mRequest.mRoomRowIndex, mRequest.mRoomColumnIndex);
        mRequest.OnPreTransitNextRoom.Unbind();
    }

    mTransitionState = ERoomTransitionStateFlag::None;

    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

    UStaticRoomSpawnData* StaticRoomData = nullptr;
    if (mRequest.mNeedInGameRoom == true)
    {
        /* 현재 방 기록 */

        GetRunMutableData()->SetCurrentRoomIndex(mRequest.mRoomRowIndex, mRequest.mRoomColumnIndex);

        const FRoom& NextRoom = GetRunMutableData()->GetRoom(mRequest.mRoomRowIndex, mRequest.mRoomColumnIndex);
        StaticRoomData = AssetManager->GetPrimaryAssetObject<UStaticRoomSpawnData>(NextRoom.mStaticRoomSpawnDataId);
    }
    else
    {
        const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
        StaticRoomData = AssetManager->GetPrimaryAssetObject<UStaticRoomSpawnData>(GamePlaySettings->mFrontendRoomId);
    }
    checkf(StaticRoomData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));

    /* 방 이동 */

    TSoftObjectPtr<UWorld> BackgroundMap = StaticRoomData->mBackgroundMap;
    if (BackgroundMap.IsNull() == true)
    {
        BackgroundMap = GetDefault<UGamePlaySettings>()->mDefaultBackgroundMap;
        UE_LOG(
            LogTransition, Warning, TEXT("%s 방 에셋 내 배경 레벨이 비어있음. 기본 레벨로 이동: %s"),
            *StaticRoomData->GetPrimaryAssetId().ToString(),
            *BackgroundMap.ToSoftObjectPath().ToString()
        );
    }
    checkf(BackgroundMap.IsNull() == false, TEXT("방 전환 실패. %s의 배경 맵 설정이 존재하지 않음"), *StaticRoomData->GetPrimaryAssetId().ToString());

    FString Option = FString::Printf(TEXT("?game=%s"), *StaticRoomData->mGameModeBase.ToSoftObjectPath().GetAssetPathString());
    UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), BackgroundMap, true, Option);
}
