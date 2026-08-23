#pragma once

/** @brief 전투 UI와 게임플레이 사이에서 주고받는 표시용 뷰 데이터입니다. */
// UI는 게임플레이 객체(UUnitData 등)를 직접 알지 않고 이 struct들만 읽는다.
// FCombatQueueNode는 스킬/행동 결과를 애니메이션 한 단위씩 전달하기 위한 큐 노드다.

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"   // FTileIndex
#include "GameplayTagContainer.h"

#include "CombatUITypes.generated.h"

class UTexture2D;
class AActor;   // FUnitUI.mViewActor(HP바 라이브 투영, 약참조)

/** @brief 무엇이 바뀌어 UI를 갱신해야 하는지 도메인 구분(부분 갱신용). */
// UI 필요값: Set*() 호출 뒤 어떤 패널만 다시 그릴지 알려주는 최소 단위다.
// 예: SetUnitUIs -> Unit, SetActionQueue -> Queue.
UENUM(BlueprintType)
enum class ECombatUIDomain : uint8
{
	All,
	Skill,
	Unit,
	Turn,
	Queue,
	Equipment,
	Meta,       // 돈/경험치 등 플레이어 메타
	// 상세 스냅샷은 목록 갱신(Unit/Skill)과 분리해 알린다. Unit은 HP 변경마다
	// 오므로, 같은 도메인에 실으면 롱프레스 상세 패널이 HP 갱신마다 열린다.
	UnitDetail,
	SkillDetail
};

/** @brief 전투 조작 UI의 단계. UI 버튼/하이라이트 레이어 전환에 쓰는 UI 전용 상태다(게임플레이 enum의 1:1 거울이 아님). */
// UI 필요값: 스킬 선택/조준/미리보기 중 어느 조작 레이어를 열지 결정한다.
// 구성: 게임플레이 develop의 ESRPGSkillBuildPhase는 None/AimSelection/Preview 3개뿐이다.
//   - AimSelection/Preview = 그 enum과 직접 대응.
//   - SkillSelected = 게임플레이엔 페이즈로 없고 SRPGSkillBuildAction 상태에서 어댑터가 파생해 채우는 UI 하위상태다.
// [합의필요] AimSelection/Preview를 ESRPGSkillBuildPhase와 매핑하는 값만 게임플레이와 맞춘다.
UENUM(BlueprintType)
enum class ECombatBuildPhaseUI : uint8
{
	None,
	SkillSelected,   // [UI 파생] 스킬을 골랐음(mSelectedSkillIndex != NONE)
	AimSelection,    // [거울] ESRPGSkillBuildPhase::AimSelection
	Preview          // [거울] ESRPGSkillBuildPhase::Preview
};

/** @brief 확정 전 AP 소모 표시가 어떤 행동을 가리키는지 구분한다. */
UENUM(BlueprintType)
enum class ECombatPendingActionType : uint8
{
	None,
	Move,
	Skill
};

/** @brief 현재 프리뷰 중인 행동의 AP 예정 소모 데이터. */
USTRUCT(BlueprintType)
struct FCombatPendingActionUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) ECombatPendingActionType mType = ECombatPendingActionType::None;
	UPROPERTY(BlueprintReadOnly) int32 mActionPointCost = 0;
};

/**
 * @brief 플로팅 로그 아이콘의 "의미". 실제 어떤 Texture2D를 쓸지는 HUD(ResolveFloatingLogIcon)가 고른다.
 * @details 게임플레이는 "이건 HP다 / 독이다"라는 의미만 정하고 그림은 UI가 정한다(디자인 요소는 UI 소관).
 *          None은 아이콘 없이 텍스트만 띄운다.
 */
UENUM(BlueprintType)
enum class EFloatingLogIconType : uint8
{
	None,				// 아이콘 없음(텍스트만)
	HP,					// 체력

	GetMove,			// 이동력 획득
	GetDefense,			// 방어력 획득

	Weakness,			// 약화 (디버프)
	Vulnerability,		// 취약 (디버프)
	Fortification,		// 요새화 (버프)
	Vigor,				// 활력 (버프)

	Poison,				// 독
	Fire,				// 화염
	Move				// 이동
};

/**
 * @brief 플로팅 로그 색의 "의미". 실제 FLinearColor(어떤 빨강/초록…)는 HUD(ResolveFloatingLogColor)가 고른다.
 * @details 아이콘과 마찬가지로 게임플레이는 의미(피해/회복/버프…)만 넘긴다. 실제 색상 톤은 UI가 정한다.
 */
UENUM(BlueprintType)
enum class EFloatingLogColorType : uint8
{
	Neutral, // 중립(흰색)
	Damage,  // 피해
	Heal,    // 회복
	PointUp, // 이동력이나 방어력등의 수치 증가
	Buff,    // 버프
	Debuff,  // 디버프
	Warning, // 경고(쓰러짐 등)
	Move     // 이동
};

