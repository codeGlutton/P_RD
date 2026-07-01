/*****************************************************************//**
 * @file   RoomUIModel.h
 * @brief  방 공통(상단바) 표시용 뷰모델 정의
 *
 * @details
 * 모든 방(전투/상점/보물)의 상단바가 공통으로 읽는 표시값을 담는 뷰모델이다.
 * ARoomGameModeBase가 하나 소유해 모든 방이 상속으로 공유하며, 게임플레이가
 * Set*()으로 값을 밀어넣으면 OnUIChanged로 알려 상단바 위젯이 다시 그린다.
 * (전투 전용 표시는 각 방의 UIModel — 예: UCombatUIModel — 이 따로 소유한다.)
 * @date   2026-07-01
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "RoomUIModel.generated.h"

/** @brief 모든 방(전투/상점/보물)의 상단바가 공통으로 표시하는 플레이어 요약입니다. */
// 방 종류와 무관하게 상단바에 뜨는 값이라, 방 전용 UIModel이 아니라 RoomUIModel(공통)이 소유한다.
USTRUCT(BlueprintType)
struct FRoomPlayerSummaryUI
{
	GENERATED_BODY()

	// 현재 플레이어 레벨. [소스] URunPersistData::GetPlayerLevel().
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 0;
	// 현재/최대 체력. [합의필요] 실제 소스는 플레이어 유닛 속성(ASC 정리 후 연결).
	UPROPERTY(BlueprintReadOnly) int32 mHP = 0;
	UPROPERTY(BlueprintReadOnly) int32 mMaxHP = 0;
	// 보유 골드. [합의필요] 실제 소스는 플레이어 유닛 속성/런 데이터(ASC 정리 후 연결).
	UPROPERTY(BlueprintReadOnly) int32 mGold = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoomUIChanged);

/**
 * @brief 방 공통(상단바) 표시용 뷰모델.
 *
 * RoomGameModeBase가 하나 소유해 모든 방(Combat/Shop/Treasure)이 공유한다.
 * 게임플레이가 Set*()으로 값을 밀어넣으면 OnUIChanged로 알리고,
 * 상단바 위젯은 Get*()으로 다시 읽어 자기 표시를 그린다.
 * (전투 전용 표시는 각 방의 UIModel — 예: UCombatUIModel — 이 따로 소유한다.)
 */
UCLASS(BlueprintType)
class P_RD_API URoomUIModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 상단바가 구독하는 알림 ───────── */
public:
	/** @brief 값이 바뀌면 발신한다. 상단바 위젯이 구독해 해당 표시를 다시 그린다. */
	UPROPERTY(BlueprintAssignable, Category = "Room|View")
	FOnRoomUIChanged OnUIChanged;

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ───────── */
public:
	/**
	 * @brief  방 공통 플레이어 요약(레벨/HP/골드)을 저장하고 변경을 알린다.
	 * @param  Summary 게임플레이가 만들어 넣는 새 플레이어 요약 값.
	 * @note   저장 후 OnUIChanged를 발신하므로, 구독 중인 상단바가 스스로 다시 그린다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Room|Push")
	void SetPlayerSummary(const FRoomPlayerSummaryUI& Summary);

	/* ───────── 위젯이 읽는다 ───────── */
public:
	/**
	 * @brief  마지막으로 push된 플레이어 요약을 읽는다.
	 * @return 현재 상단바 요약 스냅샷(레벨/HP/골드). 위젯은 참조로 읽고 수정하지 않는다.
	 */
	UFUNCTION(BlueprintPure, Category = "Room|Read")
	const FRoomPlayerSummaryUI& GetPlayerSummary() const { return mPlayerSummary; }

private:
	UPROPERTY(Transient)
	FRoomPlayerSummaryUI mPlayerSummary;
};
