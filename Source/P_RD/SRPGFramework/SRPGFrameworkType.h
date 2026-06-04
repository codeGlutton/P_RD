/*****************************************************************//**
 * @file   SRPGFrameworkType.h
 * @brief  SRPG 프레임워크에서 사용되는 타입 정의 헤더
 * @author 모호재
 * @date   2026-04-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFrameworkType.generated.h"

/**
 * @brief 타일에 배치된 액터가 바라보는 방향
 */
UENUM(BlueprintType)
enum class ETileActorDirection : uint8
{
    Forward = 0,
    Right,
    Backward,
    Left
};

/**
 * @brief 타일 맵 상 인덱스 좌표
 */
USTRUCT(BlueprintType)
struct FTileIndex
{
    GENERATED_BODY()

public:
    // @brief 가로(X) 방향 인덱스
    UPROPERTY(Category = "TileIndex", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "X"))
    int32 mX = 0;
    // @brief 세로(Y) 방향 인덱스
    UPROPERTY(Category = "TileIndex", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Y"))
    int32 mY = 0;
};

/**
 * @brief 타일 맵 상 위치
 */
USTRUCT(BlueprintType)
struct FTileTransform
{
    GENERATED_BODY()

public:
    // @brief 타일 인덱스 좌표
    UPROPERTY(Category = "TileTransform", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Index"))
    FTileIndex mIndex;
    // @brief 액터가 바라보는 방향
    UPROPERTY(Category = "TileTransform", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Direction"))
    ETileActorDirection mDirection = ETileActorDirection::Forward;

public:
    static const FTileTransform Zero;
};

inline const FTileTransform FTileTransform::Zero = FTileTransform();

/**
 * @brief 조준 범위 패턴
 */
UENUM(BlueprintType)
enum class EAimPattern : uint8
{
    Single      UMETA(ToolTip = "기준 타일 한 칸"),
    Cross       UMETA(ToolTip = "기준에서 4방향 직선"),
    Star        UMETA(ToolTip = "기준에서 8방향 직선"),
    Square      UMETA(ToolTip = "기준 중심 사각형 범위 전체"),
};

/**
 * @brief 영향 범위 패턴
 */
UENUM(BlueprintType)
enum class EEffectPattern : uint8
{
    Single      UMETA(ToolTip = "타겟 타일 한 칸"),
    Cross       UMETA(ToolTip = "타겟에서 4방향 직선"),
    Star        UMETA(ToolTip = "타겟에서 8방향 직선"),
    Square      UMETA(ToolTip = "타겟 중심 사각형 범위 전체"),
    Beam        UMETA(ToolTip = "시전자에서 타겟 방향으로 뻗는 직선"),
};

/**
 * @brief 스킬 대상 선정의 대상을 결정
 * 
 * @details
 * 영향 범위 외에 자신에게 영향 주는 스킬들이 있을 때 효과를 구분하기 위해 만든 열거형
 */
UENUM(BlueprintType)
enum class ETargetScope : uint8
{
    /** 대상 선정 기준: 시전자 본인 */
    Caster UMETA(DisplayName = "Caster"),

    /** 대상 선정 기준: 선택된 타일/대상 */
    Target UMETA(DisplayName = "Target"),

    /** 대상 선정 기준: 시전자와 타겟 모두 포함 */
    Both   UMETA(DisplayName = "Both")
};

/**
 * @brief 대상 선정 시 특정 객체를 제외하기 위한 필터(Bitmask)입니다.
 * 
 * @details
 * 영향 범위 안에 특정 유닛들은 제외하고 싶을 때 사용하는 열거형
 * 
 * @note 
 * 여러 옵션을 조합하여 사용 가능합니다. 예: ExcludeSelf | ExcludeAlly
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ETargetFilter : uint8
{
    /** 필터링을 적용하지 않습니다. */
    None = 0 UMETA(DisplayName = "None"),

    /** 시전자 본인을 대상에서 제외합니다. */
    ExcludeSelf = 1 << 0 UMETA(DisplayName = "Exclude Self"),

    /** 아군을 대상에서 제외합니다. */
    ExcludeAlly = 1 << 1 UMETA(DisplayName = "Exclude Ally"),

    /** 적군을 대상에서 제외합니다. */
    ExcludeEnemy = 1 << 2 UMETA(DisplayName = "Exclude Enemy")
};
ENUM_CLASS_FLAGS(ETargetFilter);


/**
 * @brief 방 종료 이유를 나타내는 열거형
 */
UENUM(BlueprintType)
enum class ERoomEndReason : uint8
{
    PlayerWin          UMETA(ToolTip = "플레이어 승리로 종료"),
    PlayerLose         UMETA(ToolTip = "플레이어 죽음으로 종료"),

    // @TODO 긴급 탈출 및 적 도망과 같은 로직을 추가할 경우, 항목 추가 필요
};

/**
 * @brief 방 내 진행 단계 열거형
 * @details
 * 설정한 단계에 따라서 프레임워크의 다음 로직이 결정
 */
UENUM(BlueprintType)
enum class ERoomPhase : uint8
{
    RoomInit           UMETA(ToolTip = "방 초기화"),
    RoomStart          UMETA(ToolTip = "방 시작"),
    RoomPlay           UMETA(ToolTip = "방 진행 중"),
    RoomEnd            UMETA(ToolTip = "방 종료")
};

/**
 * @brief 턴 종료 이유를 나타내는 열거형
 */
UENUM(BlueprintType)
enum class ESRPGTurnEndReason : uint8
{
    TurnFinish          UMETA(ToolTip = "가능한 모든 행동 수행 이후, 정상 종료"),
    TurnAbort           UMETA(ToolTip = "강제적으로 능력에 의해 종료"),
    OwnerDeath          UMETA(ToolTip = "턴의 소유권을 가진 유닛 죽음으로 인한 종료"),
    TeamDeath           UMETA(ToolTip = "특정 팀 소멸으로 인한 종료"),
};

/**
 * @brief 턴 내 진행 단계 열거형
 * @details
 * 설정한 단계에 따라서 턴의 다음 로직이 결정
 */
UENUM(BlueprintType)
enum class ESRPGTurnPhase : uint8
{
    TurnStart           UMETA(ToolTip = "턴 시작"),
    ActionSelect        UMETA(ToolTip = "액션 선택 중"),
    ActionPlay          UMETA(ToolTip = "액션 진행 중"),
    TurnEnd             UMETA(ToolTip = "턴 종료"),
};

UENUM(BlueprintType)
enum class ESRPGActionResult : uint8
{
    Finish              UMETA(ToolTip = "액션 정상 종료"),
    Abort               UMETA(ToolTip = "액션 중지"),
    Ongoing             UMETA(ToolTip = "액션 진행 중"),
};

