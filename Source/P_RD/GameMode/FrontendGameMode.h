/*****************************************************************//**
 * @file   FrontendGameMode.h
 * @brief  프론트엔드 GameMode 정의 헤더
 * @author 박용수, 모호재
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameMode/RDGameModeBase.h"
#include "Frontend/CharacterSelectTypes.h"
#include "Frontend/FrontendViewTypes.h"

#include "FrontendGameMode.generated.h"

class UStaticPlayerUnitSpawnData;

// Fronted 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogFrontendGameMode, Log, All)

/**
 * @brief 타이틀/캐릭터 선택/지도 팝업을 처리하는 프론트엔드 GameMode
 *
 * @details
 * HUD 클래스 선택은 BP_FrontendGameMode의 mHUDClass 기본값이 담당한다.
 * C++은 BP가 고른 HUD를 실행하고, UI가 기존 런/스테이지 데이터를 표시할 수 있도록 얇은 adapter만 제공한다.
 *
 * 프론트엔드는 실제 방이 아니므로 ARoomGameModeBase를 상속하지 않는다. ARoomGameModeBase는 방 진입 시
 * 플레이어 유닛 복원과 Run 저장을 수행하는데, 타이틀/지도 팝업 단계에서 그 흐름이 실행되면
 * "Front -> Room"과 "Room -> Room"의 책임이 섞인다. 따라서 이 클래스는 ARDGameModeBase를 상속하고,
 * 새 Run 시작 후 첫 방 입장은 PM 구조의 URoomTransitionSubsystem::MakeStageAndPreloadRoomAsync() 흐름에 맡긴다.
 * 지도 UI는 Run 시작 전 preview가 아니라, 활성 Run/Room 사이 이동 화면을 표시하는 adapter로 남긴다.
 *
 * @note API 출처
 * 이 클래스의 public 함수 대부분은 UI 파트가 타이틀 -> 캐릭터 선택 -> 첫 방 입장/지도 팝업을 붙이기 위해 만든
 * 임시 프론트엔드 facade/API다. 공식 게임 데이터는 PM 브랜치에서 들어온 URunPersistData,
 * GameProfileSubsystem, URoomTransitionSubsystem에 있고, 여기서는 그 결과를 UI DTO로 바꾸거나
 * 버튼 입력을 공식 API 호출로 연결하는 역할만 맡는다.
 *
 * 선택 가능한 다음 방 조회, 지도 노드 선택 검증, 선택된 방 입장 같은 정책 API는 최종적으로 PM 쪽 공식
 * API로 분리되어야 한다. 현재 함수들은 그 API가 생기기 전까지 WBP 연결을 유지하기 위한 adapter다.
 */
UCLASS(abstract)
class P_RD_API AFrontendGameMode : public ARDGameModeBase
{
	GENERATED_BODY()

public:
	AFrontendGameMode();

protected:
	void BeginRoom() override;

	/* UI 진입점 */
public:
	/**
	 * @brief 타이틀 START 입력을 처리해 캐릭터 선택 화면을 열도록 요청한다.
	 * @return 캐릭터 선택 화면 전환 요청에 성공하면 true
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool CreateNewRunFromTitle();

	/**
	 * @brief 선택한 플레이어 유닛과 난이도로 새 런을 시작한다.
	 * @return 런/지도 준비 요청에 성공하면 true
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool StartNewRun(const FPrimaryAssetId& PlayerUnitId, int32 Difficulty);

	/**
	 * @brief 타이틀/세팅 UI에서 기존 런 포기를 요청한다.
	 * @return 런 포기 처리가 가능하면 true
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool AbandonRunFromTitle();

public:
	/**
	 * @brief 캐릭터 선택 UI에 보여줄 카드 View 데이터를 가져온다.
	 * @param OutOptions 캐릭터 선택 카드에 표시할 View 데이터 배열
	 * @return 표시 가능한 캐릭터 후보가 있으면 true
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool GetCharacterOptions(TArray<FFrontendCharacterOption>& OutOptions) const;

protected:
	/**
	 * @brief StaticFrontendSpawnData에 등록된 플레이어 유닛 PrimaryAsset 목록을 가져온다.
	 * @return 조회된 플레이어 유닛 PrimaryAsset 배열
	 */
	const TArray<TSoftObjectPtr<UStaticPlayerUnitSpawnData>>& GetPlayerUnitDatas() const;
	/**
	 * @brief PlayerUnitId가 지정 가능한 유닛 ID가 맞는지 검증한다.
	 * @param PlayerUnitId 검사할 플레이어 유닛 PrimaryAssetId
	 * @return 유효성 여부
	 */
	bool IsPlayerUnitIdValid(const FPrimaryAssetId& PlayerUnitId) const;
	/**
	 * @brief Difficulty가 지정 가능한 난이도가 맞는지 검증한다.
	 * @param Difficulty 검사할 난이도
	 * @return 유효성 여부
	 */
	bool IsDifficultyValid(int32 Difficulty) const;

private:
	bool OpenTitleCharacterSelect();
	bool CreateRunData(const FPrimaryAssetId& PlayerUnitId, int32 Difficulty);

private:
	mutable TArray<TSoftObjectPtr<UStaticPlayerUnitSpawnData>> mPlayerUnitDataCache;
};
