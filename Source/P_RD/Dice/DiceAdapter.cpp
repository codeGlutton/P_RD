#include "Dice/DiceAdapter.h"

#include "Dice/DiceData.h"
#include "UI/Combat/CombatViewModel.h"
#include "UI/Combat/CombatViewTypes.h"
#include "UI/DiceViewData.h"

void UDiceAdapter::BuildFromDiceIds(const TArray<FPrimaryAssetId>& DiceIds)
{
	mDice.Reset();

	for (const FPrimaryAssetId& DiceId : DiceIds)
	{
		if (DiceId.IsValid() == false)
		{
			continue;
		}

		// 희귀도는 RDUIDice가 로드된 에셋 or id 이름으로 해석해 준다(미로드여도 안전).
		const ERarityType Rarity = RDUIDice::ResolveDiceRarity(DiceId);

		UDiceData* Dice = NewObject<UDiceData>(this);
		Dice->Init(Rarity, DiceId);
		mDice.Add(Dice);
	}

	PushDiceViews();
}

void UDiceAdapter::BindViewModel(UCombatViewModel* InViewModel)
{
	if (mViewModel == InViewModel)
	{
		return;
	}

	if (mViewModel != nullptr)
	{
		mViewModel->OnCombatCommand.RemoveDynamic(this, &UDiceAdapter::HandleCombatCommand);
	}

	mViewModel = InViewModel;

	if (mViewModel != nullptr)
	{
		mViewModel->OnCombatCommand.AddUniqueDynamic(this, &UDiceAdapter::HandleCombatCommand);
		PushDiceViews();
	}
}

void UDiceAdapter::RollAll(const FRandomStream& Stream)
{
	for (const TObjectPtr<UDiceData>& Dice : mDice)
	{
		if (Dice != nullptr)
		{
			Dice->Roll(Stream);
		}
	}

	PushDiceViews();
}

void UDiceAdapter::HandleCombatCommand(ECombatInputType Type, int32 IntPayload)
{
	if (Type != ECombatInputType::RollDice)
	{
		// 선택(ToggleDice) 등 나머지 의도는 데이터 계층 통합 단계에서 처리한다.
		return;
	}

	/*
	 * 어댑터 단독 단계라 임시 난수 스트림을 쓴다.
	 * 게임플레이/런 통합 시 run의 RandomStream(URandomStreamFunctionLibrary)을 주입해 결정론을 맞춘다.
	 */
	FRandomStream Stream;
	Stream.GenerateNewSeed();
	RollAll(Stream);
}

void UDiceAdapter::PushDiceViews() const
{
	if (mViewModel == nullptr)
	{
		return;
	}

	TArray<FDiceSlotView> Views;
	Views.Reserve(mDice.Num());

	for (const TObjectPtr<UDiceData>& Dice : mDice)
	{
		if (Dice == nullptr)
		{
			continue;
		}

		FDiceSlotView View;
		View.mDiceId = Dice->GetSourceDiceId();
		View.mResultValue = Dice->GetCurrentValue();
		View.mIsRolled = Dice->IsRolled();
		View.mRarityColor = RDUIDice::GetDiceRarityColor(Dice->GetRarity());
		View.mRarityText = RDUIDice::GetDiceRarityText(Dice->GetRarity());
		Views.Add(MoveTemp(View));
	}

	mViewModel->SetDiceViews(Views);
}
