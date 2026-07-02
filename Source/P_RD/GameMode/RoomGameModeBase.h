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

class UPlayerUnitModel;

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
	AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
	void InitializeCommonRoom() override;
	void BeginRoom() override;

	/* UI 진입점 */
public:
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
	 * @brief 세팅 화면용 현재 런 상태를 조회한다.
	 * @param RowIndex 현재 룸 행 index
	 * @param ColumnIndex 현재 룸 열 index
	 * @param PlayerLevel 현재 플레이어 레벨
	 * @param Difficulty 현재 난이도
	 * @return 활성 런 상태가 있으면 true
	 */
	bool GetRunControlState(OUT int32& RowIndex, OUT int32& ColumnIndex, OUT int32& PlayerLevel, OUT int32& Difficulty) const;

protected:
	bool PreloadAndTransitionSelectedRoomAsync();

private:
	void SaveRunWithUIAsync() const;
	void RestorePlayerUnit();

private:
	void ClearSelectedRoom();
	bool HasSelectedRoom() const;
	bool IsRoomSelectable(int32 RoomRow, int32 RoomColumn) const;

public:
	UPlayerUnitModel* GetPlayerUnitModel() const;

protected:
	UPROPERTY()
	TWeakObjectPtr<UPlayerUnitModel> mPlayerUnit;

protected:
	int32 mSelectedRoomRow = INDEX_NONE;
	int32 mSelectedRoomColumn = INDEX_NONE;
};
