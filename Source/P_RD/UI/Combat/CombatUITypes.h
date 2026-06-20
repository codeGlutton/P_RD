#pragma once

/** @brief 전투 UI와 게임플레이 사이에서 주고받는 표시용 뷰 데이터입니다. */
// UI는 게임플레이 객체(UUnitData/UDiceData 등)를 직접 알지 않고 이 struct들만 읽는다.
// FCombatQueueNode는 스킬/행동 결과를 애니메이션 한 단위씩 전달하기 위한 큐 노드다.

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"   // FTileIndex
#include "GameplayTagContainer.h"

#include "CombatUITypes.generated.h"

class UTexture2D;
class UTexture;

/** @brief 무엇이 바뀌어 UI를 갱신해야 하는지 도메인 구분(부분 갱신용). */
// UI 필요값: Set*() 호출 뒤 어떤 패널만 다시 그릴지 알려주는 최소 단위다.
// 예: SetDiceUIs -> Dice, SetUnitUIs -> Unit, SetActionQueue -> Queue.
UENUM(BlueprintType)
enum class ECombatUIDomain : uint8
{
	All,
	Dice,
	Skill,
	Unit,
	Turn,
	Queue,
	Equipment,
	Meta       // 돈/경험치 등 플레이어 메타
};

/** @brief 전투 조작 UI의 단계. UI 버튼/하이라이트 레이어 전환에 쓰는 UI 전용 상태다(게임플레이 enum의 1:1 거울이 아님). */
// UI 필요값: 스킬 선택/주사위 선택/조준/미리보기 중 어느 조작 레이어를 열지 결정한다.
// 구성: 게임플레이 develop의 ESRPGSkillBuildPhase는 None/AimSelection/Preview 3개뿐이다.
//   - AimSelection/Preview = 그 enum과 직접 대응.
//   - SkillSelected/DiceSelect = 게임플레이엔 페이즈로 없고 SRPGSkillBuildAction 상태(mSelectedSkillIndex 채워짐 / mSelectedDices 진행 중)에서
//     어댑터가 파생해 채우는 UI 하위상태다. 따라서 이 enum을 "develop enum의 거울"로 취급하지 말 것.
// [합의필요] AimSelection/Preview를 ESRPGSkillBuildPhase와 매핑하는 값만 게임플레이와 맞춘다(나머지 2개는 UI 파생).
UENUM(BlueprintType)
enum class ECombatBuildPhaseUI : uint8
{
	None,
	SkillSelected,   // [UI 파생] 스킬을 골랐음(mSelectedSkillIndex != NONE)
	DiceSelect,      // [UI 파생] 주사위를 올리는 중(스킬 빌드 진행)
	AimSelection,    // [거울] ESRPGSkillBuildPhase::AimSelection
	Preview          // [거울] ESRPGSkillBuildPhase::Preview
};

/** @brief 주사위 한 칸을 그릴 때 필요한 표시값입니다. */
// 어댑터가 UDiceData를 이 struct로 변환한다. 희귀도는 UI가 바로 쓸 수 있게 색/문구로 변환해 담는다.
// UI 필요값:
// - mDiceId: 슬롯이 어떤 런 보유 주사위인지 추적한다.
// - mResultValue/mIsRolled/mRolledFaceIndex: 굴림 전/후 표시, 선택 가능 여부, 3D 면 프리뷰 매칭에 필요하다.
// - mIsSelected: 스킬 빌드에 올린 주사위 강조 표시.
// - mIsUsed: 이번 턴 이미 소비된 주사위 잠금/비활성 표시.
// - mFaceCount/mFaceValues/mFaceTextures: d2/d4/d6/d8/d12/d20 종류와 면별 값/텍스처 표시.
// - mRarityColor/mRarityText: 희귀도 테두리/라벨 표시. UI가 희귀도 enum을 직접 알지 않게 어댑터가 변환한다.
// - mPreviewTexture: 후속 3D 캡처 계층이 붙으면 굴림면 렌더타깃을 표시한다. 현재는 비워질 수 있다.
USTRUCT(BlueprintType)
struct FDiceSlotUI
{
	GENERATED_BODY()

