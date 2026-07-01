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
    FTileIndex() = default;
    FTileIndex(int32 InX, int32 InY) : mX(InX), mY(InY) {}

public:
    bool operator==(const FTileIndex& Other) const
    {
        return mX == Other.mX && mY == Other.mY;
    }

    bool operator!=(const FTileIndex& Other) const
    {
        return (*this == Other) == false;
    }

    friend uint32 GetTypeHash(const FTileIndex& Tile)
    {
        return HashCombine(GetTypeHash(Tile.mX), GetTypeHash(Tile.mY));
    }

    // @brief 가로(X) 방향 인덱스
    UPROPERTY(Category = "TileIndex", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "X"))
    int32 mX = 0;
    // @brief 세로(Y) 방향 인덱스
    UPROPERTY(Category = "TileIndex", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Y"))
    int32 mY = 0;

    // @brief 유효하지 않은(미배치) 타일을 나타내는 초기/해제값
    static const FTileIndex Invalid;
};

// @brief 무효 좌표 정의: (-1, -1)
inline const FTileIndex FTileIndex::Invalid = FTileIndex(-1, -1);

/**
 * @brief 타일 맵 상 위치
 */
USTRUCT(BlueprintType)
struct FTileTransform
{
    GENERATED_BODY()

public:
    FTileTransform() = default;
    FTileTransform(const FTileIndex& InIndex, ETileActorDirection InDirection = ETileActorDirection::Forward)
        : mIndex(InIndex), mDirection(InDirection) {}

    // @brief 타일 인덱스 좌표
    UPROPERTY(Category = "TileTransform", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Index"))
    FTileIndex mIndex;
    // @brief 액터가 바라보는 방향
    UPROPERTY(Category = "TileTransform", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Direction"))
    ETileActorDirection mDirection = ETileActorDirection::Forward;

    // @brief 유효하지 않은(미배치) 트랜스폼을 나타내는 초기/해제값
    static const FTileTransform Invalid;
};

// @brief 무효 트랜스폼 정의: 인덱스가 무효
inline const FTileTransform FTileTransform::Invalid = FTileTransform(FTileIndex::Invalid);

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
    /** 아무도 대상에 포함하지 않습니다. (비어있음) */
    None = 0 UMETA(DisplayName = "None"),

    /** 시전자 본인을 대상에 포함합니다. */
    IncludeSelf = 1 << 0 UMETA(DisplayName = "Include Self"),

    /** 아군을 대상에 포함합니다. */
    IncludeAlly = 1 << 1 UMETA(DisplayName = "Include Ally"),

    /** 적군을 대상에 포함합니다. */
    IncludeEnemy = 1 << 2 UMETA(DisplayName = "Include Enemy"),

    /** 모든 대상을 포함합니다. (기본값으로 활용 가능) */
    All = IncludeSelf | IncludeAlly | IncludeEnemy UMETA(DisplayName = "All")
};
ENUM_CLASS_FLAGS(ETargetFilter);

/**
 * @brief 전투 결과를 나타내는 열거형
 */
UENUM(BlueprintType)
enum class ESRPGCombatResult : uint8
{
    PlayerWin          UMETA(ToolTip = "플레이어 승리로 종료"),
    PlayerLose         UMETA(ToolTip = "플레이어 죽음으로 종료"),

