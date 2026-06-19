#include "Dice/DiceView.h"

#include "Dice/DiceModel.h"

void UDiceView::BindModel(UDiceModel* InModel)
{
	mModel = InModel;
	RefreshRarityVisual();
}

int32 UDiceView::GetResultValue() const
{
	return mModel.IsValid() ? mModel->GetCurrentValue() : 0;
}

int32 UDiceView::GetFaceCount() const
{
	return mModel.IsValid() ? mModel->GetFaceCount() : 0;
}

int32 UDiceView::GetRolledFaceIndex() const
{
	return mModel.IsValid() ? mModel->GetRolledFaceIndex() : INDEX_NONE;
}

bool UDiceView::IsRolled() const
{
	return mModel.IsValid() && mModel->IsRolled();
}

bool UDiceView::IsUsed() const
{
	return mModel.IsValid() && mModel->IsUsed();
}

/**
 * @details 희귀도 enum → UI 표시용 색/문구 변환을 여기서 한 번 처리한다.
 * 이렇게 두면 위젯은 ERarityType을 직접 몰라도 되고, 색 체계가 바뀌어도 이 함수만 고치면 된다(시각 책임 일원화).
 */
void UDiceView::RefreshRarityVisual()
{
	const ERarityType Rarity = mModel.IsValid() ? mModel->GetRarity() : ERarityType::Common;
	switch (Rarity)
	{
	case ERarityType::Rare:
		mRarityColor = FLinearColor(0.55f, 0.72f, 1.0f, 1.0f);
		mRarityText = FText::FromString(TEXT("Rare"));
		break;
	case ERarityType::Epic:
		mRarityColor = FLinearColor(0.82f, 0.58f, 1.0f, 1.0f);
		mRarityText = FText::FromString(TEXT("Epic"));
		break;
	case ERarityType::Common:
	default:
		mRarityColor = FLinearColor(0.86f, 0.98f, 0.94f, 1.0f);
		mRarityText = FText::FromString(TEXT("Common"));
		break;
	}
}
