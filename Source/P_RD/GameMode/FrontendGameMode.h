/*****************************************************************//**
 * @file   FrontendGameMode.h
 * @brief  프론트엔드 GameMode 정의 헤더
 * @author Codex
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "Frontend/CharacterSelectTypes.h"
#include "Frontend/FrontendViewTypes.h"
#include "GameMode/RDGameModeBase.h"
#include "Kismet/GameplayStatics.h"

#include "FrontendGameMode.generated.h"

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
UCLASS()
class P_RD_API AFrontendGameMode : public ARDGameModeBase
{
	GENERATED_BODY()

public:
	AFrontendGameMode();

	/**
	 * @brief 타이틀 START 입력을 처리해 캐릭터 선택 화면을 열도록 요청한다.
	 * @return 캐릭터 선택 화면 전환 요청에 성공하면 true
	 *
	 * @note UI 파트 추가 API
	 * PM 브랜치에 있던 공식 Room/Run API가 아니라, WBP_TitleMenu가 C++ 내부 화면 전환 함수를
	 * 직접 알지 않도록 둔 프론트엔드용 얇은 진입점이다.
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool StartNewRunFromTitle();

	/**
	 * @brief 선택한 플레이어 유닛으로 새 런 준비를 요청한다.
	 * @param PlayerUnitId 선택한 플레이어 유닛 PrimaryAssetId
	 * @return 런/지도 준비 요청에 성공하면 true
	 *
	 * @note UI 파트 추가 API
	 * 현재는 PrepareRunMapWithPlayerUnit()으로 위임하는 호환용 wrapper다. 실제 새 Stage 생성과 첫 방
	 * preload/transition은 PM 브랜치의 RoomTransitionSubsystem::MakeStageAndPreloadRoomAsync()를 사용한다.
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool StartRunWithPlayerUnit(FPrimaryAssetId PlayerUnitId);

	/**
	 * @brief AssetManager에 등록된 플레이어 유닛 PrimaryAssetId 목록을 가져온다.
	 * @param OutPlayerUnitIds 조회된 플레이어 유닛 PrimaryAssetId 배열
	 * @return 하나 이상의 플레이어 유닛을 찾으면 true
	 *
	 * @note UI 파트 추가 adapter
	 * PM 브랜치의 PlayerUnit DataAsset/AssetManager 등록 정보를 UI 카드 목록으로 바꾸기 위한 보조 API다.
	 */
	bool GetPlayerUnitIds(TArray<FPrimaryAssetId>& OutPlayerUnitIds) const;

	/**
	 * @brief 캐릭터 선택 UI에 보여줄 카드 View 데이터를 가져온다.
	 * @param OutOptions 캐릭터 선택 카드에 표시할 View 데이터 배열
	 * @return 표시 가능한 캐릭터 후보가 있으면 true
	 *
	 * @note PM 타입 + UI adapter
	 * 반환 타입 FFrontendCharacterOption은 PM 브랜치에 있던 View 타입을 확장한 것이다.
	 * 이 조회 함수 자체는 UI 파트가 추가했으며, 내부 값은 PM 브랜치의 UStaticPlayerUnitSpawnData에서 만든다.
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool GetCharacterOptions(TArray<FFrontendCharacterOption>& OutOptions) const;

	/**
	 * @brief 선택한 플레이어 유닛으로 런을 만들고 첫 방 입장을 준비한다.
	 * @param PlayerUnitId 선택한 플레이어 유닛 PrimaryAssetId
	 * @return 첫 방 입장 준비 요청에 성공하면 true
	 *
	 * @note UI 파트 추가 API
	 * PM 구조에서는 새 Stage의 StartRoom이 첫 방이며, RoomTransitionSubsystem::MakeStageAndPreloadRoomAsync()
	 * 가 Stage 생성 후 StartRoom preload까지 처리한다. 따라서 캐릭터 선택 직후 지도 preview를 먼저 열지 않고,
	 * 첫 방 진입을 PM 전환 흐름에 맡긴다.
	 *
	 * 현재 난이도와 StageLevel은 타이틀 -> 캐릭터 선택 -> 첫 방 입장 연결 검증을 위한 임시 기본값을 사용한다.
	 * 최종적으로 난이도 선택, 프로필 선택, 스테이지 선택 정책이 확정되면 이 함수는 그 값을 받아 PM Run/Stage
	 * API에 전달하는 adapter 역할만 유지해야 한다.
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool PrepareRunMapWithPlayerUnit(FPrimaryAssetId PlayerUnitId);

	/**
	 * @brief 지도 UI가 그릴 방 노드 View 데이터를 가져온다.
	 * @param OutRooms 지도 노드/선 배치에 사용할 View 데이터 배열
	 * @return 표시 가능한 Stage 룸 데이터가 있으면 true
	 *
	 * @note UI 파트 추가 adapter
	 * PM 브랜치의 FStage/FRoom/URunPersistData::GetStage()를 UI가 바로 그릴 수 있는
	 * FFrontendMapRoomView 스냅샷으로 변환한다. 원본 지도 데이터 생성 책임은 PM 쪽에 있다.
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool GetMapRoomViews(TArray<FFrontendMapRoomView>& OutRooms) const;

	/**
	 * @brief 현재 런이 살아 있는지 확인한다.
	 * @return 활성 RunPersistData가 있으면 true
	 *
	 * @note UI 파트 추가 조회 API
	 * PM 브랜치의 URunPersistData 상태를 타이틀/세팅 UI가 직접 만지지 않도록 감싼다.
	 */
	UFUNCTION(Category = Title, BlueprintPure)
	bool HasActiveRun() const;

	/**
	 * @brief 타이틀/세팅 UI에서 지금 런 포기 버튼을 사용할 수 있는지 조회한다.
	 * @return 런 포기 버튼을 활성화할 수 있으면 true
	 *
	 * @note UI 파트 추가 조회 API
	 */
	UFUNCTION(Category = Title, BlueprintPure)
	bool CanAbandonRun() const;

	/**
	 * @brief 지도 첫 표시 위치 계산용 런 상태 View를 가져온다.
	 * @param OutView 타이틀/지도 UI가 표시만 할 Run 상태 View
	 * @return 활성 런 상태를 만들 수 있으면 true
	 *
	 * @note UI 파트 추가 DTO adapter
	 * PM 브랜치의 URunPersistData 값을 FFrontendRunControlView로 옮겨 담아 UI가 원본 런 데이터를 직접
	 * 변경하지 않게 한다.
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool GetRunControlView(FFrontendRunControlView& OutView) const;

	/**
	 * @brief 세팅 화면용 현재 런 상태를 조회한다.
	 * @param RowIndex 현재 룸 행 index
	 * @param ColumnIndex 현재 룸 열 index
	 * @param PlayerLevel 현재 플레이어 레벨
	 * @param Difficulty 현재 난이도
	 * @return 활성 런 상태가 있으면 true
	 *
	 * @note UI 파트 추가 호환 API
	 * GetRunControlView()를 기존 세팅/디버그 호출 형태에 맞춰 풀어주는 wrapper다.
	 */
	bool GetRunControlState(OUT int32& RowIndex, OUT int32& ColumnIndex, OUT int32& PlayerLevel, OUT int32& Difficulty) const;

	/**
	 * @brief 타이틀/세팅 UI에서 현재 런 포기를 요청한다.
	 * @return 런 포기 처리가 가능하면 true
	 *
	 * @note UI 파트 추가 command API
	 * 실제 런 종료/세이브 삭제는 PM 브랜치의 GameProfileSubsystem/SaveGameSubsystem 데이터를 사용한다.
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool AbandonRunFromTitle();

	/**
	 * @brief 지도에서 방 노드 선택을 요청한다.
	 * @param RowIndex 선택하려는 룸 행 index
	 * @param ColumnIndex 선택하려는 룸 열 index
	 * @return 선택 가능한 룸이면 true
	 *
	 * @note UI 파트 추가 command API
	 * PM 브랜치에는 지도 노드 클릭용 View API가 없어서 추가했다. 선택 가능 여부는 PM Stage 데이터의
	 * FRoom::mNextRoomColumns를 읽어 판단하되, 현재 방 위치는 바꾸지 않는다.
	 *
	 * 공식 Run 진행 API가 별도로 생기면 이 함수의 연결 검증은 그 API로 위임해야 한다. 지금은
	 * Stage 생성기가 만든 "현재 룸 -> 다음 룸 column" 연결을 사용하는 최소 adapter다.
	 *
	 * 선택이 성공하면 RoomTransitionSubsystem::PreloadRoomAsync()만 요청한다.
	 * 실제 전환은 EnterSelectedMapRoom()에서 TransitLoadedRoomAsync()로 분리해 호출한다.
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool SelectMapRoom(int32 RowIndex, int32 ColumnIndex);

	/**
	 * @brief 선택된 방으로 실제 입장을 요청한다.
	 * @return 입장 요청을 시작했으면 true
	 *
	 * @note UI wrapper + PM 전환 API 사용
	 * 함수 이름은 UI 파트가 지도 ENTER 버튼용으로 추가한 임시 facade다. 방 preload는 SelectMapRoom()에서
	 * 먼저 요청하고, 이 함수는 PM 브랜치의 URoomTransitionSubsystem::TransitLoadedRoomAsync()만 호출한다.
	 *
	 * 선택 좌표는 FrontendGameMode가 임시로 보관하지만, 실제 현재 룸 변경은 PM 브랜치의
	 * RoomTransitionSubsystem::OnTransitNextRoom()에서 수행한다. UI wrapper가 RunPersistData의 현재 룸을
	 * 미리 바꾸지 않는 이유도 이 경계를 지키기 위해서다.
	 */
	UFUNCTION(Category = Title, BlueprintCallable)
	bool EnterSelectedMapRoom();