	// 런 보유 주사위 식별자. UI가 에셋을 직접 로드하기 위한 값이 아니라 추적/프리뷰 매칭용이다.
	UPROPERTY(BlueprintReadOnly) FPrimaryAssetId mDiceId;
	// 0은 미굴림 sentinel이다. 실제 주사위 면 값으로 0을 쓰면 ToggleDice 검증과 충돌한다.
	UPROPERTY(BlueprintReadOnly) int32 mResultValue = 0;     // 굴림 결과. 0 = 아직 안 굴림
	UPROPERTY(BlueprintReadOnly) int32 mRolledFaceIndex = INDEX_NONE;   // 실제 굴러진 물리 면 index(0-base)
	UPROPERTY(BlueprintReadOnly) bool mIsRolled = false;
	UPROPERTY(BlueprintReadOnly) bool mIsSelected = false;   // 스킬 빌드에 선택됨(빛남)
	UPROPERTY(BlueprintReadOnly) bool mIsDimmed = false;     // 선택이 꽉 차 더 못 올리는 비선택 주사위(어둡게)
	UPROPERTY(BlueprintReadOnly) bool mIsUsed = false;       // 이번 턴에 이미 쓴 주사위(턴 종료/다음 턴 시작까지 잠금)
	UPROPERTY(BlueprintReadOnly) int32 mFaceCount = 6;       // 면 수(종류 표시용: 2=동전 … 20=d20)
	UPROPERTY(BlueprintReadOnly) TArray<int32> mFaceValues;  // 각 물리 면에 적힌 실제 값
	UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<UTexture>> mFaceTextures;  // 각 물리 면 텍스처(nullptr이면 기본)
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly) FText mRarityText;

	// 주사위 굴림 면의 3D 프리뷰 슬롯. 별도 캡처 계층이 생기면 렌더타깃 텍스처를 넣고, UI는 표시만 한다.
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture> mPreviewTexture = nullptr;
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
	UPROPERTY(BlueprintReadOnly) float mHP = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxHP = 0.f;
	UPROPERTY(BlueprintReadOnly) float mDamagePoint = 0.f;
	UPROPERTY(BlueprintReadOnly) float mDefensePoint = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMovementPoint = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxMovementPoint = 0.f;   // STEP으로 확보한 이동 가능 총량(현재/최대 표시용)
	UPROPERTY(BlueprintReadOnly) float mSkillPoint = 0.f;
	UPROPERTY(BlueprintReadOnly) FTileIndex mTile;

	// 머리 위 HP바를 월드→스크린 투영으로 띄우기 위한 유닛 월드 위치. 어댑터가 채우고 UI는 투영만 한다.
	UPROPERTY(BlueprintReadOnly) FVector mWorldLocation = FVector::ZeroVector;

	// 머리 위 버프/디버프 아이콘용. enum 대신 태그로 받아 UI가 게임플레이 상태 enum에 의존하지 않게 한다.
	UPROPERTY(BlueprintReadOnly) FGameplayTagContainer mStatusTags;
};

/** @brief 적/유닛을 길게 눌렀을 때 띄우는 상세 정보(초상화·이름·레벨·패시브 등). */
// 이 struct는 "상세 패널에만 추가로 필요한 값"만 담는다.
// HP/MaxHP/DamagePoint 같은 라이브 전투 스탯은 여기서 중복 보관하지 않고,
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
	Circle      // 원형
};

