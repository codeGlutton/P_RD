/*****************************************************************//**
 * @file   RoomViewTypes.h
 * @brief  UI 표시용 View 데이터 정의
 *
 * @details
 * 이 파일의 타입들은 실제 게임 상태의 원본 데이터가 아니다.
 * FStage, FRoom, URunPersistData, RoomTransitionSubsystem이 가진 값을
 * WBP가 바로 표시할 수 있는 형태로 옮겨 담는 View 전용 DTO를 정의한다.
 *
 * 즉 책임 경계는 다음과 같다.
 * - GameMode/Subsystem: Stage/Room 생성, 현재 룸 위치, 룸 전환, 저장/런 상태 같은 실제 게임 데이터와 규칙을 담당한다.
 * - View DTO: 그 결과를 UI가 노드/선/버튼 상태로 그릴 수 있게 변환한 읽기 전용 표시 데이터를 담는다.
 *
 * UI 위젯은 FStage/FRoom을 직접 해석하지 않고, GameMode가 만들어준 이 View 타입만 보고 화면을 그린다.
 * 반대로 이 View 타입도 Stage 생성이나 Room 전환을 직접 수행하지 않는다.
 * @date   2026-06-02
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "PCGStage/RoomType.h"

#include "RoomViewTypes.generated.h"

/**
 * @brief 월드맵 노드가 UI에 표시될 때 사용하는 상태
 *
 * 이 enum은 게임 진행 상태의 원본이 아니라, FStage/FRoom과 현재 룸 위치를 해석한 뒤 나온
 * UI 표시 결과다. 실제로 방에 갈 수 있는지, 어느 방으로 전환할지는 GameMode와
 * Run/Room API가 판단한다.
 */
UENUM(BlueprintType)
enum class EMapRoomState : uint8
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
 * - FMapRoomView: WBP_FrontendMap이 방 하나를 그릴 때 필요한 제목, 상태, 색상 기준,
 *   클릭 가능 여부, 선 연결 대상 같은 UI 표시용 값이다.
 *
 * 예를 들어 FRoom::mType과 FRoom::mNextRoomColumns는 Stage 데이터에서 오지만,
 * "이 노드를 Ready 색으로 보여줄지", "ENTER 버튼을 켜도 되는지", "시작 지점인지" 같은 값은
 * GameMode::GetMapRoomViews()가 현재 Run 상태를 보고 계산해서 이 구조체에 담는다.
 *
 * Widget은 이 값을 보고 노드/선을 배치하고, 클릭 시 mRow/mColumn만 GameMode에 돌려준다.
 * 방 잠금/선택 가능 여부 같은 규칙 판단도 Widget이 직접 하지 않고, GameMode가 이 값으로 내려준다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FMapRoomView
{
	GENERATED_BODY()

	// Stage 내부 행 좌표. 룸 선택/입장 요청 때 다시 GameMode로 넘긴다.
	UPROPERTY(Category = UI, BlueprintReadOnly)
	int32 mRow = INDEX_NONE;

	// Stage 내부 열 좌표. 룸 선택/입장 요청 때 다시 GameMode로 넘긴다.
	UPROPERTY(Category = UI, BlueprintReadOnly)
	int32 mColumn = INDEX_NONE;

	// 룸 종류. 나중에 UStaticStageSpawnData::mRoomIcons 같은 공식 데이터로 아이콘을 고를 때 사용한다.
	UPROPERTY(Category = UI, BlueprintReadOnly)
	ERoomType mType = ERoomType::None;

	// 이 노드가 잠김/선택 가능/선택됨/방문 완료 중 어디에 해당하는지 나타낸다.
	UPROPERTY(Category = UI, BlueprintReadOnly)
	EMapRoomState mState = EMapRoomState::Locked;

	UPROPERTY(Category = UI, BlueprintReadOnly)
	FText mTitle;

	UPROPERTY(Category = UI, BlueprintReadOnly)
	FText mDescription;

	UPROPERTY(Category = UI, BlueprintReadOnly)
	TArray<int32> mNextRoomColumns;

	// 같은 행/열 노드가 겹치지 않도록 Stage 생성기가 부여한 화면 위치 보정값.
	UPROPERTY(Category = UI, BlueprintReadOnly)
	FVector2D mPositionOffsetRate = FVector2D::ZeroVector;

	// 클릭 가능한 다음 방인지 여부. Widget은 이 값으로 버튼 활성화만 결정한다.
	UPROPERTY(Category = UI, BlueprintReadOnly)
	bool mSelectable = false;

	// 현재 사용자가 선택한 방인지 여부.
	UPROPERTY(Category = UI, BlueprintReadOnly)
	bool mSelected = false;

	// 이미 지나간 방 또는 시작 지점인지 여부.
	UPROPERTY(Category = UI, BlueprintReadOnly)
	bool mVisited = false;

	// 선택된 방이라 실제 입장 버튼을 눌러도 되는지 여부.
	UPROPERTY(Category = UI, BlueprintReadOnly)
	bool mCanEnter = false;

	// Stage 시작 위치. 표시 대상이지만 실제 입장 후보는 아니다.
	UPROPERTY(Category = UI, BlueprintReadOnly)
	bool mIsStartPoint = false;
};

