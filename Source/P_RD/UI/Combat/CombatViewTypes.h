#pragma once

/**
 * @file CombatViewTypes.h
 * @brief 전투 UI와 게임플레이 사이에서 주고받는 '뷰 데이터' 정의입니다.
 *
 * @details
 * 데이터/비주얼을 분리하기로 한 팀 구조에 맞춰, UI는 게임플레이 객체(UUnitData/UDiceData 등)를
 * 직접 알지 않고 여기 정의된 표시용 struct만 받아 그립니다. 게임플레이가 리팩토링돼도 이 타입과
 * UCombatViewModel의 계약만 지키면 UI는 흔들리지 않습니다.
 *
 * 핵심은 FCombatQueueNode 입니다. 스킬/행동의 결과는 한 번에 적용되는 게 아니라, 애니메이션 한
 * 단위마다 한 노드씩 큐로 전달되고, 게임플레이가 한 노드를 처리할 때마다 UI가 하나씩 비우며 갱신합니다.
 */

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"   // FTileIndex
#include "GameplayTagContainer.h"

#include "CombatViewTypes.generated.h"

class UTexture2D;
class UTexture;

/** @brief 무엇이 바뀌어 UI를 갱신해야 하는지 도메인 구분(부분 갱신용). */
UENUM(BlueprintType)
enum class ECombatViewDomain : uint8
{
	All,
	Dice,
	Skill,
	Unit,
	Tile,
	Turn,
	Queue,
	Equipment,
	Meta       // 돈/경험치 등 플레이어 메타
};

/** @brief 스킬 빌드 진행 단계. develop의 ESRPGSkillBuildPhase와 의미를 맞춘 UI용 거울값. */
UENUM(BlueprintType)
enum class ECombatBuildPhaseView : uint8
{
	None,
	SkillSelected,
	DiceSelect,
	AimSelection,
	Preview
};

/**
 * @brief 주사위 한 칸을 그릴 때 필요한 표시값.
 *
 * @details
 * 게임플레이의 주사위 데이터(UDiceData)를 어댑터가 이 표시 struct로 변환한다. 희귀도는 enum 대신
 * 이미 계산된 색/문구로 담아, UI가 게임플레이 enum에 의존하지 않게 한다(RDUIDice로 어댑터가 미리 변환).
 */
USTRUCT(BlueprintType)
struct FDiceSlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FPrimaryAssetId mDiceId;
	UPROPERTY(BlueprintReadOnly) int32 mResultValue = 0;     // 굴림 결과. 0 = 아직 안 굴림
	UPROPERTY(BlueprintReadOnly) bool mIsRolled = false;
	UPROPERTY(BlueprintReadOnly) bool mIsSelected = false;   // 스킬 빌드에 선택됨
	UPROPERTY(BlueprintReadOnly) bool mIsUsed = false;       // 이번 턴에 이미 쓴 주사위(다음 굴림까지 잠금)
	UPROPERTY(BlueprintReadOnly) FLinearColor mRarityColor = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly) FText mRarityText;

	// 주사위 굴림 면의 3D 프리뷰. CombatDiceCaptureActor가 렌더타깃에 그린 결과를 어댑터가 넣어준다.
	// UI는 이 텍스처를 브러시로 표시만 한다(직접 캡처/회전하지 않음).
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture> mPreviewTexture = nullptr;
};

/** @brief 유닛 한 기를 HUD에 그릴 때 필요한 표시값(HP바·스탯·위치). */
USTRUCT(BlueprintType)
struct FUnitView
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
USTRUCT(BlueprintType)
struct FUnitDetailView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mUnitId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 0;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mPortrait = nullptr;
	UPROPERTY(BlueprintReadOnly) float mHP = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxHP = 0.f;
	UPROPERTY(BlueprintReadOnly) float mDamagePoint = 0.f;
	UPROPERTY(BlueprintReadOnly) TArray<FText> mPassiveDescriptions;
};

/** @brief 스킬 레일에 그릴 스킬 한 칸. */
USTRUCT(BlueprintType)
struct FSkillView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mSkillIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) int32 mDiceCost = 0;
	UPROPERTY(BlueprintReadOnly) bool mIsUsable = false;
};

/** @brief 스킬을 길게 눌렀을 때의 상세창 내용. */
USTRUCT(BlueprintType)
struct FSkillDetailView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mSkillIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FText mName;
	UPROPERTY(BlueprintReadOnly) FText mDescription;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> mIcon = nullptr;
	UPROPERTY(BlueprintReadOnly) int32 mDiceCost = 0;
};

/** @brief 타일 하나의 하이라이트 상태(조준/사거리/효과/막힘). FTile.mHighlight를 그대로 노출. */
USTRUCT(BlueprintType)
struct FTileViewState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FTileIndex mTile;
	UPROPERTY(BlueprintReadOnly) uint8 mHighlightFlag = 0;   // ETileHighlightFlag 비트값
	UPROPERTY(BlueprintReadOnly) int32 mOccupantUnitId = INDEX_NONE;
};

/**
 * @brief 행동/시뮬레이션 결과 큐의 한 노드. 애니메이션 한 단위에 해당한다.
 *
 * @details
 * 한 노드가 데미지·상태이상·힐 같은 여러 효과를 동시에 담을 수 있어, UI는 mTags로 종류를 구분해
 * 한 칸에 표시한다(타입 폭발을 피하려고 태그로 통일). 게임플레이가 이 노드를 처리(애니 재생)하면
 * UCombatViewModel::ResolveFrontQueueNode()로 하나씩 비운다.
 */
USTRUCT(BlueprintType)
struct FCombatQueueNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FGameplayTagContainer mTags;   // Combat.Damage / Status / Heal / Push 등
	UPROPERTY(BlueprintReadOnly) int32 mSourceUnitId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 mTargetUnitId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 mAmount = 0;             // 데미지/힐 수치 등
	UPROPERTY(BlueprintReadOnly) FText mLabel;                  // 머리 위에 띄울 텍스트
};

/** @brief 장비 슬롯 하나(전투 중 표시·상세). 인벤토리/장착 변경은 의도(Request)로만 보낸다. */
USTRUCT(BlueprintType)
struct FEquipmentView
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
USTRUCT(BlueprintType)
struct FPlayerMetaView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mGold = 0;
	UPROPERTY(BlueprintReadOnly) int32 mLevel = 0;
	UPROPERTY(BlueprintReadOnly) float mExp = 0.f;
	UPROPERTY(BlueprintReadOnly) float mMaxExp = 0.f;
};

/** @brief 현재 턴/라운드/페이즈 상태. */
USTRUCT(BlueprintType)
struct FTurnView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 mCurrentUnitId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 mRound = 0;
	UPROPERTY(BlueprintReadOnly) ECombatBuildPhaseView mPhase = ECombatBuildPhaseView::None;
	UPROPERTY(BlueprintReadOnly) TArray<int32> mTurnOrderUnitIds;
};
