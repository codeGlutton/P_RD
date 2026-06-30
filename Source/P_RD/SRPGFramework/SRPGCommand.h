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
class IBoardSelectionTarget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnShowTargetDetailPanelUI, IBoardSelectionTarget* /*Target*/);


/**
 * @brief  사용자 입력 명령 객체
 */
USTRUCT(BlueprintType)
struct FSRPGCommand
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

public:
	FOnShowTargetDetailPanelUI OnShowTargetDetailPanelUI;
};

