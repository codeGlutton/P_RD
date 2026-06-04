/*****************************************************************//**
 * @file   FrontendViewTypes.h
 * @brief  프론트엔드 UI 표시용 View 데이터 정의
 * @author Codex
 * @date   2026-06-02
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "PCGStage/RoomType.h"

#include "FrontendViewTypes.generated.h"

class UTexture2D;

/**
 * @brief 프론트엔드 월드맵 노드가 UI에 표시될 때 사용하는 상태
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
 * @brief 캐릭터 선택 화면이 표시할 플레이어 유닛 카드 데이터
 *
 * @details
 * UI는 Primary Data Asset을 직접 순회하지 않고, 프론트엔드 계층이 만든 표시용 데이터만 사용한다.
 * 즉, Widget은 "어떤 캐릭터가 있는지/해금됐는지/초기 스탯이 무엇인지"를 판단하지 않는다.
 * GameMode가 이 구조체로 변환해 내려준 값을 화면에 그린 뒤, 확정 시 mPlayerUnitId만 다시 넘긴다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FFrontendCharacterOption
{
	GENERATED_BODY()

	// 화면 카드의 안정적인 선택 키. 배열 위치가 아니라 GameMode가 정렬 후 부여한 View index다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mIndex = INDEX_NONE;

	// 카드와 상세 패널에 표시할 이름. 비어 있으면 GameMode가 PrimaryAsset 이름으로 fallback한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mName;

	// 직업/역할 표시 문자열. 실제 직업 enum은 UI가 직접 해석하지 않는다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mRole;

	// 카드/상세 패널용 설명 문구.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mDescription;

	// 시작 최대 체력. UI는 표시만 하고 계산하지 않는다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mMaxHP = 0;

	// 시작 주사위 수. UI는 표시만 하고 계산하지 않는다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mDice = 0;

	// 시작 골드. UI는 표시만 하고 계산하지 않는다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mGold = 0;

	// 상세 패널에 크게 보여줄 초상화. SoftObject라 필요할 때 비동기로 로드한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> mPortrait;

	// 카드에 작게 보여줄 아이콘. 없으면 Widget에서 초상화를 fallback으로 사용한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> mIcon;

	// 런 생성 때 GameMode/Subsystem으로 넘길 실제 캐릭터 식별자.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FPrimaryAssetId mPlayerUnitId;

	// 선택 가능 여부. 해금/잠금 판단은 GameMode가 끝내고, UI는 이 값만 신뢰한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool bEnabled = false;
};

/**
 * @brief 월드맵 화면이 표시할 룸 노드 데이터
 *
 * @details
 * Stage/Room 원본 구조를 UI가 직접 해석하지 않도록, 좌표/상태/문구를 합친 View 데이터로 전달한다.
 * Widget은 이 값을 기준으로 노드를 그리고 클릭 이벤트를 GameMode로 다시 전달한다.
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

	// 룸 종류. 시작 지점은 실제 전투 방이 아니므로 None으로 내려간다.
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
	bool bSelectable = false;

	// 현재 사용자가 선택한 방인지 여부.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool bSelected = false;

	// 이미 지나간 방 또는 시작 지점인지 여부.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool bVisited = false;

	// 선택된 방이라 실제 입장 버튼을 눌러도 되는지 여부.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool bCanEnter = false;

	// Stage 시작 위치. 표시 대상이지만 실제 입장 후보는 아니다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool bIsStartPoint = false;
};

/**
 * @brief 세팅/런 컨트롤 화면이 표시할 현재 런 상태
 *
 * @details
 * 세팅 오버레이가 런 데이터 객체를 직접 만지지 않도록 현재 상태와 사용 가능한 명령만 묶어 전달한다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FFrontendRunControlView
{
	GENERATED_BODY()

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool bHasActiveRun = false;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool bCanSaveRun = false;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool bCanAbandonRun = false;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool bIsAtStageStart = false;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mRow = 0;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mColumn = 0;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mPlayerLevel = 0;

	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mDifficulty = 0;
};
