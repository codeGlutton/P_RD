/*****************************************************************//**
 * @file   SRPGCommand.h
 * @brief  SRPG의 사용자 명령 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGCommand.generated.h"

class USRPGAction;

struct FEquippedEntry;
class IBoardSelectionTargetView;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnShowTargetDetailPanelUI, IBoardSelectionTargetView* /*Target*/);

/**
 * @brief 톡 친 칸이 어디였는지 알린다.
 *
 * 트레이스는 이 커맨드를 다루는 쪽이 한다. 화면은 좌표만 넘기고 타일이 뭔지
 * 모르므로, 풀어낸 결과를 이 길로 돌려보내야 UI 가 겨냥한 자리를 안다.
 * 액터는 그 칸에 선 것이고, 빈 칸이면 nullptr 이다.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSelectTargetTile, const FTileIndex& /*Tile*/, AActor* /*Actor*/);

/**
 * @brief  사용자 입력 명령 객체
 */
USTRUCT(BlueprintType)
struct P_RD_API FSRPGCommand
{
	GENERATED_BODY()

public:
	ESRPGCommandType GetCommandType() const;

protected:
	UPROPERTY(Category = Command, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CommandType"))
	ESRPGCommandType mCommandType = ESRPGCommandType::None;

public:
	UPROPERTY(Category = Action, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RequestedAction"))
	TSubclassOf<USRPGAction> mRequestedAction = nullptr;
};

/**
 * @brief  사용자 월드 입력 명령 객체
 */
USTRUCT(BlueprintType)
struct FSRPGWorldTraceCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGWorldTraceCommand();

public:
	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "IsLongPress"))
	bool mIsLongPress = false;

	/**
	 * @brief 입력 지점의 화면 좌표(픽셀).
	 * @details 모바일은 마우스 커서가 없어 커서 트레이스가 실패하므로, 이 좌표로 직접 월드 트레이스한다.
	 *          (-1, -1) = 미지정 → PC 마우스 커서를 사용한다.
	 */
	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ScreenPosition"))
	FVector2D mScreenPosition = FVector2D(-1.0, -1.0);

	/**
	 * @brief 이미 게임플레이가 확정한 타일. Invalid이면 화면 좌표를 트레이스한다.
	 * @details 확정 버튼처럼 커서가 UI 위에 있는 입력은 다시 hit-test하면 버튼
	 *          뒤의 엉뚱한 월드를 집을 수 있다. 그런 경우 선택 단계에서 보관한
	 *          타일을 직접 전달해 동일한 WorldTrace 규칙만 재사용한다.
	 */
	UPROPERTY(Category = Input, EditAnywhere, BlueprintReadWrite,
		meta = (DisplayName = "ResolvedTileIndex"))
	FTileIndex mResolvedTileIndex = FTileIndex::Invalid;

public:
	FOnShowTargetDetailPanelUI OnShowTargetDetailPanelUI;

	/** @brief 톡 쳐서 고른 칸. 롱프레스가 아닌 보통 탭에서 온다. */
	FOnSelectTargetTile OnSelectTargetTile;
};

