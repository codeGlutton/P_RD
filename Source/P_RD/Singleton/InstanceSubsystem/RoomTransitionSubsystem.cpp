#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "Engine/AssetManager.h"
#include "DataAsset/BundleType.h"
#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"
#include "Kismet/GameplayStatics.h"

#include "Setting/GamePlaySettings.h"

DEFINE_LOG_CATEGORY(LogTransition)

namespace
{
    bool IsUsableGameModeClass(const TSoftClassPtr<AGameModeBase>& GameModeClass)
    {
        const FString GameModePath = GameModeClass.ToSoftObjectPath().GetAssetPathString();
        return GameModeClass.IsNull() == false && GameModePath.IsEmpty() == false && GameModePath != TEXT("None");
    }

    bool IsUsableWorldAsset(const TSoftObjectPtr<UWorld>& WorldAsset)
    {
        const FString WorldPath = WorldAsset.ToSoftObjectPath().GetAssetPathString();
        return WorldAsset.IsNull() == false && WorldPath.IsEmpty() == false && WorldPath != TEXT("None");
    }

    TSoftClassPtr<AGameModeBase> GetFallbackGameModeClass(ERoomType RoomType, const UGamePlaySettings* GamePlaySettings)
    {
        checkf(GamePlaySettings != nullptr, TEXT("게임플레이 세팅 nullptr"));

        switch (RoomType)
        {
        case ERoomType::Monster:
        case ERoomType::EliteMonster:
        case ERoomType::BossMonster:
            return GamePlaySettings->mCombatGameMode;
        case ERoomType::Shop:
            return GamePlaySettings->mShopGameMode;
        case ERoomType::Treasure:
            return GamePlaySettings->mTreasureGameMode;
        default:
            return TSoftClassPtr<AGameModeBase>();
        }
    }

    TSoftClassPtr<AGameModeBase> ResolveRoomGameModeClass(const UStaticRoomSpawnData* StaticRoomData, ERoomType RoomType, const UGamePlaySettings* GamePlaySettings)
    {
        checkf(StaticRoomData != nullptr, TEXT("정적 룸 데이터 nullptr"));

        if (IsUsableGameModeClass(StaticRoomData->mGameModeBase))
        {
            return StaticRoomData->mGameModeBase;
        }

        return GetFallbackGameModeClass(RoomType, GamePlaySettings);
    }

    TSoftObjectPtr<UWorld> ResolveRoomBackgroundMap(const UStaticRoomSpawnData* StaticRoomData, const UGamePlaySettings* GamePlaySettings)
    {
        checkf(StaticRoomData != nullptr, TEXT("정적 룸 데이터 nullptr"));
        checkf(GamePlaySettings != nullptr, TEXT("게임플레이 세팅 nullptr"));

        if (IsUsableWorldAsset(StaticRoomData->mBackgroundMap))
        {
            return StaticRoomData->mBackgroundMap;
        }

        return GamePlaySettings->mDefaultRoomMap;
    }
}

void URoomTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URoomTransitionSubsystem::PreloadTitleRoomAsync(bool IsAutoTransition)
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
    mRequest.mChangePersistentData = false;

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
        TSharedPtr<FStreamableHandle> NewPreloadHandle = AssetManager->PreloadPrimaryAssets({ GamePlaySettings->mTitleRoomId }, Bundles, true, MoveTemp(LoadParams));
        mRoomPreloadHandle = NewPreloadHandle;
    }
}

void URoomTransitionSubsystem::PreloadRoomAsync(int32 RoomRowIndex, int32 RoomColumnIndex, FOnPreTransitNextRoom Callback, bool IsAutoTransition)
{
    FRoomTransitionRequest Request;
    Request.mChangePersistentData = true;
    Request.mRoomRowIndex = RoomRowIndex;
    Request.mRoomColumnIndex = RoomColumnIndex;
    Request.OnPreTransitNextRoom = Callback;

    PreloadRoomAsync(MoveTemp(Request), IsAutoTransition);
}

void URoomTransitionSubsystem::PreloadRoomAsync(FRoomTransitionRequest Request, bool IsAutoTransition)
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
}

