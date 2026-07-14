#include "UI/CombatTileMapHUDWidget.h"

// 장비 슬롯 롱프레스 감지 — 스킬 레일과 동일한 "누름 시간 누적" 방식.
// 장비 아이콘은 WBP 마커(HUD_M_equip_*, UImage)라 입력을 못 받으므로, 마커 위치에 얹은 투명
// UIndexedButtonWidget(mEquipSlotButtons)의 press/release로 감지한다. 탭은 무동작, 롱프레스만 상세를 연다.

void UCombatTileMapHUDWidget::HandleEquipButtonPressed(int32 SlotIndex)
{
	BeginEquipPress(SlotIndex);
}

void UCombatTileMapHUDWidget::HandleEquipButtonReleased()
{
	// 장비는 탭 동작이 없다(롱프레스만 상세). 누름 상태만 리셋.
	mEquipPressing = false;
	mPressedEquipSlot = INDEX_NONE;
	mEquipPressElapsed = 0.0f;
	mEquipDetailOpenedFromPress = false;
}

void UCombatTileMapHUDWidget::BeginEquipPress(int32 SlotIndex)
{
	// 이미 다른 손가락이 누르는 중이면 무시한다(먼저 누른 쪽 우선 — 스킬 레일과 동일 규칙).
	if (mEquipPressing == true)
	{
		return;
	}

	mPressedEquipSlot = SlotIndex;
	mEquipPressing = true;
	mEquipDetailOpenedFromPress = false;
	mEquipPressElapsed = 0.0f;
}

void UCombatTileMapHUDWidget::UpdateEquipPress(float InDeltaTime)
{
	if (mEquipPressing == false || mEquipDetailOpenedFromPress == true)
	{
		return;
	}

	mEquipPressElapsed += InDeltaTime;
	if (mEquipPressElapsed >= mSkillLongPressThreshold)   // 스킬과 동일 임계로 손맛 통일.
	{
		ShowEquipmentDetail(mPressedEquipSlot);
		mEquipDetailOpenedFromPress = true;
	}
}