/**
 * @brief 세팅/런 컨트롤 화면이 표시할 현재 런 상태
 *
 * @details
 * URunPersistData는 현재 런/스테이지/플레이어 값을 제공하지만,
 * 월드맵 UI가 그 객체를 직접 만지면 저장/포기/지도 표시 가능 여부 판단이 위젯으로 새기 쉽다.
 * 이 구조체는 그 경계를 막기 위한 View DTO다.
 *
 * 예를 들어 "현재 Run이 있는지", "저장 버튼을 켤 수 있는지", "지도 처음 표시 시 시작 지점으로
 * 스크롤해야 하는지"는 UI에 필요한 값이지만, UI가 URunPersistData를 직접 수정하면서 판단하면
 * 게임 상태 변경 책임이 화면 코드로 넘어간다. 그래서 GameMode가 실제 상태를 읽고,
 * 위젯에는 이 구조체로 필요한 표시 상태만 전달한다.
 *
 * Widget은 이 값을 보고 버튼 활성화, 시작 위치 스크롤 같은 표시만 결정한다.
 * 실제 저장/포기/전환 명령은 GameMode 또는 Subsystem API를 통해 수행한다.
 * Run 저장은 방 진입 시 RoomGameModeBase에서 자동 처리하므로,
 * 월드맵 화면에서는 Run 저장 버튼을 활성화하지 않는다. 옵션 저장처럼 UserPersistData에 속하는
 * 설정 저장 API가 생기면 그때 별도 View 값으로 분리한다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FRunControlView
{
	GENERATED_BODY()

	UPROPERTY(Category = UI, BlueprintReadOnly)
	bool mHasActiveRun = false;

	// 월드맵에서는 Run 저장을 제공하지 않으므로 현재 false로 유지한다.
	UPROPERTY(Category = UI, BlueprintReadOnly)
	bool mCanSaveRun = false;

	UPROPERTY(Category = UI, BlueprintReadOnly)
	bool mCanAbandonRun = false;

	UPROPERTY(Category = UI, BlueprintReadOnly)
	bool mIsAtStageStart = false;

	UPROPERTY(Category = UI, BlueprintReadOnly)
	int32 mRow = 0;

	UPROPERTY(Category = UI, BlueprintReadOnly)
	int32 mColumn = 0;

	UPROPERTY(Category = UI, BlueprintReadOnly)
	int32 mPlayerLevel = 0;

	UPROPERTY(Category = UI, BlueprintReadOnly)
	int32 mDifficulty = 0;
};

/**
 * @brief 용병 패널에 늘어놓는 장착 스킬 한 칸.
 *
 * @details
 * 화면은 스킬 DA 를 모른다. 그릴 것(이름/아이콘/비용)과 상세를 청할 때
 * 되돌려 보낼 슬롯 번호만 안다 -- 인벤토리(FInventoryArtifactView)와 같은 규칙.
 */
