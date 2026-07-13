/*****************************************************************//**
 * @file   RDWorldSettings.h
 * @brief  Rogue dice의 World Setting 클래스 정의 헤더
 * @author 모호재
 * @date   2026-05-26
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "RDWorldSettings.generated.h"

USTRUCT(BlueprintType)
struct FRoomSpawnSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = StartPoint, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "MainCameraPoint"))
	TObjectPtr<AActor> mMainCameraPoint;

	UPROPERTY(Category = StartPoint, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "RoomStartPoint"))
	TObjectPtr<AActor> mRoomStartPoint;
};

/**
 * @brief  Rogue dice의 World Setting 클래스
 */
UCLASS()
class P_RD_API ARDWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	FName GetRandomRoomSpawnSettingName(const FRandomStream& Stream) const;

public:
	AActor* GetMainCameraPoint(const FName& Name) const;
	AActor* GetRoomStartPoint(const FName& Name) const;
	
protected:
	UPROPERTY(Category = Room, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SpawnSettings"))
	TMap<FName, FRoomSpawnSettings> mSpawnSettings;
};
