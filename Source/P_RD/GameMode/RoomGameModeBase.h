/*****************************************************************//**
 * @file   RoomGameModeBase.h
 * @brief  방에 대한 베이스 GameMode 정의 헤더
 * @author 모호재, 박용수
 * @date   2026-05-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameMode/RDGameModeBase.h"
#include "UI/RoomViewTypes.h"
#include "RoomGameModeBase.generated.h"

 // Room Game Mode 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogRoomGameMode, Log, All)

class UPartyModel;
class UPlayerUnitModel;
struct FCombatArtifactUI;
struct FSkillDetailUI;

DECLARE_DELEGATE_OneParam(FOnRoomSaveAndExitComplete, bool /*bSuccess*/);

/**
 * @brief  방에 대한 베이스 GameMode
 */
UCLASS(abstract)
class P_RD_API ARoomGameModeBase : public ARDGameModeBase
{
	GENERATED_BODY()

public:
	ARoomGameModeBase();
	
	/* ARDGameModeBase 상속 */
public:
	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

protected:
	void InitializeCommonRoom() override;
	void BeginRoom() override;

	/* UI 진입점 */
public:
	/**
	 * @brief 파티 용병의 스킬 상세 DTO를 조립한다.
	 *
	 * @details
	 * 지도/상점의 용병 패널이 전투와 같은 상세창을 쓰기 위한 생산 API다.
	 * 위젯은 모델을 직접 읽지 않고 이 값을 받아 그린다 (PR #426 규칙).
	 * @param MemberIndex 파티 목록 index
	 * @param SkillIndex 유닛별 스킬 슬롯 index
	 * @param[out] OutDetail 채워 줄 상세 DTO
	 * @return 스킬이 있어 채웠으면 참
	 */
	bool BuildPartyUnitSkillDetailUI(int32 MemberIndex, int32 SkillIndex,
		FSkillDetailUI& OutDetail) const;

	/**
	 * @brief 파티 공용 아티팩트의 상세 DTO를 조립한다.
	 *
	 * @details
	 * 지도/상점 레일의 인벤토리 페이지가 전투와 같은 아티팩트 상세창을 쓰기
	 * 위한 생산 API다. 위젯은 모델을 직접 읽지 않는다 (PR #426 규칙).
	 * @param ArtifactIndex 파티 아티팩트 목록 index
	 * @param[out] OutDetail 채워 줄 상세 DTO
	 * @return 아티팩트가 있어 채웠으면 참
	 */
	bool BuildPartyArtifactDetailUI(int32 ArtifactIndex,
		FCombatArtifactUI& OutDetail) const;

	/**
	 * @brief 지도에서 방 노드 선택을 요청한다.
	 * @param RoomRow 선택하려는 룸 행 index
	 * @param RoomColumn 선택하려는 룸 열 index
	 * @return 선택 가능한 룸이면 true
	 */
	UFUNCTION(Category = Room, BlueprintCallable)
	bool SelectNextRoom(int32 RoomRow, int32 RoomColumn);

	/**
	 * @brief 선택된 방으로 실제 입장을 요청한다.
	 * @return 입장 요청을 시작했으면 true
	 */
	UFUNCTION(Category = Room, BlueprintCallable)
	bool EnterSelectedRoom();

	/**
	 * @brief 런을 종료하고 Frontend로 돌아간다.
	 * @return 전환 시작했으면 true
	 */
	UFUNCTION(Category = Room, BlueprintCallable)
	bool AbandonRunFromRoom();

	/** @brief 현재 런 저장 성공 뒤에만 Frontend 전환을 시작한다. */
	void SaveAndExitRunFromRoomAsync(FOnRoomSaveAndExitComplete Completion);

public:
	/**
	 * @brief 지도 UI가 그릴 방 노드 View 데이터를 가져온다.
	 * @param OutRooms 지도 노드/선 배치에 사용할 View 데이터 배열
	 * @return 표시 가능한 Stage 룸 데이터가 있으면 true
	 */
	UFUNCTION(Category = Room, BlueprintCallable)
	bool GetMapRoomViews(TArray<FMapRoomView>& OutRooms) const;

	/**
	 * @brief 지도 첫 표시 위치 계산용 런 상태 View를 가져온다.
	 * @param OutView WorldMap이 표시만 할 Run 상태 View
	 * @return 활성 런 상태를 만들 수 있으면 true
	 */
	UFUNCTION(Category = Room, BlueprintCallable)
	bool GetRunControlView(FRunControlView& OutView) const;

	/**
	 * @brief 용병 패널이 그릴 파티 명단(직업명/스탯/초상/장착 스킬)을 조회한다.
	 *
	 * @details
	 * 인벤토리와 같은 규칙이다 -- 밀지 않고 물어보게 둔다. 패널이 열려 있을
	 * 때만 보면 되므로, 매번 밀어 두면 안 볼 값을 계속 만드는 셈이 된다.
	 * 직업 표시명과 초상 해석까지 여기서 끝낸다 (PR #426 규칙 -- 위젯은
	 * 모델/AttributeSet/DA 를 직접 읽지 않는다).
	 * 파티 공용 골드/아티팩트도 함께 싣는다 -- 레일 인벤토리 페이지가
	 * 별도 조회 없이 이 한 판으로 그린다.
	 * @param[out] OutView 채워 줄 명단
	 * @return 채웠으면 참
	 */
	UFUNCTION(Category = UI, BlueprintPure)
	bool GetPartyRosterView(FPartyRosterView& OutView) const;

	/**
	 * @brief 세팅 화면용 현재 런 상태를 조회한다.
	 * @param RowIndex 현재 룸 행 index
	 * @param ColumnIndex 현재 룸 열 index
	 * @param PlayerLevel 현재 플레이어 레벨
	 * @param Difficulty 현재 난이도
	 * @return 활성 런 상태가 있으면 true
	 */
	bool GetRunControlState(OUT int32& RowIndex, OUT int32& ColumnIndex, OUT int32& PlayerLevel, OUT int32& Difficulty) const;

	/*
	 * @brief 현재 전투 방의 표시 이름('일반'/'엘리트'/'보스'). HUD 룸 이름 라벨용. 
	 */
	FText GetCurrentRoomDisplayName() const;

protected:
	bool PreloadAndTransitionSelectedRoomAsync();
	void SaveRunWithUIAsync() const;

private:
	void RestorePlayerUnit();

private:
	void ClearSelectedRoom();
	bool HasSelectedRoom() const;
	bool IsRoomSelectable(int32 RoomRow, int32 RoomColumn) const;

public:
	UPartyModel* GetPartyModel() const;
	UPlayerUnitModel* GetPlayerUnitModel(int32 PlayerIndex) const;
	TArray<TObjectPtr<UPlayerUnitModel>>& GetPlayerUnitModels() const;

public:
	const FName& GetRoomSpawnSettingName() const;

protected:
	void SetRoomSpawnSettingName(const FName& Name);

protected:
	UPROPERTY()
	TWeakObjectPtr<UPartyModel> mPartyModel;

protected:
	int32 mSelectedRoomRow = INDEX_NONE;
	int32 mSelectedRoomColumn = INDEX_NONE;

private:
	FName mSelectedRoomSpawnSettingName = NAME_None;
	bool mSaveAndExitPending = false;
};
