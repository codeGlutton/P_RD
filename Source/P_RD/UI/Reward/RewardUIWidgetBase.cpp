#include "UI/Reward/RewardUIWidgetBase.h"

#include "UI/Reward/RewardUIModel.h"

/** @brief 새 UIModel을 구독하고 이미 들어온 보상 스냅샷도 즉시 한 번 그린다. */
void URewardUIWidgetBase::BindUIModel(URewardUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}

	UnbindUIModel();
	mUIModel = InUIModel;

	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &URewardUIWidgetBase::HandleUIChanged);
		mUIModel->OnChoicesChanged.AddDynamic(this, &URewardUIWidgetBase::HandleChoicesChanged);

		// 보상 데이터가 BindUIModel보다 먼저 들어온 경우도 있으므로 연결 직후 한 번 그려 초기 상태를 표시한다.
		OnRewardRefreshed();
		OnRewardChoicesRefreshed();
	}
}

/** @brief WBP의 선택지 카드 입력을 UIModel의 선택 의도 이벤트로 전달한다. */
void URewardUIWidgetBase::ChooseReward(int32 ChoiceIndex)
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestChooseReward(ChoiceIndex);
	}
}

/** @brief WBP의 받기 버튼 입력을 UIModel의 Claim 의도 이벤트로 전달한다. */
void URewardUIWidgetBase::Claim()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestClaim();
	}
}

/** @brief 현재 UIModel 구독을 해제해 화면 파괴 후 OnUIChanged가 들어오지 않게 한다. */
void URewardUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &URewardUIWidgetBase::HandleUIChanged);
		mUIModel->OnChoicesChanged.RemoveDynamic(this, &URewardUIWidgetBase::HandleChoicesChanged);
	}
	mUIModel = nullptr;
}

/** @brief UIModel 변경 알림을 WBP 구현 이벤트로 변환한다. */
void URewardUIWidgetBase::HandleUIChanged()
{
	OnRewardRefreshed();
}

/** @brief 선택지 변경 알림을 WBP 선택지 구현 이벤트로 변환한다. */
void URewardUIWidgetBase::HandleChoicesChanged()
{
	OnRewardChoicesRefreshed();
}

/** @brief 위젯 생명주기 종료 시 UIModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void URewardUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}