    // @TODO 긴급 탈출 및 적 도망과 같은 로직을 추가할 경우, 항목 추가 필요
};

/**
 * @brief 전투 방 내 진행 단계 열거형
 */
UENUM(BlueprintType)
enum class ESRPGCombatRoomPhase : uint8
{
    None                UMETA(Hidden),
    CombatInit          UMETA(ToolTip = "전투 초기화"),
    CombatStart         UMETA(ToolTip = "전투 시작"),
    CombatPlay          UMETA(ToolTip = "전투 진행 중"),
    CombatAbort         UMETA(ToolTip = "전투 진행 중단"),
    CombatEnd           UMETA(ToolTip = "전투 종료"),
};

/**
 * @brief 턴 결과를 나타내는 열거형
 */
UENUM(BlueprintType)
enum class ESRPGTurnResult : uint8
{
    Succeeded           UMETA(ToolTip = "정상 종료"),
    Cancelled           UMETA(ToolTip = "중단"),
};

/**
 * @brief 턴 내 진행 단계 열거형
 */
UENUM(BlueprintType)
enum class ESRPGTurnPhase : uint8
{
    None                UMETA(Hidden),
    TurnInit            UMETA(ToolTip = "턴 초기화"),
    TurnStart           UMETA(ToolTip = "턴 시작"),
    TurnPlay            UMETA(ToolTip = "턴 진행 중"),
    TurnAbort           UMETA(ToolTip = "턴 진행 중단"),
    TurnEnd             UMETA(ToolTip = "턴 종료"),
};

UENUM(BlueprintType)
enum class ESRPGActionResult : uint8
{
    Succeeded           UMETA(ToolTip = "액션 정상 종료"),
    Cancelled           UMETA(ToolTip = "액션 중지"),
};

/**
 * @brief 액션 내 진행 단계 열거형
 */
UENUM(BlueprintType)
enum class ESRPGActionType : uint8
{
    BuildAction         UMETA(ToolTip = "타 액션을 제작하는 액션"),
    InPlayAction        UMETA(ToolTip = "실제 게임 적용 액션"),
};

/**
 * @brief 액션 내 진행 단계 열거형
 */
UENUM(BlueprintType)
enum class ESRPGActionPhase : uint8
{
    None                UMETA(Hidden),
    ActionInit          UMETA(ToolTip = "액션 초기화"),
    ActionStart         UMETA(ToolTip = "액션 시작"),
    ActionPlay          UMETA(ToolTip = "액션 진행 중"),
    ActionAbort         UMETA(ToolTip = "액션 진행 중단"),
    ActionEnd           UMETA(ToolTip = "액션 종료"),
};

/**
 * @brief 스킬 빌드 액션 내 진행 단계 열거형
 */
UENUM(BlueprintType)
enum class ESRPGSkillBuildPhase : uint8
{
    None                UMETA(Hidden),
    AimSelection        UMETA(ToolTip = "대상 영역 선택"),
    Preview             UMETA(ToolTip = "프리뷰 표기"),
    Build               UMETA(ToolTip = "빌드 성공"),
};

/**
 * @brief 이동 빌드 액션 내 진행 단계 열거형
 */
UENUM(BlueprintType)
enum class ESRPGMoveBuildPhase : uint8
{
    None                UMETA(Hidden),
    DestSelection       UMETA(ToolTip = "도달 가능 목적지 선택"),
    Preview             UMETA(ToolTip = "이동 경로 프리뷰 표기"),
    Build               UMETA(ToolTip = "빌드 성공"),
};

/**
 * @brief SRPG 명령 타입에 대한 열거형
 */
UENUM(BlueprintType)
enum class ESRPGCommandType : uint8
{
    None                UMETA(Hidden),

    WorldTrace          UMETA(ToolTip = "월드 공간 선택"),

    DicePrepare         UMETA(ToolTip = "굴릴 주사위 준비"),
    DiceRoll            UMETA(ToolTip = "주사위 굴리기"),

    SkillSelect         UMETA(ToolTip = "사용 스킬 결정"),
    DiceSelect          UMETA(ToolTip = "사용 주사위 결정"),
    SkillCast           UMETA(ToolTip = "스킬 사용"),

    MoveSelect          UMETA(ToolTip = "이동 시작"),
    MoveCast            UMETA(ToolTip = "이동 사용"),

    TurnEnd             UMETA(ToolTip = "턴 종료"),
};

/**
 * @brief 명령 처리 여부를 나타내는 열거형
 */
UENUM(BlueprintType)
enum class ESRPGCommandResult : uint8
{
    Handled             UMETA(ToolTip = "처리되어 커맨드가 소비됨"),
    Continue            UMETA(ToolTip = "처리했으나 계속됨"),
    Ignored             UMETA(ToolTip = "전혀 처리하지 않음"),
};

/**
 * 액션 커맨드 결과를 결합하여, 반환해주는 함수.
 * 두 결과는 Handle > Continue > Ignore 순으로 한쪽으로 덮어 씌워짐
 * @param Lhs 계산 결과 A
 * @param Rhs 계산 결과 B
 * @return 최종 결과
 */
inline ESRPGCommandResult CombineSRPGCommandResult(ESRPGCommandResult Lhs, ESRPGCommandResult Rhs)
{
    if (static_cast<uint8>(Lhs) < static_cast<uint8>(Rhs))
    {
        return Lhs;
    }
    return Rhs;
}

