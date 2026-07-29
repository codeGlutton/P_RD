/*****************************************************************//**
 * @file   MercenaryHireTests.cpp
 * @brief  용병 선택 게시판의 규칙이 실제로 도는지.
 * @details
 * 화면을 띄워 눈으로 보는 것만으로는 "세 명 채운 뒤 네 번째를 누를 때"나
 * "골랐다가 다시 눌러 무를 때" 같은 자리를 못 훑는다. 규칙만 떼어 시험한다.
 *
 * 위젯을 실제로 만들되 WBP 는 쓰지 않는다. 이름으로 찾는 위젯이 하나도 안
 * 잡히지만 값을 넣는 자리는 전부 널을 걸러내므로, 규칙은 그대로 돈다.
 *
 * 앞선 판은 규칙을 시험 안에 그대로 다시 적어 놓고 그것을 검사했다. 코드가
 * 틀려도 통과하는 시험이었다. 여기서는 위젯이 진짜로 세는 것을 본다.
 * @author 박용수
 * @date   2026-07-27
 *********************************************************************/

#include "Misc/AutomationTest.h"

#include "Frontend/CharacterSelectTypes.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Hire/MercenaryHireWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** @brief 고를 수 있는 후보 한 명. */
	FFrontendCharacterOption MakeOption(const int32 Index, const TCHAR* Name,
		const bool bSelectable)
	{
		FFrontendCharacterOption Option;
		Option.mIndex = Index;
		Option.mDisplayName = FText::FromString(Name);
		Option.mMaxHP = 100;
		Option.mSelectable = bSelectable;
		Option.mPlayerUnitId = FPrimaryAssetId(TEXT("PlayerUnit"), FName(Name));
		return Option;
	}

	/** @brief 여섯 명 걸린 게시판. WBP 없이 규칙만 돈다. */
	UMercenaryHireWidget* MakeBoard(const int32 LockedIndex = INDEX_NONE)
	{
		UMercenaryHireWidget* Board =
			NewObject<UMercenaryHireWidget>(GetTransientPackage());
		TArray<FFrontendCharacterOption> Options;
		for (int32 Index = 0; Index < 6; ++Index)
		{
			Options.Add(MakeOption(Index,
				*FString::Printf(TEXT("후보%d"), Index),
				Index != LockedIndex));
		}
		Board->SetCharacterOptions(Options, 3);
		return Board;
	}

	/** @brief 하나를 정한다. 한 번이면 된다. */
	void Choose(UMercenaryHireWidget& Board, const int32 CardIndex)
	{
		Board.ClickCard(CardIndex);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireChooseTest,
	"P_RD.UI.MercenaryHire.Choose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireChooseTest::RunTest(const FString& Parameters)
{
	UMercenaryHireWidget* Board = MakeBoard();

	// 한 번 누르면 정해진다.
	Board->ClickCard(2);
	TestEqual(TEXT("한 번 누르면 정해짐"), Board->StateOf(2),
		EMercenaryCardState::Chosen);
	TestEqual(TEXT("한 명 정해짐"), Board->GetChosenIndices().Num(), 1);

	// 정해진 것을 또 누르면 풀린다.
	Board->ClickCard(2);
	TestEqual(TEXT("또 누르면 풀린다"), Board->GetChosenIndices().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHirePartyLimitTest,
	"P_RD.UI.MercenaryHire.PartyLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHirePartyLimitTest::RunTest(const FString& Parameters)
{
	UMercenaryHireWidget* Board = MakeBoard();
	TestFalse(TEXT("빈 채로는 못 떠난다"), Board->IsReadyToDepart());

	Choose(*Board, 0);
	Choose(*Board, 3);
	Choose(*Board, 5);
	TestTrue(TEXT("셋을 채우면 떠날 수 있다"), Board->IsReadyToDepart());
	TestEqual(TEXT("누른 차례가 파티 칸 순서"),
		Board->GetChosenIndices()[1], 3);

	// 자리가 찼는데 새 후보를 누르면 마지막 자리를 내준다.
	TestEqual(TEXT("고르기 전 마지막 자리는 5"),
		Board->GetChosenIndices()[2], 5);
	Choose(*Board, 1);
	TestEqual(TEXT("여전히 셋"), Board->GetChosenIndices().Num(), 3);
	TestEqual(TEXT("마지막 자리가 새 후보로"),
		Board->GetChosenIndices()[2], 1);
	TestFalse(TEXT("내준 후보는 빠져 있다"),
		Board->GetChosenIndices().Contains(5));
	TestEqual(TEXT("먼저 고른 둘은 그대로"),
		Board->GetChosenIndices()[0], 0);

	// 내준 자리는 다시 고를 수 있다. 이번엔 방금 들어온 1 이 밀려난다.
	Choose(*Board, 5);
	TestEqual(TEXT("바뀐 뒤에도 셋"), Board->GetChosenIndices().Num(), 3);
	TestTrue(TEXT("다시 고른 후보가 들어와 있다"),
		Board->GetChosenIndices().Contains(5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireLockedTest,
	"P_RD.UI.MercenaryHire.Locked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireLockedTest::RunTest(const FString& Parameters)
{
	// 잠긴 후보는 눌러도 아무 일이 없다. 잠금 판정은 게임 모드가 끝내 놓았고
	// 화면은 그 bool 만 믿는다.
	UMercenaryHireWidget* Board = MakeBoard(/*LockedIndex=*/4);

	Choose(*Board, 4);
	TestEqual(TEXT("잠긴 것은 안 골린다"), Board->GetChosenIndices().Num(), 0);

	Choose(*Board, 0);
	TestEqual(TEXT("안 잠긴 것은 골린다"), Board->GetChosenIndices().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireConfirmTest,
	"P_RD.UI.MercenaryHire.Confirm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireConfirmTest::RunTest(const FString& Parameters)
{
	// 출발이 넘기는 것은 식별자다. 런을 만드는 데 필요한 것이 그것뿐이다.
	UMercenaryHireWidget* Board = MakeBoard();
	TArray<FPrimaryAssetId> Sent;
	Board->mOnPartyConfirmed.AddLambda(
		[&Sent](const TArray<FPrimaryAssetId>& Party) { Sent = Party; });

	Board->ConfirmParty();
	TestEqual(TEXT("아무도 안 고르면 안 넘어간다"), Sent.Num(), 0);

	// 용병 자료가 셋을 채울 만큼 안 들어와서, 한 명으로도 출발하게 열어 두었다.
	// 자료가 갖춰지면 mMinPartySize 를 셋으로 올리고 이 시험도 같이 조인다.
	Choose(*Board, 1);
	Board->ConfirmParty();
	TestEqual(TEXT("한 명이면 넘어간다"), Sent.Num(), 1);

	Choose(*Board, 2);
	Choose(*Board, 0);
	Board->ConfirmParty();
	TestEqual(TEXT("셋을 넘긴다"), Sent.Num(), 3);
	TestEqual(TEXT("고른 차례대로 온다"), Sent[2].PrimaryAssetName,
		FName(TEXT("후보0")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatHUDCardNestingTest,
	"P_RD.UI.CombatHUD.CardNesting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief 카드 묶음이 제 부품을 실제로 품고 있나.
 *
 * @details
 * 카드가 안 접힌다를 여러 번 되풀이했다. 그때마다 입력 쪽을 뒤졌는데 첫
 * 원인은 위젯 구조였다 -- 묶음이 빈 껍데기여서 접어도 접을 것이 없었다.
 * 화면은 멀쩡히 나오므로 캡처로는 절대 안 잡힌다.
 *
 * 이 시험이 통과하는데 게임에서 안 되면 원인은 위젯이 아니라 입력이 거기까지
 * 안 온다는 뜻이다. 그 둘을 가르는 것이 이 시험의 일이다.
 *
 * 새 파일을 만들지 않고 여기 붙인 이유: P_RDTests 에 .cpp 를 하나 더하면
 * 유니티 빌드 묶음이 바뀌면서, 여러 시험 파일이 익명 이름공간에 같은 이름으로
 * 둔 도우미들이 한꺼번에 부딪힌다.
 */
bool FCombatHUDCardNestingTest::RunTest(const FString& Parameters)
{
	UClass* HUDClass = LoadClass<UCombatLayoutHUDWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if (!TestNotNull(TEXT("HUD 클래스를 찾았다"), HUDClass))
	{
		return false;
	}

	// 위젯 나무는 WBP 가 만든 클래스가 원본으로 들고 있다. 인스턴스나 클래스
	// 기본값에서 읽으면 아직 비어 있어 늘 널이다.
	UWidgetBlueprintGeneratedClass* Generated =
		Cast<UWidgetBlueprintGeneratedClass>(HUDClass);
	UWidgetTree* Tree = Generated != nullptr
		? Generated->GetWidgetTreeArchetype() : nullptr;
	if (!TestNotNull(TEXT("위젯 나무"), Tree))
	{
		return false;
	}

	auto CountChildren = [Tree](const TCHAR* Name) -> int32
	{
		UPanelWidget* Panel = Cast<UPanelWidget>(Tree->FindWidget(FName(Name)));
		return Panel != nullptr ? Panel->GetChildrenCount() : -1;
	};

	for (int32 Index = 0; Index < UCombatLayoutHUDWidget::CommandSlotCount; ++Index)
	{
		const FString Name = FString::Printf(TEXT("CommandCard_%d"), Index);
		// 판, 아이콘, 글자, 버튼이 이 안에 있어야 한다. 하나라도 밖에 있으면
		// 접었을 때 그것만 남는다.
		TestTrue(*FString::Printf(TEXT("%s 가 부품을 품는다"), *Name),
			CountChildren(*Name) >= 4);
	}

	for (int32 Index = 0; Index < UCombatLayoutHUDWidget::PartySlotCount; ++Index)
	{
		const FString Name = FString::Printf(TEXT("PartyCard_%d"), Index);
		TestTrue(*FString::Printf(TEXT("%s 가 부품을 품는다"), *Name),
			CountChildren(*Name) >= 4);

		// 다시 펴는 손잡이. 카드 안에 있어야 접힌 카드가 안 눌린다.
		const FString ButtonName = FString::Printf(TEXT("PartyButton_%d"), Index);
		UWidget* Button = Tree->FindWidget(FName(*ButtonName));
		if (TestNotNull(*ButtonName, Button))
		{
			TestEqual(*FString::Printf(TEXT("%s 가 카드 안에 있다"), *ButtonName),
				Button->GetParent(),
				Cast<UPanelWidget>(Tree->FindWidget(FName(*Name))));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatHUDCardToggleTest,
	"P_RD.UI.CombatHUD.CardToggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief 아군 칸 버튼이 실제로 묶여 카드를 접었다 폈다 하나.
 *
 * @details
 * 묶는 것을 빠뜨려도 화면은 똑같이 나오고 누르면 아무 일이 없다. 눈으로는
 * "입력이 안 온다" 와 구분이 안 된다. 이 시험이 통과하는데 게임에서 안 되면
 * 원인은 배선이 아니라 입력이 버튼까지 안 온다는 뜻이다.
 */
bool FCombatHUDCardToggleTest::RunTest(const FString& Parameters)
{
	UWorld* World = nullptr;
	if (GEngine != nullptr)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() != nullptr)
			{
				World = Context.World();
				break;
			}
		}
	}
	if (World == nullptr)
	{
		AddInfo(TEXT("월드가 없어 건너뜀"));
		return true;
	}

	UClass* HUDClass = LoadClass<UCombatLayoutHUDWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if (!TestNotNull(TEXT("HUD 클래스"), HUDClass))
	{
		return false;
	}

	UCombatLayoutHUDWidget* HUD =
		CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	// 위젯 캐시와 버튼 묶기는 NativeConstruct 에서 일어난다.
	HUD->TakeWidget();

	UButton* PartyButton = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("PartyButton_0")));
	if (!TestNotNull(TEXT("아군 칸 버튼"), PartyButton))
	{
		return false;
	}
	TestTrue(TEXT("아군 칸 버튼에 무언가 묶여 있다"),
		PartyButton->OnClicked.IsBound());

	UWidget* Card = HUD->WidgetTree->FindWidget(TEXT("CommandCard_0"));
	if (!TestNotNull(TEXT("명령 카드"), Card))
	{
		return false;
	}

	// 아군 칸은 뒤집기가 아니다.
	//
	// 전에는 누를 때마다 카드를 접었다 폈다 했다. 지금은 "누구의 스킬을 볼지"
	// 고르는 자리이고, 접고 펴는 것은 판 탭이 맡는다. 그래서 몇 번을 눌러도
	// 카드는 펴져 있어야 한다 -- 스킬을 보러 눌렀는데 접히면 아무 일도 안
	// 일어난 것처럼 보인다.
	PartyButton->OnClicked.Broadcast();
	TestEqual(TEXT("누르면 카드가 펴진다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	PartyButton->OnClicked.Broadcast();
	TestEqual(TEXT("다시 눌러도 접히지 않는다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	return true;
}

#endif
