/*****************************************************************//**
 * @file   FrontendViewTypes.h
 * @brief  프론트엔드 UI 표시용 View 데이터 정의
 *
 * @details
 * 이 파일의 타입들은 실제 게임 상태의 원본 데이터가 아니다.
 * FStage, FRoom, URunPersistData, RoomTransitionSubsystem이 가진 값을
 * WBP가 바로 표시할 수 있는 형태로 옮겨 담는 View 전용 DTO를 정의한다.
 *
 * 즉 책임 경계는 다음과 같다.
 * - GameMode/Subsystem: Stage/Room 생성, 현재 룸 위치, 룸 전환, 저장/런 상태 같은 실제 게임 데이터와 규칙을 담당한다.
 * - FrontendViewTypes: 그 결과를 UI가 노드/선/버튼 상태로 그릴 수 있게 변환한 읽기 전용 표시 데이터를 담는다.
 *
 * UI 위젯은 FStage/FRoom을 직접 해석하지 않고, ARoomGameModeBase가 만들어준 이 View 타입만 보고 화면을 그린다.
 * 반대로 이 View 타입도 Stage 생성이나 Room 전환을 직접 수행하지 않는다.
 * @date   2026-06-02
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "PCGStage/RoomType.h"

#include "FrontendViewTypes.generated.h"

/**
 * @brief 프론트엔드 월드맵 노드가 UI에 표시될 때 사용하는 상태
 *
 * 이 enum은 게임 진행 상태의 원본이 아니라, FStage/FRoom과 현재 룸 위치를 해석한 뒤 나온
 * UI 표시 결과다. 실제로 방에 갈 수 있는지, 어느 방으로 전환할지는 RoomGameMode와
 * Run/Room API가 판단한다.
 */
UENUM(BlueprintType)
enum class EFrontendMapRoomState : uint8
{
	// 현재 위치에서 갈 수 없는 방. UI에서는 잠금/비활성 상태로만 보여준다.
	Locked,
	// 현재 위치의 다음 후보 방. 클릭해서 Selected 상태로 바꿀 수 있다.
	Ready,
	// 사용자가 선택한 다음 방. Confirm/Enter 입력이 가능한 상태다.
	Selected,
	// 시작 지점이거나 이미 지난 방. 다시 입장할 수 있는 후보는 아니다.
	Cleared
};

/**
 * @brief 월드맵 화면이 표시할 룸 노드 데이터
 *
 * @details
 * FStage/FRoom, URunPersistData::GetStage(), URoomTransitionSubsystem::PreloadRoomAsync()는
 * 게임 진행에 필요한 원본 데이터와 전환 API를 제공한다. 다만 지도 위젯이 바로 그릴 수 있는
 * 제목, 색상, 선택 가능 여부, 연결선 정보는 따로 정리해야 하므로 이 View DTO를 유지한다.
 *
 * 원본 데이터와 이 구조체의 차이는 다음과 같다.
 * - FStage/FRoom: 실제 Stage 구조, 방 타입, 연결 관계, 현재 Run 데이터의 원본이다.
 * - FFrontendMapRoomView: WBP_FrontendMap이 방 하나를 그릴 때 필요한 제목, 상태, 색상 기준,
 *   클릭 가능 여부, 선 연결 대상 같은 UI 표시용 값이다.
 *
 * 예를 들어 FRoom::mType과 FRoom::mNextRoomColumns는 Stage 데이터에서 오지만,
 * "이 노드를 Ready 색으로 보여줄지", "ENTER 버튼을 켜도 되는지", "시작 지점인지" 같은 값은
 * ARoomGameModeBase::GetMapRoomViews()가 현재 Run 상태를 보고 계산해서 이 구조체에 담는다.
 *
 * Widget은 이 값을 보고 노드/선을 배치하고, 클릭 시 mRow/mColumn만 GameMode에 돌려준다.
 * 방 잠금/선택 가능 여부 같은 규칙 판단도 Widget이 직접 하지 않고, GameMode가 이 값으로 내려준다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FFrontendMapRoomView
{
	GENERATED_BODY()

	// Stage 내부 행 좌표. 룸 선택/입장 요청 때 다시 GameMode로 넘긴다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mRow = INDEX_NONE;

	// Stage 내부 열 좌표. 룸 선택/입장 요청 때 다시 GameMode로 넘긴다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mColumn = INDEX_NONE;

	// 룸 종류. 나중에 UStaticStageSpawnData::mRoomIcons 같은 공식 데이터로 아이콘을 고를 때 사용한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	ERoomType mType = ERoomType::None;

	// 이 노드가 잠김/선택 가능/선택됨/방문 완료 중 어디에 해당하는지 나타낸다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	EFrontendMapRoomState mState = EFrontendMapRoomState::Locked;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mTitle;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mDescription;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	TArray<int32> mNextRoomColumns;

	// 같은 행/열 노드가 겹치지 않도록 Stage 생성기가 부여한 화면 위치 보정값.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FVector2D mPositionOffsetRate = FVector2D::ZeroVector;

	// 클릭 가능한 다음 방인지 여부. Widget은 이 값으로 버튼 활성화만 결정한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool mSelectable = false;

	// 현재 사용자가 선택한 방인지 여부.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool mSelected = false;

	// 이미 지나간 방 또는 시작 지점인지 여부.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool mVisited = false;

	// 선택된 방이라 실제 입장 버튼을 눌러도 되는지 여부.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool mCanEnter = false;

	// Stage 시작 위치. 표시 대상이지만 실제 입장 후보는 아니다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool mIsStartPoint = false;
};

/**
 * @brief 세팅/런 컨트롤 화면이 표시할 현재 런 상태
 *
 * @details
 * URunPersistData는 현재 런/스테이지/플레이어 값을 제공하지만,
 * 타이틀, 설정, 월드맵 UI가 그 객체를 직접 만지면 저장/포기/지도 표시 가능 여부 판단이 위젯으로 새기 쉽다.
 * 이 구조체는 그 경계를 막기 위한 View DTO다.
 *
 * 예를 들어 "현재 Run이 있는지", "저장 버튼을 켤 수 있는지", "지도 처음 표시 시 시작 지점으로
 * 스크롤해야 하는지"는 UI에 필요한 값이지만, UI가 URunPersistData를 직접 수정하면서 판단하면
 * 게임 상태 변경 책임이 화면 코드로 넘어간다. 그래서 GameMode가 실제 상태를 읽고,
 * 위젯에는 이 구조체로 필요한 표시 상태만 전달한다.
 *
 * Widget은 이 값을 보고 버튼 활성화, 시작 위치 스크롤 같은 표시만 결정한다.
 * 실제 저장/포기/전환 명령은 FrontendGameMode 또는 RoomGameModeBase/Subsystem API를 통해 수행한다.
 * Run 저장은 방 진입 시 RoomGameModeBase에서 자동 처리하므로,
 * 타이틀과 월드맵 화면에서는 Run 저장 버튼을 활성화하지 않는다. 옵션 저장처럼 UserPersistData에 속하는
 * 설정 저장 API가 생기면 그때 별도 View 값으로 분리한다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FFrontendRunControlView
{
	GENERATED_BODY()

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool mHasActiveRun = false;

	// 타이틀과 월드맵에서는 Run 저장을 제공하지 않으므로 현재 false로 유지한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool mCanSaveRun = false;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool mCanAbandonRun = false;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool mIsAtStageStart = false;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mRow = 0;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mColumn = 0;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mPlayerLevel = 0;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mDifficulty = 0;
};
