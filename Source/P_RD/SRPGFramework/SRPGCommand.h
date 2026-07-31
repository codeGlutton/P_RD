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

public:
	FOnShowTargetDetailPanelUI OnShowTargetDetailPanelUI;
};