USTRUCT(BlueprintType)
struct FCombatResultUI
{
	GENERATED_BODY()

public:
	/** @brief 승리 여부 */
	UPROPERTY(BlueprintReadWrite) bool mIsWin = false;
	/** @brief 결과 화면에 표시할 현재 전투 지역 이름. */
	UPROPERTY(BlueprintReadWrite) FText mLocationName;
	/** @brief 패배가 확정된 라운드. */
	UPROPERTY(BlueprintReadWrite) int32 mRound = 0;
	/** @brief 이번 전투에서 처치한 몬스터 수. */
	UPROPERTY(BlueprintReadWrite) int32 mDefeatedMonsterCount = 0;
	/** @brief 패배 결과에서 확정된 골드 획득량. */
	UPROPERTY(BlueprintReadWrite) int32 mGoldGained = 0;
	/** @brief 패배 결과에서 확정된 경험치 획득량. */
	UPROPERTY(BlueprintReadWrite) int32 mExpGained = 0;
	/** @brief 패배 카드에 표시할 파티 초상화(최대 3명). */
	UPROPERTY(BlueprintReadWrite) TArray<TObjectPtr<UTexture2D>> mPartyPortraits;
};

/**
 * @brief 플로팅 로그 한 건을 "어디에 / 무엇을 / 어떻게 다룰지" 담아 게임플레이가 UI로 넘기는 요청.
 * @details 게임플레이가 채워 CombatUIModel::NotifyCombatFloatingLog(s)로 보내면 HUD가 그린다.
 *          - 위치는 호출자가 정한다(유닛 위/타일 위 판별을 UI가 하지 않는다) → mWorldLocation.
 *          - 아이콘/색은 "의미(enum)"로만 준다 → 실제 텍스처·색은 HUD가 매핑.
 *          - (mTurnIndex, mActionIndex, mMotionIndex)는 UI까지 그대로 전달해
 *            후속 표시 그룹화나 재생 상관관계에 사용할 수 있도록 보존한다.
 */
USTRUCT(BlueprintType)
struct FCombatFloatingLogRequest
{
	GENERATED_BODY()

	/** @brief 로그를 띄울 월드 좌표. 유닛 위/타일 위 판단은 호출자가 끝낸 뒤 전달한다. */
	UPROPERTY(BlueprintReadWrite) FVector mWorldLocation = FVector::ZeroVector;

	/** @brief 화면에 표시할 문구. */
	UPROPERTY(BlueprintReadWrite) FText mText;

	/** @brief HUD가 실제 Texture2D로 변환할 아이콘 의미값. */
	UPROPERTY(BlueprintReadWrite) EFloatingLogIconType mIconType = EFloatingLogIconType::None;

	/** @brief HUD가 실제 색상으로 변환할 색상 의미값. */
	UPROPERTY(BlueprintReadWrite) EFloatingLogColorType mColorType = EFloatingLogColorType::Neutral;

	/** @brief 낮은 값부터 순차 표시한다. 같은 값이면 수신 순서를 따른다. */
	UPROPERTY(BlueprintReadWrite) int32 mSequence = 0;

	/** @brief TurnEventLogs 배열 기준 턴 인덱스. INDEX_NONE이면 특정 턴에 묶이지 않은 로그다. */
	UPROPERTY(BlueprintReadWrite) int32 mTurnIndex = INDEX_NONE;

	/** @brief 해당 턴의 ActionEventLogs 배열 기준 액션 인덱스. INDEX_NONE이면 특정 액션에 묶이지 않은 로그다. */
	UPROPERTY(BlueprintReadWrite) int32 mActionIndex = INDEX_NONE;

	/** @brief 같은 액션의 MotionEventLogs 배열 기준 인덱스. INDEX_NONE이면 수명 시간으로만 자동 제거한다. */
	UPROPERTY(BlueprintReadWrite) int32 mMotionIndex = INDEX_NONE;

	/** @brief true면 미리보기 로그 — 수명으로 자동 소멸하지 않고, 모션 종료(MotionFinished)나 전체 클리어로만 사라진다. */
	UPROPERTY(BlueprintReadWrite) bool mIsPreview = false;
};

/** @brief UI에 전달된 전투 이벤트가 예측 결과인지 실제 전투 결과인지 구분한다. */
// 라우팅은 모델이 나눠 든다(예측=USimulationPreviewUIModel, 실전=UCombatUIModel) —
// 이 태그는 어느 경로에서 온 배치인지 기록만 한다(로그/디버그용).
UENUM(BlueprintType)
enum class ECombatEventDataSourceUI : uint8
{
	None,
	SimulationPreview,
	LiveCombat
};

/**
 * @brief 시뮬레이션과 실제 전투가 공통으로 사용하는 UI 이벤트 모델 스냅샷.
 * @details UI는 원본 전투 객체를 읽지 않고 이 데이터만 구독하므로 두 실행 경로를 같은 방식으로 표시할 수 있다.
 */