/** @brief 스킬 조준/타격 가이드 표시값(스킬데이터가 "UI 가이드라인 출력용"이라 명시한 메타). */
// 조준 단계에서 "사거리 N / 십자 범위" 같은 안내와 가이드 형태를 그리기 위한 값이다.
// 주의: 실제 조준 가능 타일의 색칠은 여기서 하지 않는다 — 그건 ATileMap 쿼리 결과(타일맵 파트(ATileMap) 하이라이트)다.
//       이 struct는 "스킬 스펙 안내/예비 형태"만 담고, 확정 조준 타일은 게임플레이가 계산해 타일 하이라이트로 내려준다.
// [합의필요] 최종 소스 = StaticSkillData(SelectType/SelectRange/HitType/HitRange/Ratio/IsIndirect/IsPenetration).
USTRUCT(BlueprintType)
struct FSkillTargetingUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) ECombatSkillSelectShapeUI mSelectShape = ECombatSkillSelectShapeUI::None;
	UPROPERTY(BlueprintReadOnly) float mSelectRange = 0.f;        // 기본 사정거리
	UPROPERTY(BlueprintReadOnly) float mSelectRangeRatio = 0.f;   // 사거리에 주사위값 반영 계수
	UPROPERTY(BlueprintReadOnly) ECombatSkillHitShapeUI mHitShape = ECombatSkillHitShapeUI::None;
	UPROPERTY(BlueprintReadOnly) float mHitRange = 0.f;           // 기본 타격 범위
	UPROPERTY(BlueprintReadOnly) float mHitRangeRatio = 0.f;      // 타격 범위에 주사위값 반영 계수
	UPROPERTY(BlueprintReadOnly) bool mIsIndirect = false;        // 곡사(장애물/유닛 너머 타겟 가능)
	UPROPERTY(BlueprintReadOnly) bool mIsPenetration = false;     // 관통(막히지 않고 투사체가 뚫음)
};

/** @brief 스킬 레일에 그릴 스킬 한 칸. */
// UI 필요값:
// - mSkillIndex: 클릭/롱프레스 시 RequestSelectSkill/RequestLongPressSkill payload.
// - mName/mIcon: 스킬 버튼의 기본 표시.
// - mDiceCost: 필요한 주사위 개수/조건 표시.
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
	UPROPERTY(BlueprintReadOnly) int32 mDiceCost = 0;
	UPROPERTY(BlueprintReadOnly) bool mIsUsable = false;
	UPROPERTY(BlueprintReadOnly) FSkillTargetingUI mTargeting;
};

/** @brief 스킬을 길게 눌렀을 때의 상세창 내용. */
// UI 필요값:
// - mSkillIndex: 현재 상세창이 어떤 스킬을 설명하는지 식별.
// - mName/mDescription/mIcon: 롱프레스 상세 패널의 제목/본문/아이콘.
// - mDiceCost: 상세 패널에서도 요구 주사위 조건을 보여준다.
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
	UPROPERTY(BlueprintReadOnly) int32 mDiceCost = 0;
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

/** @brief 플레이어 메타 정보(전투 HUD 상단/보상에 쓰는 돈·경험치·레벨). */
// UI 필요값:
// - mGold: 상단 HUD의 현재 골드.
// - mLevel: 플레이어/런 레벨 표시.
// - mExp/mMaxExp: 경험치 바와 레벨업 진행도 표시.
// [합의필요] Gold/Exp 최종 소스는 UUnitData/URunPersistData 쪽에서 정리 필요.
USTRUCT(BlueprintType)
struct FPlayerMetaUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mGold = 0;
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 0;
	UPROPERTY(BlueprintReadOnly) float mExp = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxExp = 0.f;
};

/** @brief 현재 턴/라운드/페이즈 상태. */
// UI 필요값:
// - mCurrentUnitId: 현재 조작/행동 주체 표시.
// - mRound: 라운드 카운터.
// - mPhase: 스킬 선택/주사위 선택/조준/미리보기 등 UI 레이어 전환(ECombatBuildPhaseUI — UI 전용 상태).
// - mTurnOrderUnitIds: 턴 순서 바/다음 행동자 표시.
// [합의필요] mPhase의 AimSelection/Preview만 develop ESRPGSkillBuildPhase와 매핑(SkillSelected/DiceSelect는 어댑터 파생).
USTRUCT(BlueprintType)
struct FTurnUI
{
	GENERATED_BODY()

	// FUnitUI.mUnitId와 같은 id 공간이다. Actor 포인터나 배열 index를 직접 노출하지 않는다.
	UPROPERTY(BlueprintReadOnly) int32 mCurrentUnitId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 mRound = 0;
	UPROPERTY(BlueprintReadOnly) ECombatBuildPhaseUI mPhase = ECombatBuildPhaseUI::None;
	UPROPERTY(BlueprintReadOnly) TArray<int32> mTurnOrderUnitIds;
};