USTRUCT(BlueprintType)
struct P_RD_API FPartyRosterSkillView
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;

	/** @brief 스킬 컴포넌트 원본 슬롯 index. 상세 요청(BuildPartyUnitSkillDetailUI) payload다. */
	UPROPERTY(BlueprintReadOnly) int32 mSlotIndex = INDEX_NONE;

	/** @brief 소모 AP. 유닛 스킬이 아니라 비용 개념이 없으면 INDEX_NONE(표시 생략). */
	UPROPERTY(BlueprintReadOnly) int32 mActionPointCost = INDEX_NONE;
};

/**
 * @brief 용병 패널의 파티원 한 줄(초상·직업명·레벨·스탯·장착 스킬).
 *
 * @details
 * 직업 표시명과 초상 해석까지 생산자(GameMode)가 끝낸다. 화면이 직업 enum 을
 * 직접 해석하면 표시 규칙이 위젯마다 갈라진다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FPartyRosterMemberView
{
	GENERATED_BODY()

public:
	/** @brief 파티 목록 원본 index. 상세 요청(BuildPartyUnitSkillDetailUI)의 MemberIndex payload다. */
	UPROPERTY(BlueprintReadOnly) int32 mMemberIndex = INDEX_NONE;

	/** @brief 직업 표시명(기사/마법사…). 로컬라이징 규칙 변경도 생산자만 고친다. */
	UPROPERTY(BlueprintReadOnly) FText mJobName;
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 1;
	UPROPERTY(BlueprintReadOnly) int32 mHP = 0;
	UPROPERTY(BlueprintReadOnly) int32 mMaxHP = 0;
	UPROPERTY(BlueprintReadOnly) int32 mAP = 0;
	UPROPERTY(BlueprintReadOnly) int32 mMaxAP = 0;
	UPROPERTY(BlueprintReadOnly) int32 mSpeed = 0;

	/** @brief 목록/상세 초상. 직업 아이콘 → 보드 아이콘 → 초상화 순 해석 결과. */
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mPortrait = nullptr;

	/** @brief 장착 스킬(빈 슬롯 제외). 상세 왕복은 각 칸의 mSlotIndex 로 한다. */
	UPROPERTY(BlueprintReadOnly) TArray<FPartyRosterSkillView> mSkills;
};

/**
 * @brief 레일 인벤토리 페이지에 늘어놓는 파티 공용 아티팩트 한 줄.
 *
 * @details
 * 골드/아티팩트는 용병 개인이 아니라 파티 공용 소유다. 레일의 용병 패널이
 * 인벤토리 페이지까지 함께 그리므로 명단 View(FPartyRosterView)에 실어 내린다.
 * 화면은 자산도 모델도 모른다 -- 그릴 것과 상세 왕복용 index 만 안다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FPartyRosterArtifactView
{
	GENERATED_BODY()

public:
	/** @brief 눌렀을 때 되돌려 보낼 번호. 파티 아티팩트 목록 안 자리다. */
	UPROPERTY(BlueprintReadOnly) int32 mArtifactIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;

	/** @brief 공용 효과 같은 보조 한 줄. 없으면 빈 값. */
	UPROPERTY(BlueprintReadOnly) FText mDetail;
};

/**
 * @brief 용병 패널 한 판(파티 전체).
 *
 * @details
 * 지도/상점의 우상단 레일이 여는 용병 패널용. 인벤토리와 같은 규칙이다 --
 * 밀지 않고 물어보게 둔다(GetPartyRosterView). 패널이 열려 있을 때만 보면 된다.
 * 골드/아티팩트는 파티 공용 소유라 명단과 한 판으로 내린다 -- 레일의
 * 인벤토리 페이지가 이 값만 보고 그린다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FPartyRosterView
{
	GENERATED_BODY()

public:
	/** @brief 파티원(빈 파티 슬롯 제외). 상세 왕복은 각 줄의 mMemberIndex 로 한다. */
	UPROPERTY(BlueprintReadOnly) TArray<FPartyRosterMemberView> mMembers;

	/** @brief 파티 공용 골드. */
	UPROPERTY(BlueprintReadOnly) int32 mGold = 0;

	/** @brief 파티가 공용으로 소유 중인 아티팩트. 레일 인벤토리 페이지 전용. */
	UPROPERTY(BlueprintReadOnly) TArray<FPartyRosterArtifactView> mArtifacts;
};