USTRUCT(BlueprintType)
struct FCombatEventBatchUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) ECombatEventDataSourceUI mSource = ECombatEventDataSourceUI::None;
	UPROPERTY(BlueprintReadOnly) int32 mRevision = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FCombatFloatingLogRequest> mFloatingLogs;
};

/** @brief 시뮬레이션 미리보기 뷰모델에서 무엇이 바뀌었는지 도메인 구분(부분 갱신용). */
// ECombatUIDomain의 미리보기판이다. 실전 도메인과 섞지 않는다 — 미리보기 위젯은
// 이 값만 보고 자기 표시를 갱신하고, 실전 위젯은 이 알림을 아예 듣지 않는다.
UENUM(BlueprintType)
enum class ESimulationPreviewDomainUI : uint8
{
	All,
	Events,
	Units,
	PendingAction
};

/** @brief 유닛 머리 위 HP바에 표시할 상태이상 한 칸(버프/디버프 아이콘 + 스택 수). */
// UI 필요값:
// - mTag: 어떤 상태이상인지(요새화/신속/약화/취약 …). UI는 태그로만 받고 게임플레이 enum에 의존하지 않는다.
// - mStackCount: 스택 수(StatusCountText 표시). 어댑터가 AttributeSet의 GetTagCount로 채운다.
// [모호재 #290] 이 값을 채우는 곳은 PushUnitUIData(태그 순회 + GetTagCount). 태그→아이콘 텍스처 매핑은 HUD가 소유.
USTRUCT(BlueprintType)
struct FStatusEffectUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FGameplayTag mTag;
	UPROPERTY(BlueprintReadOnly) int32 mStackCount = 0;
};

/** @brief 시뮬레이션 미리보기가 예고하는 유닛 한 기의 결과 요약(HP 증감·사망·도착 타일). */
// 소스 = FSRPGTurnEventLog 집계(ACombatGameMode::BuildUnitPredictions). 실전 스냅샷
// (FUnitUI)과 저장 자리를 나눈다 — 예측은 실제와 다를 수 있고, 무르면 실전을 건드리지
// 않고 통째로 버려져야 한다. mPredictedHP(절대값)는 1차 구현에서 보류 — 델타만 나른다.
// (FStatusEffectUI 정의 뒤에 두어야 해서 ESimulationPreviewDomainUI와 자리가 떨어져 있다.)
USTRUCT(BlueprintType)
struct FUnitPredictionUI
{
	GENERATED_BODY()

	/** @brief FUnitUI.mUnitId와 같은 id 공간. */
	UPROPERTY(BlueprintReadOnly) int32 mUnitId = INDEX_NONE;
	/** @brief 예측 구간 동안의 HP 증감 합. 음수면 피해. */
	UPROPERTY(BlueprintReadOnly) float mHPDelta = 0.f;
	/** @brief 예측 구간 안에서 판을 떠나는가(사망/제거 Exit 로그 기준). */
	UPROPERTY(BlueprintReadOnly) bool mWillDie = false;
	/** @brief 예측 구간이 끝났을 때 서 있을 타일. 이동 로그가 없으면 Invalid. */
	UPROPERTY(BlueprintReadOnly) FTileIndex mPredictedTile = FTileIndex::Invalid;
	/** @brief 예측 상태이상. 이벤트 로그의 태그 값은 증감(델타)뿐이라 절대 스택을 지어낼 수 없어 1차에서는 비워 둔다. */
	UPROPERTY(BlueprintReadOnly) TArray<FStatusEffectUI> mPredictedStatuses;
};

