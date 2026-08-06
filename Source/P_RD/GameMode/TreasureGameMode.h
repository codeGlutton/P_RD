/*****************************************************************//**
 * @file   TreasureGameMode.h
 * @brief  보상 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "TreasureGameMode.generated.h"

class UTreasureUIModel;

/**
 * @brief  보상 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API ATreasureGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

public:
	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/**
	 * @brief 보물방 화면이 읽을 뷰모델
	 *
	 * @details
	 * 게임모드가 SetTreasure()로 밀어넣고 위젯은 GetTreasure()로 읽는 구조
	 * 개봉과 나가기는 의도만 전달
	 * 보상 지급과 개봉 판정은 게임모드 담당
	 * @return 뷰모델
	 */
	UFUNCTION(BlueprintPure, Category = "Treasure")
	UTreasureUIModel* GetTreasureUIModel() const { return mTreasureUIModel; }

protected:
	void InitializeRoom() override;
	void BeginRoom() override;

private:
	/**
	 * @brief 보물상자 액터를 방 스타트포인트에 스폰 (연출용)
	 * @details 스폰데이터에 상자 클래스가 없으면 스킵
	 */
	void SpawnTreasureBox();

	/** @brief 상자 상태와 지급 내역을 화면에 내림 */
	void PushTreasureUIData();

	/**
	 * @brief 상자 개봉 및 보상 전부 지급
	 * @details 골드와 아티팩트를 순서대로 지급 후 개봉 상태로 갱신. 재개봉 불가
	 */
	UFUNCTION() void HandleOpenRequested();

	/** @brief 나가기 의도 처리. 다음 방 선택은 지도(월드맵) 담당 */
	UFUNCTION() void HandleLeaveRequested();

	/** @brief 파티 골드 지급. 0 이하 금액은 무시 */
	void GivePartyGold(int32 Amount);

	/** @brief 현재 파티 골드. @return 없으면 0 */
	int32 GetPartyGold() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTreasureUIModel> mTreasureUIModel;

	// @brief 스폰된 보물상자 액터 (연출용)
	UPROPERTY(Transient)
	TObjectPtr<AActor> mTreasureBox;

	/**
	 * @brief 상자 개봉 여부 (재개봉 가드 + 화면 상태 소스)
	 *
	 * @details
	 * 방에 머무는 동안만 유효한 런타임 상태. 런 저장이 방 입장 시점에만
	 * 이뤄지므로 종료→로드 시 보상 지급까지 통째로 롤백되어 세이브 불필요.
	 * 개봉 후 저장을 추가하는 경우 이 값도 세이브 대상에 포함할 것
	 */
	bool mOpened = false;

	// @brief 실제 지급에 성공한 아티팩트 ID (지급 내역 표시용)
	TArray<FPrimaryAssetId> mGrantedArtifactIds;
};
