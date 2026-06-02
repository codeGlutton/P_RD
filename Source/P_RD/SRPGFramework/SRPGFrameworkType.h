/*****************************************************************//**
 * @file   SRPGFrameworkType.h
 * @brief  SRPG 프레임워크에서 사용되는 타입 정의 헤더
 * @author 모호재
 * @date   2026-04-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFrameworkType.generated.h"

UENUM(BlueprintType)
enum class ETileRotation : uint8
{
    Forward = 0,
    Right,
    Backward,
    Left
};

/**
 * @brief 타일 맵 상 위치
 */
USTRUCT(BlueprintType)
struct FTileTransform
{
    GENERATED_BODY()

public:
    UPROPERTY(Category = "TileTransform", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Direction"))
    ETileRotation mDirection = ETileRotation::Forward;
    UPROPERTY(Category = "TileTransform", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "IndexX"))
    int32 mIndexX = 0;
    UPROPERTY(Category = "TileTransform", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "IndexY"))
    int32 mIndexY = 0;
    
public:
    static const FTileTransform Zero;
};

inline const FTileTransform FTileTransform::Zero = FTileTransform();

/**
 * @brief 전투 결과를 나타내는 열거형
 */
UENUM(BlueprintType)
enum class ECombatResult : uint8
{
    PlayerWin          UMETA(ToolTip = "플레이어 승리로 종료"),
    PlayerLose         UMETA(ToolTip = "플레이어 죽음으로 종료"),

    // @TODO 긴급 탈출 및 적 도망과 같은 로직을 추가할 경우, 항목 추가 필요
};

/**
 * @brief 전투 방 내 진행 단계 열거형
 */
UENUM(BlueprintType)
enum class ECombatRoomPhase : uint8
{
    None               UMETA(Hidden),
    CombatInit         UMETA(ToolTip = "전투 초기화"),
    CombatStart        UMETA(ToolTip = "전투 시작"),
    CombatPlay         UMETA(ToolTip = "전투 진행 중"),
    CombatAbort        UMETA(ToolTip = "전투 진행 중단"),
    CombatEnd          UMETA(ToolTip = "전투 종료"),
};

/**
 * @brief 턴 결과를 나타내는 열거형
 */
UENUM(BlueprintType)
enum class ESRPGTurnResult : uint8
{
    Succeeded          UMETA(ToolTip = "정상 종료"),
    Cancelled          UMETA(ToolTip = "중단"),
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
    ActionSelect        UMETA(ToolTip = "액션 선택 중"),
    ActionPlay          UMETA(ToolTip = "액션 진행 중"),
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
enum class ESRPGActionPhase : uint8
{
    None                UMETA(Hidden),
    ActionInit          UMETA(ToolTip = "액션 초기화"),
    ActionStart         UMETA(ToolTip = "액션 시작"),
    ActionPlay          UMETA(ToolTip = "액션 진행 중"),
    ActionAbort         UMETA(ToolTip = "액션 진행 중단"),
    ActionEnd           UMETA(ToolTip = "액션 종료"),
};