/** @brief 유닛 한 기를 HUD에 그릴 때 필요한 표시값(HP바·스탯·위치). */
// UI 필요값:
// - mUnitId/mIsPlayer: 플레이어/적 구분, 터치 타겟, 상세창 요청 payload에 필요하다.
// - mHP/mMaxHP: 머리 위 HP바와 적/플레이어 생존 상태 표시.
// - mDamagePoint/mDefensePoint/mSkillPoint: 전투 HUD/상세 패널에 노출할 스탯.
// - mMovementPoint/mMaxMovementPoint: STEP/MOVE 후 이동력 게이지와 MOVE 버튼 상태 표시(유닛 자원 — 이동 가능 "영역"과는 별개. 영역은 ATileMap 쿼리 → 타일맵 파트(ATileMap) 하이라이트).
// - mTile: 이 유닛이 올라간 타일. [소스] ATileMap 점유의 거울값(권위 아님) — 점유 단일 진실원본은 타일맵 파트(ATileMap).mOccupantUnitId다.
//          UI는 "유닛을 그 타일 위에 그린다" 방향으로만 쓰고, "이 타일에 누가 있나" 판정은 mTile에서 역산하지 말 것.
// - mWorldLocation: 월드 유닛 위치를 스크린으로 투영해 HP바/상태 아이콘을 띄운다.
// - mStatusTags: 버프/디버프 아이콘 표시. UI가 게임플레이 상태 enum에 직접 의존하지 않게 태그로 받는다.
// [책임 경계] "어디"(점유·범위·오버랩)=ATileMap 권위, "무엇"(HP·스탯·상태)=유닛 권위. (06/04·06/09 회의)
// [합의필요] HP/MaxHP/스탯 최종 소스는 UUnitData 쪽에서 공급하는 방향.
USTRUCT(BlueprintType)
struct FUnitUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mUnitId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) bool mIsPlayer = false;
	/** @brief 유닛 세로 초상화(DA mPortrait). 파티·적·상세 카드용. */
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mPortrait;
	/**
	 * @brief 턴바 얼굴 초상화(DA mIcon).
	 *
	 * @details 256x256 HeadV2를 우선 사용한다. 어댑터가 아이콘 없는 신규
	 *          유닛에는 mPortrait를 넣으므로 턴바가 빈칸이 되지 않는다.
	 */
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mTurnPortrait;
	// 파티 카드가 3명의 이름을 동시에 보여줘야 해서 유닛 목록 쪽에도 필요하다.
	// mName은 FUnitDetailUI에도 있지만 그쪽은 한 번에 한 유닛(롱프레스 상세)이다.
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) float mHP = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxHP = 0.f;
	// 행동력. 이동과 스킬이 함께 쓰는 단일 자원이라 유닛 단위로 표시해야 한다.
	// 원본은 SkillComponentModel의 mActionPoints / mMaxActionPoints.
	UPROPERTY(BlueprintReadOnly) int32 mActionPoints = 0;
	UPROPERTY(BlueprintReadOnly) int32 mMaxActionPoints = 0;
	/** @brief 턴 순서 칩 아래에 표시할 현재 속도. */
	UPROPERTY(BlueprintReadOnly) float mSpeedPoint = 0.f;
	UPROPERTY(BlueprintReadOnly) float mDamagePoint = 0.f;
	UPROPERTY(BlueprintReadOnly) float mDefensePoint = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMovementPoint = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxMovementPoint = 0.f;   // STEP으로 확보한 이동 가능 총량(현재/최대 표시용)
	UPROPERTY(BlueprintReadOnly) float mSkillPoint = 0.f;
	UPROPERTY(BlueprintReadOnly) FTileIndex mTile;

	// 적 요약판의 "다음 스킬" 소켓용 아이콘. 장착 스킬 중 첫 유효 슬롯을 대표로 건다
	// (플래너의 실제 계획은 적 턴에야 나오므로, 상시 표시는 대표 스킬로 한다).
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mNextSkillIcon;
	// 위 아이콘이 스킬 배열 몇 번째인지 -- 소켓 클릭 시 상세 요청에 쓴다.
	UPROPERTY(BlueprintReadOnly) int32 mNextSkillIndex = INDEX_NONE;

	// 몬스터 탭 목록 줄의 "Lv N" 배지용. 상세 응답을 기다리지 않고 목록에서 바로 쓴다.
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 0;

	// 머리 위 HP바를 월드→스크린 투영으로 띄우기 위한 유닛 월드 위치. 어댑터가 채우고 UI는 투영만 한다.
	// (스냅샷 — 뷰 액터가 없을 때의 폴백. 이동 중 갱신 안 됨)
	UPROPERTY(BlueprintReadOnly) FVector mWorldLocation = FVector::ZeroVector;

	// 머리 위 HP바가 이동을 매 프레임 따라가도록 하는 라이브 투영 소스(약참조, 투영 전용).
	// 어댑터가 유닛 뷰 액터를 채우고, UI는 유효하면 이 액터의 현재 GetActorLocation()을 투영한다(스냅샷보다 우선).
	UPROPERTY() TWeakObjectPtr<AActor> mViewActor;

	// 머리 위 버프/디버프 아이콘용. 정확한 태그와 수가 아닌 대략적인 정보를 담고 있다.
	UPROPERTY(BlueprintReadOnly) FGameplayTagContainer mOwnedTags;

	// 머리 위 HP바 상태이상 칸(태그 + 스택 수). HP바가 GetUnitUIs()로 매 프레임 읽어 아이콘/개수를 채운다.
	// (mStatusTags는 집합만이라 스택 수가 없어서, 개수까지 표시하려면 이 배열을 쓴다.)
	UPROPERTY(BlueprintReadOnly) TArray<FStatusEffectUI> mStatusEffects;
};

/** @brief 유닛 상세창에 늘어놓는 스킬 한 칸. 탭하면 그 스킬의 상세로 넘어간다. */
// UI 필요값:
// - mSkillIndex: 탭했을 때 되돌려 보낼 왕복 식별자. **그 상세창에 뜬 유닛의** 스킬 배열 기준이다
//                (카드 레일의 FSkillUI.mSkillIndex와 같은 배열이 아니다 — 적 스킬일 수도 있다).
// - mName: 그림이 없을 때 대신 적을 글자. 손가락을 올려 둘 때의 안내에도 쓴다.
// - mIcon: 칸에 그릴 그림.
USTRUCT(BlueprintType)
struct FUnitDetailSkillUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mSkillIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
};

