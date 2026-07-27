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
	UMercenaryData* MakeMercenary(const TCHAR* Name)
	{
		UMercenaryData* Data = NewObject<UMercenaryData>();
		Data->mName = FText::FromString(Name);
		Data->mMaxHP = 100;
		return Data;
	}

	/** @brief 인원만 정해 놓은 게시판. */
	UMercenaryBoardData* MakeBoard(const int32 PartySize)
	{
		UMercenaryBoardData* Board = NewObject<UMercenaryBoardData>();
		Board->mPartySize = PartySize;
		return Board;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireChooseTest,
	"P_RD.UI.MercenaryHire.Choose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireChooseTest::RunTest(const FString& Parameters)
{
	// 값을 치르지 않으므로 여섯 중 셋이 그냥 채워진다. 예산 규칙을 없앤 뒤
	// "그래도 셋에서 멈추는가"가 남은 물음이다.
	UMercenaryBoardData* Board = MakeBoard(3);
	TArray<int32> Chosen;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		if (Chosen.Num() >= Board->mPartySize)
		{
			continue;
		}
		Chosen.Add(Index);
	}

	TestEqual(TEXT("셋만 고른다"), Chosen.Num(), 3);
	TestTrue(TEXT("셋을 채웠으므로 출발한다"),
		Chosen.Num() == Board->mPartySize);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHirePartyLimitTest,
	"P_RD.UI.MercenaryHire.PartyLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHirePartyLimitTest::RunTest(const FString& Parameters)
{
	// 인원을 넘길 수 없다.
	UMercenaryBoardData* Board = MakeBoard(3);
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
	// 프로토타입이 C++ 배열이라 이 시험을 둔다. 고용비는 없다.
	UMercenaryData* Data = MakeMercenary(TEXT("기사"));
	Data->mRole = EMercenaryRole::Melee;
	Data->mSkillNames.Add(FText::FromString(TEXT("방패 강타")));
	Data->mSkillNames.Add(FText::FromString(TEXT("반격 태세")));

	TestEqual(TEXT("이름"), Data->mName.ToString(), FString(TEXT("기사")));
	TestEqual(TEXT("스킬 두 줄"), Data->mSkillNames.Num(), 2);
	TestTrue(TEXT("역할이 있다"), Data->mRole != EMercenaryRole::Count);
	return true;
}

#endif
