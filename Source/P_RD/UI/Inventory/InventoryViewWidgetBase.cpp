#include "UI/Inventory/InventoryViewWidgetBase.h"
#include "UI/Inventory/InventoryViewModel.h"

/** @brief 새 ViewModel을 구독하고 이미 들어온 인벤토리 스냅샷도 즉시 한 번 그린다. */
void UInventoryViewWidgetBase::BindViewModel(UInventoryViewModel* InViewModel)
{
	if (mViewModel == InViewModel)
	{
		return;
	}
	UnbindViewModel();
	mViewModel = InViewModel;
	if (mViewModel != nullptr)
	{
		mViewModel->OnViewChanged.AddDynamic(this, &UInventoryViewWidgetBase::HandleViewChanged);
		// 인벤토리 데이터가 BindViewModel보다 먼저 들어온 경우도 있으므로 연결 직후 한 번 그려 초기 상태를 표시한다.
		OnInventoryRefreshed();
	}
}

/** @brief WBP의 항목 롱프레스 입력을 ViewModel의 상세 요청 의도 이벤트로 전달한다. */
void UInventoryViewWidgetBase::LongPressItem(EInventoryItemKind Kind, int32 ItemIndex)
{
	if (mViewModel != nullptr)
	{
		mViewModel->RequestItemLongPress(Kind, ItemIndex);
	}
}

/** @brief 현재 ViewModel 구독을 해제해 화면 파괴 후 OnViewChanged가 들어오지 않게 한다. */
void UInventoryViewWidgetBase::UnbindViewModel()
{
	if (mViewModel != nullptr)
	{
		mViewModel->OnViewChanged.RemoveDynamic(this, &UInventoryViewWidgetBase::HandleViewChanged);
	}
	mViewModel = nullptr;
}

/** @brief ViewModel 변경 알림을 WBP 구현 이벤트로 변환한다. */
void UInventoryViewWidgetBase::HandleViewChanged()
{
	OnInventoryRefreshed();
}

/** @brief 위젯 생명주기 종료 시 ViewModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void UInventoryViewWidgetBase::NativeDestruct()
{
	UnbindViewModel();
	Super::NativeDestruct();
}