/** @brief 적/유닛을 길게 눌렀을 때 띄우는 상세 정보(초상화·이름·레벨·패시브 등). */
// 이 struct는 "상세 패널에만 추가로 필요한 값"만 담는다.
// HP/MaxHP/AttackFactor 같은 라이브 전투 스탯은 여기서 중복 보관하지 않고,
// mUnitId로 같은 유닛의 FUnitUI를 찾아 거기서 읽는다(HP 진실원본 이원화 방지).
// UI 필요값:
// - mUnitId: 어떤 유닛 상세인지 식별 + 라이브 스탯(HP 등)을 가져올 FUnitUI 매칭 키.
// - mName/mLevel/mPortrait: 큰 정보 패널 헤더와 초상화 표시.
// - mPassiveDescriptions: 적 패시브/특수 규칙을 텍스트 리스트로 표시.
// [합의필요] 이름/초상화/패시브 최종 소스는 UUnitData 연결 필요.
USTRUCT(BlueprintType)
struct FUnitDetailUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mUnitId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 0;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mPortrait = nullptr;
	UPROPERTY(BlueprintReadOnly) TArray<FText> mPassiveDescriptions;
	/** @brief 이 유닛이 들고 있는 스킬 칸들. 탭하면 RequestInspectUnitSkill로 상세를 청한다. */
	UPROPERTY(BlueprintReadOnly) TArray<FUnitDetailSkillUI> mSkills;
};

/** @brief 스킬 시전(선택) 범위 형태. UI 조준 가이드용. 스킬데이터 SelectType의 UI 거울. */
// [합의필요] develop 최종 SelectType enum과 매핑 필요(아래는 스킬데이터 설계 표의 후보 형태).
UENUM(BlueprintType)
enum class ECombatSkillSelectShapeUI : uint8
{
	None,
	Single,     // 단일 타일
	Square,     // 사각형
	Cross,      // 십자
	Diagonal,   // 8방향/대각
	Line        // 직선(Liner — 사거리 1 고정, 06/04 회의)
};

/** @brief 스킬 타격 범위 형태. UI 피격 가이드용. 스킬데이터 HitType의 UI 거울. */
// [합의필요] develop 최종 HitType enum과 매핑 필요.
UENUM(BlueprintType)
enum class ECombatSkillHitShapeUI : uint8
{
	None,
	Single,     // 단일 타일
	Cross,      // 십자형 폭발
	Circle,     // 원형
	// 8방향(직교+대각). 게임플레이 진실은 TileMapModel::GetEffectTiles의
	// EEffectPattern::Star -- 예전에는 Cross로 뭉개져 대각 줄이 안 그려졌다.
	Star
};

/**
 * @brief 영향 시점 타일을 정하는 타겟 패턴. 스킬데이터 ETargetPattern의 UI 거울.
 *
 * @details LineToTarget은 시전자→조준 타일 경로 전체가 영향 시점이 된다
 * (PR #466). 모식도는 이 값으로 시전자→조준 방향의 경로 점선을 그린다.
 */
UENUM(BlueprintType)
enum class ECombatSkillTargetPatternUI : uint8
{
	Default,        // 조준 타일 한 칸 (ETargetPattern::TargetOnly)
	LineToTarget    // 시전자에서 조준 타일까지의 경로 전체
};

/** @brief 스킬 조준/타격 가이드 표시값(스킬데이터가 "UI 가이드라인 출력용"이라 명시한 메타). */
// 이 struct가 먹이는 것은 판 밖의 개략 모식도(무장애물 가정 개략도)다 — 상세창의
// 9x9 전술판처럼 "모양과 거리"만 설명하고, 장애물/점유에 따른 실제 차단은 흉내내지 않는다.
// 권위 있는 판 위 색칠은 여전히 타일맵 하이라이트다: 게임플레이가 ATileMap 쿼리로
// 확정 조준/영향 타일을 계산해 내려주고, 이 값으로 판을 다시 칠하면 안 된다.
// 소스 = StaticSkillData(AimPattern/AimRange/EffectPattern/EffectArea/AimBlockerMask/EffectBlockerMask/TargetPattern).
USTRUCT(BlueprintType)
struct FSkillTargetingUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) ECombatSkillSelectShapeUI mSelectShape = ECombatSkillSelectShapeUI::None;
	UPROPERTY(BlueprintReadOnly) float mSelectRange = 0.f;        // 기본 사정거리
	UPROPERTY(BlueprintReadOnly) ECombatSkillHitShapeUI mHitShape = ECombatSkillHitShapeUI::None;
	UPROPERTY(BlueprintReadOnly) float mHitRange = 0.f;           // 기본 타격 범위
	/**
	 * @brief 조준/영향을 막는 레이어. ETileLayerFlag 비트마스크 그대로다.
	 *
	 * @details
	 * 곡사(mIsIndirect)/관통(mIsPenetration) bool 두 개를 대신한다. 그 둘로는
	 * "장애물만 막힘" 과 "유닛만 막힘" 을 구분할 수 없어서, 세 경우가 전부
	 * 같은 값으로 뭉개졌다.
	 *
	 * 0 이면 아무것도 막지 않는다(= 옛 곡사/관통). INDEX_NONE 은 "게임플레이가
	 * 아직 안 채웠다" 는 뜻이라 화면은 값을 지어내지 말고 비워 둬야 한다.
	 */
	UPROPERTY(BlueprintReadOnly) int32 mAimBlockerMask = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 mEffectBlockerMask = INDEX_NONE;
	/** @brief 영향 시점 패턴. LineToTarget이면 모식도가 시전자→조준 경로를 그린다. */
	UPROPERTY(BlueprintReadOnly) ECombatSkillTargetPatternUI mTargetPattern = ECombatSkillTargetPatternUI::Default;
};

