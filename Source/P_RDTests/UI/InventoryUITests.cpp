/*****************************************************************//**
 * @file   InventoryUITests.cpp
 * @brief  파티 공용 골드와 용병별 EXP/획득 보상 표시 계약을 검증한다.
 *********************************************************************/

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UI/Inventory/InventoryUIModel.h"
#include "UI/Inventory/InventoryUIWidgetBase.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryPartyProgressSnapshotTest,
	"P_RD.UI.Inventory.PartyProgressSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryPartyProgressSnapshotTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드를 찾았다"), World))
	{
		return false;
	}

	UClass* InventoryWidgetClass = LoadClass<UInventoryUIWidgetBase>(
		nullptr, TEXT("/Game/UI/WBP_Inventory.WBP_Inventory_C"));
	if (!TestNotNull(TEXT("인벤토리 WBP 클래스를 찾았다"), InventoryWidgetClass))
	{
		return false;
	}

	UInventoryUIModel* Model = NewObject<UInventoryUIModel>(GetTransientPackage());
	UInventoryUIWidgetBase* Widget = CreateWidget<UInventoryUIWidgetBase>(
		World, InventoryWidgetClass);
	if (!TestNotNull(TEXT("인벤토리 위젯을 만들었다"), Widget))
	{
		return false;
	}

	UTexture2D* BackgroundArt = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/UI/Art/RunFlow/T_Inventory_Background_Current.T_Inventory_Background_Current"));
	if (TestNotNull(TEXT("현재 인벤토리 배경 아트를 찾았다"), BackgroundArt))
	{
		const FIntPoint ImportedSize = BackgroundArt->GetImportedSize();
		TestEqual(TEXT("배경 아트 가로 디자인 크기는 1672다"), ImportedSize.X, 1672);
		TestEqual(TEXT("배경 아트 세로 디자인 크기는 941이다"), ImportedSize.Y, 941);
	}
	TestTrue(TEXT("위젯 CDO 하드 참조로 배경 아트가 준비된다"), Widget->HasBackgroundArt());
	TestTrue(TEXT("아이콘 없는 획득품의 종류별 기본 아이콘이 준비된다"),
		Widget->HasFallbackItemIcons());

	FInventoryUI Inventory;
	Inventory.mGold = 275;

	const float ExpectedExp[] = { 15.f, 48.f, 91.f };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ExpectedExp); ++Index)
	{
		FInventoryMercenaryUI& Mercenary = Inventory.mMercenaries.AddDefaulted_GetRef();
		Mercenary.mPartyIndex = Index;
		Mercenary.mName = FText::FromString(
			FString::Printf(TEXT("Mercenary %d"), Index + 1));
		Mercenary.mLevel = Index + 2;
		Mercenary.mExp = ExpectedExp[Index];
		Mercenary.mMaxExp = 100.f + 20.f * Index;
		Mercenary.mHP = 20.f + Index;
		Mercenary.mMaxHP = 30.f;
	}

	FInventoryItemUI Skill;
	Skill.mKind = EInventoryItemKind::Skill;
	Skill.mItemIndex = 0;
	Skill.mName = FText::FromString(TEXT("Reward Skill"));
	Inventory.mSkills.Add(Skill);
	// 동일 스킬 획득이 허용되므로 화면 스냅샷에서도 중복을 제거하지 않는다.
	Skill.mItemIndex = 1;
	Inventory.mSkills.Add(Skill);

	FInventoryItemUI Equipment;
	Equipment.mKind = EInventoryItemKind::Equipment;
	Equipment.mItemIndex = 0;
	Equipment.mName = FText::FromString(TEXT("Reward Equipment"));
	Inventory.mEquipment.Add(Equipment);

	FInventoryItemUI Artifact;
	Artifact.mKind = EInventoryItemKind::Artifact;
	Artifact.mItemIndex = 0;
	Artifact.mName = FText::FromString(TEXT("Party Artifact"));
	Inventory.mArtifacts.Add(Artifact);

	Model->SetInventory(Inventory);
	Widget->BindUIModel(Model);
	const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();

	TestNotNull(TEXT("1672x941 fallback 디자인 캔버스를 생성한다"),
		Widget->GetWidgetFromName(TEXT("InventoryDesignCanvas")));
	TestNotNull(TEXT("배경 아트를 실제 위젯 트리에 배치한다"),
		Widget->GetWidgetFromName(TEXT("InventoryFallbackBackground")));
	TestNotNull(TEXT("모바일 닫기 터치 영역을 생성한다"),
		Widget->GetWidgetFromName(TEXT("InventoryRuntimeCloseButton")));

	TestEqual(TEXT("파티 공용 골드를 그대로 표시한다"), Widget->GetDisplayedGold(), 275);
	TestEqual(TEXT("용병 세 명을 각각 한 행으로 유지한다"), Widget->GetMercenaryRowCount(), 3);
	TestEqual(TEXT("획득 보상 중복을 포함해 네 개를 유지한다"), Widget->GetAcquiredItemCount(), 4);

	const FInventoryUI& Snapshot = Model->GetInventory();
	TestEqual(TEXT("첫 번째 용병 EXP가 독립적으로 유지된다"),
		Snapshot.mMercenaries[0].mExp, ExpectedExp[0]);
	TestEqual(TEXT("두 번째 용병 EXP가 독립적으로 유지된다"),
		Snapshot.mMercenaries[1].mExp, ExpectedExp[1]);
	TestEqual(TEXT("세 번째 용병 EXP가 독립적으로 유지된다"),
		Snapshot.mMercenaries[2].mExp, ExpectedExp[2]);
	TestEqual(TEXT("같은 스킬 두 개도 둘 다 보인다"), Snapshot.mSkills.Num(), 2);

	return true;
}

#endif
