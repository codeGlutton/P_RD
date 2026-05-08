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
    /**
     * @brief 현재 방이 등장할 수 있는 스테이지 레벨
     * @details
     * Primary Asset을 방 타입과 레벨 별로 분류해두었기 때문에, 해당 값은 Primary Asset Type에 영향을 줌
     */
	UPROPERTY(Category = "Stage", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StageLevel"))
	int32 mStageLevel;

public:
	UPROPERTY(Category = "Background", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "BackgroundMap", AssetBundles = "World"))
	TSoftObjectPtr<UWorld> mBackgroundMap;
};