/**
 * @brief 지금 겨냥한 자리.
 *
 * @details
 * 판을 톡 치면 그 칸이 겨냥한 자리가 된다. **타일 하나**이고, 그 위에 유닛이
 * 있으면 유닛 id 도 같이 온다. 빈 칸도 겨냥할 수 있다 -- 이동 목적지가 그렇다.
 *
 * 왜 유닛 id 만으로 안 되나: "여기로 이동" 은 유닛이 없는 칸을 가리킨다.
 * 유닛만 겨냥할 수 있게 하면 이동이 이 흐름 밖으로 밀려난다.
 *
 * UI 는 이 값을 만들지 않는다. 화면 좌표를 RequestWorldTouch 로 넘기면
 * 게임플레이가 어느 타일인지 풀어서 SetTarget 으로 내린다 -- 화면은 타일맵
 * 좌표계를 모른다.
 *
 * 이 값이 바뀌면 게임플레이가 FSkillUI.mIsUsable 을 다시 계산해 내려준다.
 * 그래서 "사거리에 따라 카드가 켜지고 꺼진다" 는 UI 가 판정하는 것이 아니라
 * 받아 그리는 것이다.
 */
USTRUCT(BlueprintType)
struct FCombatTargetUI
{
	GENERATED_BODY()

	/** @brief 겨냥한 것이 있나. 없으면 아래 값은 뜻이 없다. */
	UPROPERTY(BlueprintReadOnly) bool mIsValid = false;

	/** @brief 겨냥한 타일. */
	UPROPERTY(BlueprintReadOnly) FTileIndex mTile;

	/** @brief 그 타일에 선 유닛. 빈 칸이면 INDEX_NONE. */
	UPROPERTY(BlueprintReadOnly) int32 mUnitId = INDEX_NONE;
};

/** @brief 스킬 레일에 그릴 스킬 한 칸. */
// UI 필요값:
// - mSkillIndex: 클릭/롱프레스 시 RequestSelectSkill/RequestLongPressSkill payload.
// - mName/mIcon: 스킬 버튼의 기본 표시.
// - mIsUsable: 현재 페이즈/자원에서 누를 수 있는지 비활성 표시.
// - mTargeting: 스킬 선택 시 조준 가이드(사거리/형태)를 즉시 그리기 위한 시전/타격 범위 메타.
// [합의필요] 최종 소스는 USkillComponent 또는 스킬 빌드 액션 쪽과 매핑 필요.
USTRUCT(BlueprintType)
struct FSkillUI
{
	GENERATED_BODY()

	// UI payload로 왕복하는 index. 최종 스킬 데이터 연결 시 SkillId와 1:1 매핑되어야 한다.
	UPROPERTY(BlueprintReadOnly) int32 mSkillIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	// 새 전투 규칙: 이동과 스킬이 행동력 하나를 나눠 쓰고, 스킬은 턴 쿨타임을
	// 가지며, 피해는 min~max에 크리티컬이 max의 1.5배다. 스킬 카드가 이 넷을
	// 모두 보여줘야 플레이어가 무엇을 고를지 판단할 수 있다.
	UPROPERTY(BlueprintReadOnly) int32 mActionPointCost = 0;
	UPROPERTY(BlueprintReadOnly) int32 mActionPointGain = 0;
	UPROPERTY(BlueprintReadOnly) int32 mCooldownTurns = 0;
	UPROPERTY(BlueprintReadOnly) int32 mRemainingCooldown = 0;
	UPROPERTY(BlueprintReadOnly) int32 mDamageMin = 0;
	UPROPERTY(BlueprintReadOnly) int32 mDamageMax = 0;
	UPROPERTY(BlueprintReadOnly) int32 mCriticalDamage = 0;
	UPROPERTY(BlueprintReadOnly) bool mIsUsable = false;
	UPROPERTY(BlueprintReadOnly) FSkillTargetingUI mTargeting;
};