void URoomTransitionSubsystem::MakeStageAndPreloadRoomAsync(EStageLevelType StageLevel, FOnPreTransitNextRoom Callback, bool IsAutoTransition)
{
    if (EnumHasAnyFlags(mTransitionState, ERoomTransitionStateFlag::AllTaskRequested) == true)
    {
        UE_LOG(LogTransition, Log, TEXT("다른 룸 데이터 처리 중으로 Preload 불가"));
        return;
    }
    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::NewStageRequested);

    // 스테이지 먼저 생성
    GetRunMutableData()->MakeStageAsync(StageLevel, FOnCreateStage::CreateLambda([this, Callback, IsAutoTransition](const FStage& NewStage) {
        EnumRemoveFlags(mTransitionState, ERoomTransitionStateFlag::NewStageRequested);

        // 이후 스테이지의 배정된 첫 방으로 Preload 시작
        const FRoom& StartRoom = NewStage.GetStartRoom();
        PreloadRoomAsync(StartRoom.mRow, StartRoom.mColumn, Callback, IsAutoTransition);
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

void URoomTransitionSubsystem::OnLoadNextPlayer(TSharedPtr<FStreamableHandle> AssetHandle)
{
    EnumAddFlags(mTransitionState, ERoomTransitionStateFlag::PlayerLoaded);

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

    // 에셋 로드가 완료된 경우
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
    /* 방 이동 전 대리자 */

    if (mRequest.OnPreTransitNextRoom.IsBound() == true)
    {
        mRequest.OnPreTransitNextRoom.Execute(mRequest.mRoomRowIndex, mRequest.mRoomColumnIndex);
        mRequest.OnPreTransitNextRoom.Unbind();
    }
    mTransitionState = ERoomTransitionStateFlag::None;

    /* 현재 방 기록 */

    if (mRequest.mChangePersistentData == true)
    {
        GetRunMutableData()->SetCurrentRoomIndex(mRequest.mRoomRowIndex, mRequest.mRoomColumnIndex);
    }

    /* 방 이동 */

    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

    const FRoom& NextRoom = GetRunMutableData()->GetRoom(mRequest.mRoomRowIndex, mRequest.mRoomColumnIndex);
    UStaticRoomSpawnData* StaticRoomData = AssetManager->GetPrimaryAssetObject<UStaticRoomSpawnData>(NextRoom.mStaticRoomSpawnDataId);
    if (StaticRoomData == nullptr && NextRoom.mStaticRoomSpawnDataId.IsValid())
    {
        const FSoftObjectPath RoomDataPath = AssetManager->GetPrimaryAssetPath(NextRoom.mStaticRoomSpawnDataId);
        StaticRoomData = Cast<UStaticRoomSpawnData>(RoomDataPath.TryLoad());
    }
    if (StaticRoomData == nullptr)
    {
        UE_LOG(LogTransition, Error, TEXT("방 이동 실패. 룸 정보를 찾을 수 없습니다. RoomType: %d, RoomData: %s"),
            StaticCast<uint8>(NextRoom.mType),
            *NextRoom.mStaticRoomSpawnDataId.ToString());
        return;
    }

    const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
    const TSoftClassPtr<AGameModeBase> GameModeClass = ResolveRoomGameModeClass(StaticRoomData, NextRoom.mType, GamePlaySettings);
    const TSoftObjectPtr<UWorld> BackgroundMap = ResolveRoomBackgroundMap(StaticRoomData, GamePlaySettings);
    const FString Option = IsUsableGameModeClass(GameModeClass) ? FString::Printf(TEXT("game=%s"), *GameModeClass.ToSoftObjectPath().GetAssetPathString()) : FString();
    if (IsUsableWorldAsset(BackgroundMap) == false)
    {
        UE_LOG(LogTransition, Error, TEXT("방 이동 실패. 사용할 배경 맵이 없습니다. RoomType: %d, RoomData: %s"),
            StaticCast<uint8>(NextRoom.mType),
            *NextRoom.mStaticRoomSpawnDataId.ToString());
        return;
    }

    const FSoftObjectPath BackgroundMapPath = BackgroundMap.ToSoftObjectPath();
    const FString BackgroundMapPackageName = BackgroundMapPath.GetLongPackageName();
    UE_LOG(LogTransition, Display, TEXT("방 이동. RoomType: %d, RoomData: %s, Map: %s, GameMode: %s, Option: %s"),
        StaticCast<uint8>(NextRoom.mType),
        *NextRoom.mStaticRoomSpawnDataId.ToString(),
        *BackgroundMapPath.GetAssetPathString(),
        *GameModeClass.ToSoftObjectPath().GetAssetPathString(),
        *Option);
    UGameplayStatics::OpenLevel(GetWorld(), FName(*BackgroundMapPackageName), true, Option);
}
