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

	/**
	 * @brief 지정 전용 스폰 세팅 여부
	 * @details
	 * true면 방 DA가 DefaultSpawnSettingName으로 이름을 지정할 때만 사용되고,
	 * 랜덤 추첨(GetRandomRoomSpawnSettingName) 대상에서 제외됨.
	 * 상점방처럼 특정 방 전용 세팅을 공유맵에 둘 때 사용.
	 */
	UPROPERTY(Category = StartPoint, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "IsDedicated"))
	bool mIsDedicated = false;
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
