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
 * @brief 타이틀/캐릭터 선택을 처리하는 프론트엔드 GameMode
 *
 * @details
 * HUD 클래스 선택은 BP_FrontendGameMode의 mHUDClass 기본값이 담당한다.
 * C++은 BP가 고른 HUD를 실행하고, UI가 캐릭터 선택과 이어하기 상태를 표시할 수 있도록 얇은 adapter만 제공한다.
 *
 * 프론트엔드는 실제 방이 아니므로 ARoomGameModeBase를 상속하지 않는다. ARoomGameModeBase는 방 진입 시
 * 플레이어 유닛 복원과 Run 저장을 수행하는데, 타이틀 단계에서 그 흐름이 실행되면
 * "Front -> Room"과 "Room -> Room"의 책임이 섞인다. 따라서 이 클래스는 ARDGameModeBase를 상속하고,
 * 새 Run 시작 후 첫 방 입장은 URoomTransitionSubsystem::MakeStageAndPreloadRoomAsync() 흐름에 맡긴다.
 *
 * 이 클래스의 public 함수는 타이틀 -> 캐릭터 선택 -> 첫 방 입장을 연결하는
 * 프론트엔드 진입점이다. 실제 게임 데이터는 URunPersistData, GameProfileSubsystem,
 * URoomTransitionSubsystem에 있고, 여기서는 그 결과를 UI DTO로 바꾸거나 버튼 입력을
 * 게임 API 호출로 연결하는 역할만 맡는다.
 *
 * 선택 가능한 다음 방 조회, 지도 노드 선택 검증, 선택된 방 입장 같은 정책은
 * 실제 방에 들어간 뒤 ARoomGameModeBase와 월드맵 위젯이 판단한다.
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
	 *
	 * @details
	 * 이 함수 이름은 기존 타이틀 흐름과 맞추기 위해 남아 있지만, 실제 RunPersistData를 즉시 만들지는 않는다.
	 * START 시점에는 아직 캐릭터가 정해지지 않았으므로, 여기서는 TitleMenuWidget에게 캐릭터 선택 화면을 열게 하는 것이 전부다.
	 * Run 생성은 캐릭터 선택 Confirm 이후 StartNewRun()에서만 진행한다.
	 *
	 * @return 캐릭터 선택 화면 전환 요청에 성공하면 true
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool CreateNewRunFromTitle();

	/**
	 * @brief 선택한 플레이어 유닛과 난이도로 새 런을 시작한다.
	 *
	 * @details
	 * CharacterSelectWidget이 선택 결과를 확정한 뒤 호출하는 진입점이다.
	 * PlayerUnitId 검증, RunPersistData 생성, Stage1 첫 방 프리로드 요청을 순서대로 수행한다.
	 * UI가 직접 Subsystem을 호출하지 않고 이 함수로 들어오게 한 이유는
	 * "캐릭터 선택 완료 -> 런 생성 -> 첫 방 전환" 순서를 GameMode 한 곳에서 고정하기 위해서다.
	 *
	 * @return 런 생성과 첫 방 준비 요청에 성공하면 true
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool StartNewRun(const FPrimaryAssetId& PlayerUnitId, int32 Difficulty);

	/**
	 * @brief 저장된 활성 Run의 현재 방으로 이어서 입장한다.
	 *
	 * @details
	 * 프론트엔드 타이틀에서는 지도 미리보기를 열지 않는다.
	 * 이미 복구된 RunPersistData의 현재 행/열을 기준으로 방 전환만 요청하고,
	 * 지도 조회와 다음 방 선택은 방 입장 후 RoomGameMode의 WorldMap에서 처리한다.
	 *
	 * @return 현재 방 전환 요청을 시작했으면 true
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool ContinueRunFromTitle();

	/**
	 * @brief 타이틀/세팅 UI에서 기존 런 포기를 요청한다.
	 *
	 * @details
	 * 타이틀 화면은 아직 실제 방 안이 아니므로 방 퇴장 처리나 전투 정리를 하지 않는다.
	 * 활성 Run 데이터만 지우고, 이후 TitleMenuWidget이 메뉴 상태를 다시 읽어 Continue 버튼 표시 여부를 바꾼다.
	 *
	 * @return 런 포기 처리가 가능하면 true
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool AbandonRunFromTitle();

public:
	/**
	 * @brief 캐릭터 선택 UI에 보여줄 카드 View 데이터를 가져온다.
	 *
	 * @details
	 * 원본 데이터는 StaticFrontendRoomSpawnData의 mPlayableUnits에 있다.
	 * 이 함수는 그 DataAsset을 UI 표시용 FFrontendCharacterOption 배열로 바꿔준다.
	 * CharacterSelectWidget이 DataAsset의 내부 필드나 GAS 스탯 계산 위치를 몰라도 되게 만드는 adapter다.
	 *
	 * @param OutOptions 캐릭터 선택 카드에 표시할 View 데이터 배열
	 * @return 표시 가능한 캐릭터 후보가 있으면 true
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool GetCharacterOptions(TArray<FFrontendCharacterOption>& OutOptions) const;

	/**
	 * @brief 타이틀 메뉴 상태와 설정 패널에 사용할 현재 Run 상태를 가져온다.
	 *
	 * @details
	 * TitleMenuWidget과 SettingsPanelWidget이 PersistentData 내부 구조를 직접 알지 않도록,
	 * 활성 Run 여부, 현재 행/열, 레벨, 난이도만 묶어 내려준다.
	 * 같은 DTO를 RoomGameMode에서도 쓰기 때문에 타이틀과 인게임 설정 패널이 같은 표시 규칙을 공유할 수 있다.
	 *
	 * @param OutView 현재 Run 표시용 View 데이터
	 * @return 활성 Run 상태가 있으면 true
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool GetRunControlView(FFrontendRunControlView& OutView) const;

protected:
	/**
	 * @brief StaticFrontendSpawnData에 등록된 플레이어 유닛 PrimaryAsset 목록을 가져온다.
	 *
	 * @details
	 * DA_TestFrontend 같은 프론트엔드 방 DataAsset이 "이 화면에서 선택 가능한 캐릭터 목록"의 소스다.
	 * 처음 조회할 때 AssetManager에서 프론트엔드 방 PrimaryAsset을 가져와 mPlayableUnits를 캐시하고,
	 * 이후 캐릭터 선택 화면 갱신에서는 같은 배열을 재사용한다.
	 *
	 * @note
	 * 이 함수는 PlayerUnit을 새로 로드하지 않는다. Intro/FrontendRoom 프리로드 번들이 PlayerUnit을 함께 올려둔다는 전제를 사용한다.
	 * 로드가 빠진 캐릭터는 GetCharacterOptions()에서 경고 후 건너뛰거나 잠김 카드로 보강한다.
	 *
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
