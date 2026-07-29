/*****************************************************************//**
 * @file InventoryUITests.cpp
 * @brief 파티 공용 골드 + 아티팩트 인벤토리 계약을 검증한다.
 *********************************************************************/

#include "Misc/AutomationTest.h"

#include "Components/WrapBox.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UI/Inventory/InventoryUIModel.h"
#include "UI/Inventory/InventoryUIWidgetBase.h"
#include "UI/Inventory/MockInventoryDriver.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSharedInventoryArtifactSnapshotTest,
	"P_RD.UI.Inventory.SharedArtifacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSharedInventoryArtifactSnapshotTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World()
		: nullptr;
	if (!TestNotNull(TEXT("에디터 월드를 찾았다"), World))
	{
		return false;
	}

	UClass* InventoryWidgetClass = LoadClass<UInventoryUIWidgetBase>(
		nullptr, TEXT("/Game/UI/WBP_Inventory.WBP_Inventory_C"));
	if (!TestNotNull(TEXT("실제 인벤토리 WBP 클래스를 찾았다"), InventoryWidgetClass))
	{
		return false;
	}

	UInventoryUIModel* Model =
		NewObject<UInventoryUIModel>(GetTransientPackage());
	UInventoryUIWidgetBase* Widget =
		CreateWidget<UInventoryUIWidgetBase>(World, InventoryWidgetClass);
	if (!TestNotNull(TEXT("인벤토리 위젯을 만들었다"), Widget))
	{
		return false;
	}

	// 먼저 빈 스냅샷을 바인딩해 empty state도 실제 레이아웃에서 검증한다.
	Model->SetInventory(FInventoryUI());
	Widget->BindUIModel(Model);
	const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	(void)SlateWidget;

	UTexture2D* BackgroundArt = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/UI/Art/RunFlow/T_Inventory_Background_Current.T_Inventory_Background_Current"));
	if (TestNotNull(TEXT("공용 인벤토리 배경 아트를 찾았다"), BackgroundArt))
	{
		const FIntPoint ImportedSize = BackgroundArt->GetImportedSize();
		TestEqual(TEXT("배경 아트 가로 디자인 크기는 1672다"), ImportedSize.X, 1672);
		TestEqual(TEXT("배경 아트 세로 디자인 크기는 941이다"), ImportedSize.Y, 941);
	}

	TestTrue(TEXT("위젯 CDO 하드 참조로 배경 아트가 준비된다"),
		Widget->HasBackgroundArt());
	TestTrue(TEXT("아이콘 없는 아티팩트의 기본 아이콘이 준비된다"),
		Widget->HasFallbackArtifactIcon());
	TestTrue(TEXT("아티팩트 카드에 CombatHUD 슬롯 프레임이 준비된다"),
		Widget->HasArtifactCardFrame());

	TestNotNull(TEXT("1672x941 fallback 디자인 캔버스를 생성한다"),
		Widget->GetWidgetFromName(TEXT("InventoryDesignCanvas")));
	TestNotNull(TEXT("공용 인벤토리 배경을 위젯 트리에 배치한다"),
		Widget->GetWidgetFromName(TEXT("InventoryFallbackBackground")));
	TestNotNull(TEXT("아티팩트 세로 스크롤을 생성한다"),
		Widget->GetWidgetFromName(TEXT("InventoryRuntimeArtifactScroll")));
	TestNotNull(TEXT("5열 WrapBox 아티팩트 그리드를 생성한다"),
		Widget->GetWidgetFromName(TEXT("InventoryRuntimeArtifactGrid")));
	TestNotNull(TEXT("모바일 닫기 터치 영역을 생성한다"),
		Widget->GetWidgetFromName(TEXT("InventoryRuntimeCloseButton")));

	UWrapBox* ArtifactGrid = Cast<UWrapBox>(
		Widget->GetWidgetFromName(TEXT("InventoryRuntimeArtifactGrid")));
	if (!TestNotNull(TEXT("아티팩트 그리드는 UWrapBox다"), ArtifactGrid))
	{
		return false;
	}

	TestEqual(TEXT("빈 인벤토리의 데이터 아티팩트 수는 0이다"),
		Widget->GetArtifactCount(), 0);
	TestEqual(TEXT("빈 인벤토리는 empty-state 한 칸만 그린다"),
		ArtifactGrid->GetChildrenCount(), 1);

	UMockInventoryDriver* Driver =
		NewObject<UMockInventoryDriver>(GetTransientPackage());
	Driver->Start(Model);

	TestEqual(TEXT("파티 공용 골드를 그대로 표시한다"),
		Widget->GetDisplayedGold(), 1840);
	TestEqual(TEXT("개발용 아티팩트 12개를 유지한다"),
		Widget->GetArtifactCount(), 12);
	TestEqual(TEXT("filled refresh가 카드 12개를 정확히 그린다"),
		ArtifactGrid->GetChildrenCount(), 12);

	const FInventoryUI& FilledSnapshot = Model->GetInventory();
	TestEqual(TEXT("첫 아티팩트 순서를 유지한다"),
		FilledSnapshot.mArtifacts[0].mName.ToString(), FString(TEXT("Ember Coin")));
	TestEqual(TEXT("마지막 아티팩트 순서를 유지한다"),
		FilledSnapshot.mArtifacts[11].mName.ToString(), FString(TEXT("Dragon's Omen")));

	TSet<UTexture2D*> DistinctIcons;
	for (const FInventoryArtifactUI& Artifact : FilledSnapshot.mArtifacts)
	{
		if (Artifact.mIcon != nullptr)
		{
			DistinctIcons.Add(Artifact.mIcon);
		}
	}
	TestEqual(TEXT("mock 12칸은 서로 다른 cooked MapNode 아이콘을 사용한다"),
		DistinctIcons.Num(), 12);

	// 같은 아티팩트를 여러 개 소유하는 경우에도 UI 계약은 중복 제거하지 않는다.
	FInventoryUI DuplicateSnapshot = FilledSnapshot;
	DuplicateSnapshot.mArtifacts.Add(FilledSnapshot.mArtifacts[0]);
	Model->SetInventory(DuplicateSnapshot);
	TestEqual(TEXT("중복 아티팩트를 제거하지 않는다"),
		Widget->GetArtifactCount(), 13);
	TestEqual(TEXT("중복도 입력 순서 끝에 남는다"),
		Model->GetInventory().mArtifacts[12].mName.ToString(),
		FString(TEXT("Ember Coin")));
	TestEqual(TEXT("중복 포함 카드 13개를 그린다"),
		ArtifactGrid->GetChildrenCount(), 13);

	// filled -> empty -> filled를 반복해도 이전 카드가 누적되면 안 된다.
	Model->SetInventory(FInventoryUI());
	TestEqual(TEXT("empty refresh가 기존 카드를 모두 지운다"),
		Widget->GetArtifactCount(), 0);
	TestEqual(TEXT("empty refresh 뒤 empty-state 하나만 남는다"),
		ArtifactGrid->GetChildrenCount(), 1);

	Driver->Start(Model);
	TestEqual(TEXT("두 번째 filled refresh도 정확히 12개다"),
		Widget->GetArtifactCount(), 12);
	TestEqual(TEXT("두 번째 filled refresh에서 카드가 누적되지 않는다"),
		ArtifactGrid->GetChildrenCount(), 12);

	return true;
}

#endif
