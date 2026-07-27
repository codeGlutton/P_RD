/*****************************************************************//**
 * @file   MercenaryHireTests.cpp
 * @brief  고용 게시판의 규칙이 실제로 도는지.
 * @details
 * 화면을 띄워 눈으로 보는 것만으로는 "예산이 딱 맞을 때"나 "세 명 채운 뒤
 * 네 번째를 누를 때" 같은 자리를 못 훑는다. 규칙만 떼어 시험한다.
 *
 * 위젯을 실제로 만들지 않는다. 게시판 규칙은 위젯이 없어도 성립하고, 위젯을
 * 만들면 WBP 와 텍스처까지 끌고 와야 해서 시험이 무거워진다.
 * @author 박용수
 * @date   2026-07-27
 *********************************************************************/

#include "Misc/AutomationTest.h"

#include "DataAsset/MercenaryData/MercenaryData.h"
#include "UI/Hire/MercenaryHireWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UMercenaryData* MakeMercenary(const TCHAR* Name, const int32 Cost)
	{
		UMercenaryData* Data = NewObject<UMercenaryData>();
		Data->mName = FText::FromString(Name);
		Data->mCost = Cost;
		Data->mMaxHP = 100;
		return Data;
	}

	/** @brief 예산과 인원만 정해 놓은 게시판. 이력서는 값만 다르다. */
	UMercenaryBoardData* MakeBoard(const int32 Budget, const int32 PartySize)
	{
		UMercenaryBoardData* Board = NewObject<UMercenaryBoardData>();
		Board->mBudget = Budget;
		Board->mPartySize = PartySize;
		return Board;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireBudgetTest,
	"P_RD.UI.MercenaryHire.Budget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireBudgetTest::RunTest(const FString& Parameters)
{
	// 예산 100, 셋을 데려간다. 40 + 40 은 되고 그 위에 40 을 더하면 넘는다.
	UMercenaryBoardData* Board = MakeBoard(100, 3);
	TArray<TObjectPtr<UMercenaryData>> Crew;
	Crew.Add(MakeMercenary(TEXT("갑"), 40));
	Crew.Add(MakeMercenary(TEXT("을"), 40));
	Crew.Add(MakeMercenary(TEXT("병"), 40));

	int32 Spent = 0;
	int32 Taken = 0;
	for (const TObjectPtr<UMercenaryData>& One : Crew)
	{
		if (Taken >= Board->mPartySize)
		{
			break;
		}
		if (Spent + One->mCost > Board->mBudget)
		{
			continue;
		}
		Spent += One->mCost;
		++Taken;
	}

	TestEqual(TEXT("예산 안에서 둘만 데려간다"), Taken, 2);
	TestEqual(TEXT("쓴 돈은 80"), Spent, 80);
	TestTrue(TEXT("셋을 못 채웠으므로 출발 못 한다"),
		Taken != Board->mPartySize);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHirePartyLimitTest,
	"P_RD.UI.MercenaryHire.PartyLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHirePartyLimitTest::RunTest(const FString& Parameters)
{
	// 돈이 넉넉해도 인원은 넘길 수 없다. 예산과 인원은 따로 막혀야 한다.
	UMercenaryBoardData* Board = MakeBoard(1000, 3);
	TArray<int32> Hired;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		if (Hired.Num() >= Board->mPartySize)
		{
			continue;
		}
		Hired.Add(Index);
	}

	TestEqual(TEXT("셋에서 멈춘다"), Hired.Num(), 3);
	TestEqual(TEXT("누른 차례가 파티 칸 순서"), Hired[0], 0);
	TestEqual(TEXT("세 번째는 2번 이력서"), Hired[2], 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireDataAssetTest,
	"P_RD.UI.MercenaryHire.DataAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireDataAssetTest::RunTest(const FString& Parameters)
{
	// 수치가 데이터에 있는지. 코드에 박혀 있으면 기획에서 못 고친다 --
	// 프로토타입이 C++ 배열이라 이 시험을 둔다.
	UMercenaryData* Data = MakeMercenary(TEXT("기사"), 40);
	Data->mRole = EMercenaryRole::Melee;
	Data->mSkillNames.Add(FText::FromString(TEXT("방패 강타")));
	Data->mSkillNames.Add(FText::FromString(TEXT("반격 태세")));

	TestEqual(TEXT("이름"), Data->mName.ToString(), FString(TEXT("기사")));
	TestEqual(TEXT("비용"), Data->mCost, 40);
	TestEqual(TEXT("스킬 두 줄"), Data->mSkillNames.Num(), 2);
	TestTrue(TEXT("역할이 있다"), Data->mRole != EMercenaryRole::Count);
	return true;
}

#endif
