#include "UI/Inventory/MockInventoryDriver.h"

#include "UI/Inventory/InventoryViewModel.h"

namespace
{
	// 희귀도 색은 어댑터 책임이지만, mock은 표시 검증용으로 직접 색을 박는다(Common/Rare/Epic).
	FLinearColor MockRarityColor(int32 Tier)
	{
		switch (Tier % 3)
		{
		case 1:  return FLinearColor(0.55f, 0.72f, 1.0f, 1.0f);   // Rare
		case 2:  return FLinearColor(0.82f, 0.58f, 1.0f, 1.0f);   // Epic
		default: return FLinearColor(0.86f, 0.98f, 0.94f, 1.0f);  // Common
		}
	}

	// 종류/이름/희귀도/보조문구로 mock 항목 한 칸을 만든다.
	FInventoryItemView MakeItem(EInventoryItemKind Kind, int32 Index, const FString& Name, const FString& Detail)
	{
		FInventoryItemView Item;
		Item.mKind = Kind;
		Item.mItemIndex = Index;
		Item.mName = FText::FromString(Name);
		Item.mDetailText = FText::FromString(Detail);
		Item.mRarityColor = MockRarityColor(Index);
		return Item;
	}
}

/**
 * @details 게임플레이 어댑터가 붙기 전, 표시 검증용 가짜 런 상태를 채워 push한다.
 * 보유 다이스 6 / 스킬 4 / 장비 3 + 메타(골드/레벨/체력)로 그리드·상단바 레이아웃을 확인한다.
 */
void UMockInventoryDriver::Start(UInventoryViewModel* ViewModel)
{
	if (ViewModel == nullptr)
	{
		return;
	}
	mViewModel = ViewModel;

	FInventoryView Inv;
	Inv.mGold = 120;
	Inv.mLevel = 3;
	Inv.mHP = 24.f;
	Inv.mMaxHP = 30.f;

	const int32 FaceCounts[] = { 6, 6, 4, 8, 12, 20 };
	for (int32 i = 0; i < 6; ++i)
	{
		Inv.mDice.Add(MakeItem(EInventoryItemKind::Dice, i, FString::Printf(TEXT("Dice %d"), i + 1), FString::Printf(TEXT("d%d"), FaceCounts[i])));
	}
	for (int32 i = 0; i < 4; ++i)
	{
		Inv.mSkills.Add(MakeItem(EInventoryItemKind::Skill, i, FString::Printf(TEXT("Skill %d"), i + 1), TEXT("주사위 1")));
	}
	for (int32 i = 0; i < 3; ++i)
	{
		Inv.mEquipment.Add(MakeItem(EInventoryItemKind::Equipment, i, FString::Printf(TEXT("Equip %d"), i + 1), TEXT("장착")));
	}

	mViewModel->SetInventory(Inv);
}