protected:
	void BeginRoom() override;

private:
	/**
	 * @brief 타이틀 진입 시 저장된 유저를 로드하고, 아직 프로필 화면이 없으면 임시 기본 유저를 만든다.
	 *
	 * @note 임시 프론트엔드 부트스트랩
	 * Run 시작 함수에서 UserData를 만들지 않기 위한 분리 지점이다. 최종 프로필/계정 생성 화면이 들어오면
	 * 이 fallback은 그 흐름으로 대체되어야 한다.
	 */
	void LoadOrCreateFrontendUserProfile();
	void StartIntroCinematicOrShowTitle();
	void ShowTitleHUD();
	bool OpenTitleCharacterSelect();
	void ClearSelectedMapRoom();
	bool HasSelectedMapRoom() const;
	bool PreloadSelectedMapRoom();
	/**
	 * @brief 선택 캐릭터로 새 Run을 만들고 PM 전환 API로 첫 방 입장을 시작한다.
	 * @param PlayerUnitId 선택한 플레이어 유닛 PrimaryAssetId
	 * @return 첫 방 preload/transition 요청을 시작했으면 true
	 *
	 * @note UI 파트 추가 내부 함수
	 * PM 브랜치의 URoomTransitionSubsystem::MakeStageAndPreloadRoomAsync()는 Stage 생성 뒤 StartRoom을
	 * preload하는 API다. StartRoom이 첫 방이라는 PM 구조를 따르기 위해 캐릭터 선택 직후 이 API를 사용한다.
	 *
	 * 유저 생성은 게임 최초 프로필 생성 단계의 책임이므로 여기서 GameProfileSubsystem::MakeUser()를
	 * 호출하지 않는다. 이 함수는 이미 활성화된 UserPersistData가 있다는 전제에서 새 RunPersistData만 만든다.
	 *
	 * DefaultDifficulty, EStageLevelType::Stage1은 현재 UI 흐름 테스트를 위한 임시 기본값이다. 이 함수가
	 * 게임 규칙을 소유한다는 뜻이 아니며, 관련 선택 API가 생기면 그 결과를 PM API에 넘기는 쪽으로 줄여야 한다.
	 */
	bool StartRunAndEnterFirstRoomWithPlayerUnit(FPrimaryAssetId PlayerUnitId);
	void ShowTitleMessage(const FText& Message) const;

	UFUNCTION()
	void HandleIntroCinematicFinished();

private:
	bool bStartRunRequested = false;
	bool bTitleHUDShown = false;
	int32 mSelectedMapRoomRow = INDEX_NONE;
	int32 mSelectedMapRoomColumn = INDEX_NONE;
	bool bSelectedMapRoomPreloadRequested = false;
};
