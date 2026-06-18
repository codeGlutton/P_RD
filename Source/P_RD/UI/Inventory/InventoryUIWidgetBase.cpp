#include "UI/Inventory/InventoryUIWidgetBase.h"
#include "UI/Inventory/InventoryUIModel.h"

/** @brief 새 UIModel을 구독하고 이미 들어온 인벤토리 스냅샷도 즉시 한 번 그린다. */
void UInventoryUIWidgetBase::BindUIModel(UInventoryUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}
	UnbindUIModel();
	mUIModel = InUIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &UInventoryUIWidgetBase::HandleUIChanged);
		// 인벤토리 데이터가 BindUIModel보다 먼저 들어온 경우도 있으므로 연결 직후 한 번 그려 초기 상태를 표시한다.
		OnInventoryRefreshed();
	}
}

/** @brief WBP의 항목 롱프레스 입력을 UIModel의 상세 요청 의도 이벤트로 전달한다. */
void UInventoryUIWidgetBase::LongPressItem(EInventoryItemKind Kind, int32 ItemIndex)
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestItemLongPress(Kind, ItemIndex);
	}
}

/** @brief 현재 UIModel 구독을 해제해 화면 파괴 후 OnUIChanged가 들어오지 않게 한다. */
void UInventoryUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &UInventoryUIWidgetBase::HandleUIChanged);
	}
	mUIModel = nullptr;
}

/** @brief UIModel 변경 알림을 WBP 구현 이벤트로 변환한다. */
void UInventoryUIWidgetBase::HandleUIChanged()
{
	OnInventoryRefreshed();
}

/** @brief 위젯 생명주기 종료 시 UIModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void UInventoryUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}
