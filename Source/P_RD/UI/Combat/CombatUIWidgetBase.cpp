#include "UI/Combat/CombatUIWidgetBase.h"

#include "UI/Combat/CombatUIModel.h"

/** @brief 새 UIModel을 구독하고 이미 들어온 스냅샷을 전체 갱신으로 한 번 그린다. */
void UCombatUIWidgetBase::BindUIModel(UCombatUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}

	UnbindUIModel();
	mUIModel = InUIModel;

	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &UCombatUIWidgetBase::HandleUIChanged);
		mUIModel->OnQueueNodeResolved.AddDynamic(this, &UCombatUIWidgetBase::HandleQueueNodeResolved);

		// 연결 직후 한 번 전체 갱신을 호출해 초기 상태를 그린다.
		OnUIRefreshed(ECombatUIDomain::All);
	}
}

/** @brief 현재 UIModel의 도메인/큐 델리게이트 구독을 해제한다. */
void UCombatUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &UCombatUIWidgetBase::HandleUIChanged);
		mUIModel->OnQueueNodeResolved.RemoveDynamic(this, &UCombatUIWidgetBase::HandleQueueNodeResolved);
	}
	mUIModel = nullptr;
}

/** @brief 도메인별 갱신 알림을 WBP 구현 이벤트로 전달한다. */
void UCombatUIWidgetBase::HandleUIChanged(ECombatUIDomain Domain)
{
	OnUIRefreshed(Domain);
}

/** @brief 큐 resolved 노드를 WBP 재생 이벤트로 전달한다. */
void UCombatUIWidgetBase::HandleQueueNodeResolved(FCombatQueueNode Node)
{
	OnQueueNodePlayed(Node);
}

/** @brief 위젯 파괴 전에 UIModel 구독을 끊어 dangling delegate 호출을 막는다. */
void UCombatUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}
