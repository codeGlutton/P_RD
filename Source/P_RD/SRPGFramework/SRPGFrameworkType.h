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
};

/**
 * @brief 방 종료 이유를 나타내는 열거형
 */
UENUM(BlueprintType)
enum class ESRPGRoomEndReason : uint8
{
    PlayerWin          UMETA(ToolTip = "플레이어 승리로 종료"),
    PlayerLose         UMETA(ToolTip = "플레이어 죽음으로 종료"),

    // @TODO 긴급 탈출 및 적 도망과 같은 로직을 추가할 경우, 항목 추가 필요
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
 * @brief 방 내 진행 단계 열거형
 * @details
 * 설정한 단계에 따라서 프레임워크의 다음 로직이 결정
 */
UENUM(BlueprintType)
enum class ESRPGRoomPhase : uint8
{
    RoomStart          UMETA(ToolTip = "방 시작"),
    RoomPlay           UMETA(ToolTip = "방 진행 중"),
    RoomEnd            UMETA(ToolTip = "방 종료")
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

