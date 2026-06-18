#include "UI/Inventory/InventoryViewModel.h"

/**
 * @details UI가 항목을 길게 눌렀다는 의도만 구독자(게임플레이)에게 중계한다.
 * 상세 패널을 무엇으로 어떻게 띄울지는 게임플레이가 결정한다 — ViewModel은 순수 신호만 보낸다.
 */
void UInventoryViewModel::RequestItemLongPress(EInventoryItemKind Kind, int32 ItemIndex)
{
	OnItemLongPress.Broadcast(Kind, ItemIndex);
}

/**
 * @details 게임플레이/어댑터가 모은 런 상태 스냅샷을 저장하고 변경 알림을 쏜다.
 * 위젯은 OnViewChanged를 받아 GetInventory()를 읽어 목록을 다시 그린다.
 */
void UInventoryViewModel::SetInventory(const FInventoryView& Inventory)
{
	mInventory = Inventory;
	OnViewChanged.Broadcast();
}
