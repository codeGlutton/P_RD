/*****************************************************************//**
 * @file   SRPGDiceRollAction.h
 * @brief  주사위 굴리기 SRPG 행동 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"
#include "SRPGDiceRollAction.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnShowDicePanelUI);

USTRUCT()
struct FSRPGDicePrepareCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGDicePrepareCommand();

public:
	FOnShowDicePanelUI OnShowDicePanelUI;
};

USTRUCT()
struct FSRPGDiceRollCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGDiceRollCommand();

	/** @brief 물리 굴림 연출이 확정한 면 index(0-base, 주사위 순서). 비어 있으면 내부 난수로 굴린다. */
	UPROPERTY(Category = Roll, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RolledFaceIndices"))
	TArray<int32> mRolledFaceIndices;
};

/**
 * @brief  주사위 굴리기 SRPG 행동 객체
 */
UCLASS()
class USRPGDiceRollAction : public USRPGAction
{
	GENERATED_BODY()

protected:
	USRPGDiceRollAction();

protected:
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;
};

