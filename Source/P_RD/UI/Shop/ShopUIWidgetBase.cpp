#include "UI/Shop/ShopUIWidgetBase.h"
#include "UI/Shop/ShopUIModel.h"

/** @brief 새 UIModel을 구독하고 이미 들어온 상점 스냅샷도 즉시 한 번 그린다. */
void UShopUIWidgetBase::BindUIModel(UShopUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}
	UnbindUIModel();
	mUIModel = InUIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &UShopUIWidgetBase::HandleUIChanged);
		// 상점 데이터가 BindUIModel보다 먼저 들어온 경우도 있으므로 연결 직후 한 번 그린다.
		OnShopRefreshed();
	}
}

/** @brief WBP의 슬롯 구매 입력을 UIModel의 구매 의도 이벤트로 전달한다. */
void UShopUIWidgetBase::BuyItem(int32 SlotIndex)
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestBuy(SlotIndex);
	}
}

/** @brief WBP의 나가기 입력을 UIModel의 나가기 의도 이벤트로 전달한다. */
void UShopUIWidgetBase::Leave()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestLeave();
	}
}

/** @brief 현재 UIModel 구독을 해제해 화면 파괴 후 OnUIChanged가 들어오지 않게 한다. */
void UShopUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &UShopUIWidgetBase::HandleUIChanged);
	}
	mUIModel = nullptr;
}

/** @brief UIModel 변경 알림을 WBP 구현 이벤트로 변환한다. */
void UShopUIWidgetBase::HandleUIChanged()
{
	OnShopRefreshed();
}

/** @brief 위젯 생명주기 종료 시 UIModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void UShopUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}
