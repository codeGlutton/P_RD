#include "UI/CombatTileMapHUDWidget.h"

#include "GameMode/CombatGameMode.h"
#include "UI/Combat/CombatUIModel.h"

/** @brief 스킬 버튼 해제 시 짧은 탭은 선택, 롱프레스는 전투 쪽 상세 요청 API가 생길 때까지 소비만 한다. */
void UCombatTileMapHUDWidget::HandleSkillButtonReleased()
{
	if (mSkillPressing == false)
	{
		return;
	}

	if (mSkillLongPressConsumed == false)
	{
		SelectSkillForAssignment(mPressedSkillIndex);
	}

	mSkillPressing = false;
	mPressedSkillIndex = INDEX_NONE;
	mSkillPressElapsed = 0.0f;
	mSkillLongPressConsumed = false;
}

/** @brief 스킬 레일 누름 상태로 진입한다. */
void UCombatTileMapHUDWidget::HandleSkillButtonPressed(int32 SkillIndex)
{
	BeginSkillPress(SkillIndex);
}

/** @brief 스킬 레일 입력의 시작점을 저장한다. INDEX_NONE은 히트 테스트 실패를 뜻하는 sentinel이다. */
void UCombatTileMapHUDWidget::BeginSkillPress(int32 SkillIndex)
{
	if (SkillIndex == INDEX_NONE)
	{
		return;
	}
	if (IsSkillSlotAvailable(SkillIndex) == false)
	{
		return;
	}

	mPressedSkillIndex = SkillIndex;
	mSkillPressing = true;
	mSkillLongPressConsumed = false;
	mSkillPressElapsed = 0.0f;
}

/** @brief NativeTick에서 누름 시간을 누적해 롱프레스 입력을 선택 탭과 분리한다. */
void UCombatTileMapHUDWidget::UpdateSkillPress(float InDeltaTime)
{
	if (mSkillPressing == false || mSkillLongPressConsumed == true)
	{
		return;
	}

	mSkillPressElapsed += InDeltaTime;
	if (mSkillPressElapsed >= mSkillLongPressThreshold)
	{
		mSkillLongPressConsumed = true;
		if (IsSkillSlotAvailable(mPressedSkillIndex))
		{
			if (ACombatGameMode* CombatGameMode = GetWorld()->GetAuthGameMode<ACombatGameMode>())
			{
				if (CombatGameMode->GetSkillDetail(mPressedSkillIndex) != nullptr)
				{
					ShowSkillDetailPanel(mPressedSkillIndex);
				}
			}
		}
	}
}

void UCombatTileMapHUDWidget::BeginWorldPress()
{
	mWorldPressing = true;
	mWorldLongPressConsumed = false;
	mWorldPressElapsed = 0.0f;
}

FReply UCombatTileMapHUDWidget::EndWorldPress(bool bReleaseMouseCapture)
{
	if (mWorldLongPressConsumed == false)
	{
		if (ACombatGameMode* CombatGameMode = GetWorld()->GetAuthGameMode<ACombatGameMode>())
		{
			CombatGameMode->ResolveWorldTouchEvent();
		}
	}

	mWorldPressing = false;
	mWorldLongPressConsumed = false;
	mWorldPressElapsed = 0.0f;

	return bReleaseMouseCapture
		? FReply::Handled().ReleaseMouseCapture()
		: FReply::Handled();
}

void UCombatTileMapHUDWidget::UpdateWorldPress(float InDeltaTime)
{
	if (mWorldPressing == false || mWorldLongPressConsumed == true)
	{
		return;
	}

	mWorldPressElapsed += InDeltaTime;
	if (mWorldPressElapsed >= mWorldLongPressThreshold)
	{
		mWorldLongPressConsumed = true;
		if (ACombatGameMode* CombatGameMode = GetWorld()->GetAuthGameMode<ACombatGameMode>())
		{
			CombatGameMode->ResolveWorldLongPressEvent();
		}
	}
}

/** @brief 스킬 선택은 UI 강조와 의도 전달까지만 담당하고, 스킬 유효성/실행은 전투 계층에 맡긴다. */
void UCombatTileMapHUDWidget::SelectSkillForAssignment(int32 SkillIndex)
{
	if (IsSkillSlotAvailable(SkillIndex) == false)
	{
		return;
	}

	if (mSelectedSkillIndex != SkillIndex)
	{
		mSelectedDiceIndex = INDEX_NONE;
	}
	mSelectedSkillIndex = SkillIndex;

	if (ACombatGameMode* CombatGameMode = GetWorld()->GetAuthGameMode<ACombatGameMode>())
	{
		CombatGameMode->SelectSkill(SkillIndex);
	}

	RefreshSkillRailWidgets();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}

bool UCombatTileMapHUDWidget::IsSkillSlotAvailable(int32 SkillIndex) const
{
	if (SkillIndex == INDEX_NONE)
	{
		return false;
	}

	if (mCombatUIModel == nullptr)
	{
		return true;
	}

	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	return Skills.IsValidIndex(SkillIndex) && Skills[SkillIndex].mIsUsable;
}
