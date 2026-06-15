/*****************************************************************//**
 * @file   SRPGSkillBuildCommand.h
 * @brief  스킬 생성 액션 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"

struct FSRPGSkillBuildAction;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangeSkillBuildPhase, const FSRPGSkillBuildAction& /*Action*/, ESRPGSkillBuildPhase /*Phase*/);

class UStaticSkillData;

struct FSRPGSkillSelectCommand : public FSRPGActionCreationCommand<FSRPGSkillBuildAction>
{
public:
	FSRPGSkillSelectCommand();

public:
	FOnChangeSkillBuildPhase OnChangeSkillBuildPhase;

public:
	int32 mSkillIndex = 0;
};

struct FSRPGCDiceSelectCommand : public FSRPGActionCommand
{
public:
	FSRPGCDiceSelectCommand();

public:
	int32 mDiceIndex = 0;
};

/**
 * @brief  스킬 생성 액션 객체
 */
struct FSRPGSkillBuildAction : public FSRPGAction
{
	template<typename ActionType>
	friend struct FSRPGActionCreationCommand;
	using Super = FSRPGAction;

protected:
	FSRPGSkillBuildAction();

	/* FSRPGAction 상속 */
protected:
	void OnBeginAction() override;
	void OnEndAction() override;

protected:
	ESRPGActionCommandResult CanHandleCommand(TSharedPtr<const FSRPGActionCommand> Command) const override;
	void ApplyCommand(TSharedPtr<const FSRPGActionCommand> Command) override;

protected:
	void ApplyWorldTraceCommand(TSharedPtr<const FSRPGActionCommand> Command);

	/* 빌드 로직 처리 */
private:
	void ChangeDices(int32 RequestedDiceIndex);

	void SetSkill(int32 SkillIndex);
	void ResetSkill();
	void SetTargetTile(const FTileIndex& TileIndex);
	void ResetTargetTile();
	void BuildSkill();

private:
	void SetBuildPhase(ESRPGSkillBuildPhase BuildPhase);

public:
	// const TArray<FSRPGCalculatedData>& GetCalculateDatas() const;

protected:
	FOnChangeSkillBuildPhase OnChangeSkillBuildPhase;

protected:
	ESRPGSkillBuildPhase mSkillBuildPhase = ESRPGSkillBuildPhase::None;

protected:
	TArray<FTileIndex> mAimTileIndexes;
	TObjectPtr<UStaticSkillData> mSelectedSkill;
	int32 mSelectedSkillIndex = INDEX_NONE;

	TArray<int32> mSelectedDices;
	int32 mSelectedDiceSum = 0;

	TArray<FTileIndex> mEffectTileIndexes;
	FTileIndex mTargetIndex = FTileIndex::Invalid;
	// TArray<FSRPGCalculatedData> mCalculateDatas;
};

