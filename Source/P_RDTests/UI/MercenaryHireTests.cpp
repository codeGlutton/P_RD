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

	/** @brief 두 번 눌러 하나를 정한다. 한 번은 검토, 두 번째가 확정이다. */
	void Choose(UMercenaryHireWidget& Board, const int32 CardIndex)
	{
		Board.ClickCard(CardIndex);
		Board.ClickCard(CardIndex);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireChooseTest,
	"P_RD.UI.MercenaryHire.Choose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireChooseTest::RunTest(const FString& Parameters)
{
	UMercenaryHireWidget* Board = MakeBoard();

	// 한 번 누르면 검토일 뿐 아직 안 정해진다.
	Board->ClickCard(2);
	TestEqual(TEXT("한 번 누르면 검토 중"), Board->StateOf(2),
		EMercenaryCardState::Reviewing);
	TestEqual(TEXT("아직 아무도 안 정해짐"),
		Board->GetChosenIndices().Num(), 0);

	// 같은 것을 다시 누르면 정해진다.
	Board->ClickCard(2);
	TestEqual(TEXT("두 번 누르면 정해짐"), Board->StateOf(2),
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

	// 네 번째는 안 들어간다. 자리가 찼다.
	Choose(*Board, 1);
	TestEqual(TEXT("셋에서 멈춘다"), Board->GetChosenIndices().Num(), 3);
	TestEqual(TEXT("남은 이력서는 자리 참"), Board->StateOf(4),
		EMercenaryCardState::Full);
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

	Choose(*Board, 1);
	Choose(*Board, 2);
	Board->ConfirmParty();
	TestEqual(TEXT("덜 채우면 안 넘어간다"), Sent.Num(), 0);

	Choose(*Board, 0);
	Board->ConfirmParty();
	TestEqual(TEXT("셋을 넘긴다"), Sent.Num(), 3);
	TestEqual(TEXT("고른 차례대로 온다"), Sent[2].PrimaryAssetName,
		FName(TEXT("후보0")));
	return true;
}

#endif
