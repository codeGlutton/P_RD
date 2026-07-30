#include "UI/Inventory/InventoryUIModel.h"

/**
 * @details 게임플레이/어댑터가 모은 런 상태 스냅샷을 저장하고 변경 알림을 쏜다.
 * 위젯은 OnUIChanged를 받아 GetInventory()를 읽어 목록을 다시 그린다.
 */
void UInventoryUIModel::SetInventory(const FInventoryUI& Inventory)
{
	mInventory = Inventory;
	OnUIChanged.Broadcast();
}