/** @brief 스킬을 길게 눌렀을 때의 상세창 내용. */
// UI 필요값:
// - mSkillIndex: 현재 상세창이 어떤 스킬을 설명하는지 식별.
// - mName/mDescription/mIcon: 롱프레스 상세 패널의 제목/본문/아이콘.
// - mTargeting: 상세 패널에서 사거리/타격범위/곡사·관통 등 풀스펙 안내.
// [합의필요] 스킬 설명/아이콘 최종 데이터 연결 필요.
USTRUCT(BlueprintType)
struct FSkillDetailUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mSkillIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) FText mDescription;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	// 상세 응답을 요청한 유닛의 정적 스킬 수치다. 스킬 index는 유닛별
	// 슬롯이라 플레이어 카드 레일의 같은 index를 다시 조회하면 안 된다.
	UPROPERTY(BlueprintReadOnly) int32 mActionPointCost = 0;
	UPROPERTY(BlueprintReadOnly) int32 mActionPointGain = 0;
	UPROPERTY(BlueprintReadOnly) int32 mCooldownTurns = 0;
	UPROPERTY(BlueprintReadOnly) int32 mDamageMin = 0;
	UPROPERTY(BlueprintReadOnly) int32 mDamageMax = 0;
	UPROPERTY(BlueprintReadOnly) int32 mCriticalDamage = 0;
	UPROPERTY(BlueprintReadOnly) FSkillTargetingUI mTargeting;
};


/** @brief 행동/시뮬레이션 결과 큐의 한 노드이며 애니메이션 한 단위에 해당합니다. */
// 여러 효과를 한 노드에 담을지 효과별로 나눌지는 합의 전이다. 현재 계약은 mTags로 종류를 구분한다.
// UI 필요값:
// - mTags: 데미지/힐/상태이상/밀침 등 연출 종류와 색/아이콘 결정.
// - mSourceUnitId/mTargetUnitId: 발동자/대상 위치를 찾아 투사체, 머리 위 숫자, 흔들림 연출에 사용.
// - mAmount: 데미지/힐 수치처럼 숫자로 띄울 값.
// - mLabel: 숫자 외 텍스트(Stun, Miss 등)를 그대로 띄울 때 사용.
// [합의필요] 한 노드에 여러 효과를 묶을지, 효과별로 노드를 쪼갤지는 게임플레이/연출 합의 필요.
USTRUCT(BlueprintType)
struct FCombatQueueNode
{
	GENERATED_BODY()

	// 태그가 연출 종류의 확장 지점이다. enum 분기 추가 대신 GameplayTag로 UI 효과를 확장한다.
	UPROPERTY(BlueprintReadOnly) FGameplayTagContainer mTags;   // Combat.Damage / Status / Heal / Push 등
	UPROPERTY(BlueprintReadOnly) int32 mSourceUnitId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 mTargetUnitId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 mAmount = 0;             // 데미지/힐 수치 등
	UPROPERTY(BlueprintReadOnly) FText mLabel;                  // 머리 위에 띄울 텍스트
};

/** @brief 장비 슬롯 하나(전투 중 표시·상세). 인벤토리/장착 변경은 의도(Request)로만 보낸다. */
// UI 필요값:
// - mSlotIndex: 장비 슬롯 클릭/롱프레스 payload.
// - mItemId: 실제 아이템 데이터와 매칭할 식별자.
// - mName/mIcon: 슬롯 표시와 상세 패널 제목/아이콘.
// - mIsEquipped: 장착 상태 강조.
// - mRarityColor: 희귀도 테두리/배경색.
// [합의필요] 장비/인벤토리 최종 데이터 소스 연결 필요. 현재 어댑터는 임시 슬롯만 넣는다.
USTRUCT(BlueprintType)
struct FEquipmentUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mSlotIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FPrimaryAssetId mItemId;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) bool mIsEquipped = false;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
};

/** @brief 장비 슬롯 롱프레스 상세(공용 상세창 표시용). 목록 칩(FEquipmentUI)과 달리 설명 등 상세 필드를 포함한다. */
// UI 필요값:
// - mSlotIndex: 어느 슬롯의 상세인지(요청 payload와 매칭).
// - mItemId: 실제 아이템 데이터 식별자.
// - mName/mIcon: 상세 패널 제목/아이콘.
// - mIsEquipped: 장착 상태 부제("장착 중").
// - mDescription: 상세 본문(목록 DTO에는 없는 필드).
// - mRarityColor: 희귀도 강조색.
// [모호재 #290] 이 DTO를 채우는 PushEquipmentDetailUIData(게임플레이 장비모델→값)는 GameMode에서 구현 필요.
USTRUCT(BlueprintType)
struct FEquipmentDetailUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mSlotIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FPrimaryAssetId mItemId;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) bool mIsEquipped = false;
	UPROPERTY(BlueprintReadOnly) FText mDescription;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
};

