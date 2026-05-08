/*****************************************************************//**
 * @file   StaticRoomSpawnData.h
 * @brief  방 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-03
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "StaticRoomSpawnData.generated.h"

/**
 * @brief  방 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS(abstract)
class P_RD_API UStaticRoomSpawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    void PostLoad() override
    {
        Super::PostLoad();

        UE_LOG(LogTemp, Warning, TEXT("Time: %f, PostLoad: %s"), FPlatformTime::Seconds(), *GetName());
    }

    void BeginDestroy() override
    {
        UE_LOG(LogTemp, Warning, TEXT("Time: %f, BeginDestroy: %s"), FPlatformTime::Seconds(), *GetName());

        Super::BeginDestroy();
    }

public:
	UPROPERTY(Category = "Room", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Level"))
	int32 mLevel;

public:
	UPROPERTY(Category = "Background", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "BackgroundMap", AssetBundles = "World"))
	TSoftObjectPtr<UWorld> mBackgroundMap;
};