/** @brief 전투 HUD에 늘 꺼내 놓는 파티 공용 아티팩트 한 칸. */
USTRUCT(BlueprintType)
struct FCombatArtifactUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;

	/**
	 * @brief 아티팩트를 꾹 눌렀을 때 보여 줄 값들.
	 *
	 * @details 이름과 그림만으로는 "이게 무슨 효과인지" 를 알 수 없어 아이콘을
	 * 눌러도 볼 것이 없었다. 설명은 유닛 패시브와 같은 방식으로 붙인 패시브의
	 * mDescription 을 모아 만든다 -- 아티팩트가 따로 설명 글을 갖지 않는다.
	 */
	UPROPERTY(BlueprintReadOnly) TArray<FText> mEffectDescriptions;
	UPROPERTY(BlueprintReadOnly) FText mRarityName;
	UPROPERTY(BlueprintReadOnly) int32 mPrice = 0;

	/** @brief 희귀도 등급(0=일반, 1=희귀, 2=영웅). 상세창 보석 줄이 이 값을 켠다. */
	UPROPERTY(BlueprintReadOnly) int32 mRarityLevel = 0;
};

/** @brief 플레이어 메타 정보(전투 HUD 상단/보상에 쓰는 돈·경험치·레벨). */
// UI 필요값:
// - mGold: 상단 HUD의 현재 골드.
// - mLevel: 플레이어/런 레벨 표시.
// - mExp/mMaxExp: 경험치 바와 레벨업 진행도 표시.
// - mArtifacts: 좌하단 AP 위에 늘 보이는 파티 공용 아티팩트.
// [합의필요] Gold/Exp 최종 소스는 UUnitData/URunPersistData 쪽에서 정리 필요.
USTRUCT(BlueprintType)
struct FPlayerMetaUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mGold = 0;
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 0;
	UPROPERTY(BlueprintReadOnly) float mExp = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxExp = 0.f;
	UPROPERTY(BlueprintReadOnly) TArray<FCombatArtifactUI> mArtifacts;
};

/** @brief 현재 턴/라운드/페이즈 상태. */
// UI 필요값:
// - mCurrentUnitId: 현재 조작/행동 주체 표시.
// - mRound: 라운드 카운터.
// - mPhase: 스킬 선택/조준/미리보기 등 UI 레이어 전환(ECombatBuildPhaseUI — UI 전용 상태).
// - mTurnOrderUnitIds: 턴 순서 바/다음 행동자 표시.
// - mCurrentRoundRemainingTurnCount: 위 배열에서 이번 라운드에 속하는 앞쪽 원소 수.
// [합의필요] mPhase의 AimSelection/Preview만 develop ESRPGSkillBuildPhase와 매핑(SkillSelected는 어댑터 파생).
USTRUCT(BlueprintType)
struct FTurnUI
{
	GENERATED_BODY()

	// FUnitUI.mUnitId와 같은 id 공간이다. Actor 포인터나 배열 index를 직접 노출하지 않는다.
	UPROPERTY(BlueprintReadOnly) int32 mCurrentUnitId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 mRound = 0;
	UPROPERTY(BlueprintReadOnly) ECombatBuildPhaseUI mPhase = ECombatBuildPhaseUI::None;
	/**
	 * @brief 이번 라운드에 **남은** 턴 순서(현재 턴이 [0]).
	 *
	 * @details 턴 개편(속도제) 후 모델의 턴 목록은 소비형이다 -- 지나간 턴은
	 *          빠진다. 그래서 이 배열은 한 바퀴 전체가 아니라 잔여분이다.
	 *          지나간 순서를 다시 그리려고 감아 돌리면 안 된다.
	 */
	UPROPERTY(BlueprintReadOnly) TArray<int32> mTurnOrderUnitIds;
	/**
	 * @brief 다음 라운드 순서 미리보기(속도 기준 재계산).
	 *
	 * @details 표기는 다음 라운드까지만 하기로 했다(0806 합의). 속도가 모자라
	 *          다음 라운드에 턴을 못 받는 유닛은 여기 없다.
	 */
	UPROPERTY(BlueprintReadOnly) TArray<int32> mNextRoundUnitIds;
	/** @brief mTurnOrderUnitIds 원소 수와 같다(전부 이번 라운드 잔여분). */
	UPROPERTY(BlueprintReadOnly) int32 mCurrentRoundRemainingTurnCount = 0;
	/**
	 * @brief 다음 턴이 실제로 서는 라운드까지의 거리(라운드 수).
	 *
	 * @details 충전 5·비용 10이면 턴은 두 라운드에 한 번 서고, 사이의 빈
	 *          라운드는 모델이 즉시 건너뛴다. 그래서 "다음" 이 mRound+1이
	 *          아닐 수 있다 -- 표기는 이 거리를 더한 라운드 번호로 한다.
	 */
	UPROPERTY(BlueprintReadOnly) int32 mNextRoundOffset = 1;
};
