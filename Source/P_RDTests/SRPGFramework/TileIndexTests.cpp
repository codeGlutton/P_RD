/*****************************************************************//**
 * @file   TileIndexTests.cpp
 * @brief  FTileIndex 거리 함수 유닛테스트
 * @details
 * 맨해튼 거리와 체비셰프 거리 검증.
 * 두 함수의 결과가 갈리는 최소 사례(대각 1칸)를 포함해 구현이 서로 뒤바뀌는 실수를 방지.
 * @author 이문환
 * @date   2026-09-21
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "SRPGFramework/SRPGFrameworkType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTileIndexDistanceTests,
	"P_RD.SRPG.TileIndex.Distance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTileIndexDistanceTests::RunTest(const FString& Parameters)
{
	AddInfo(TEXT("같은 칸: 두 거리 모두 0"));
	{
		const FTileIndex Tile(3, 4);
		TestEqual(TEXT("같은 칸 맨해튼"), FTileIndex::ManhattanDistance(Tile, Tile), 0);
		TestEqual(TEXT("같은 칸 체비셰프"), FTileIndex::ChebyshevDistance(Tile, Tile), 0);
	}

	AddInfo(TEXT("직교 3칸: 두 거리가 같음"));
	{
		const FTileIndex Origin(2, 2);
		const FTileIndex Horizontal(5, 2);
		const FTileIndex Vertical(2, 5);
		TestEqual(TEXT("가로 3칸 맨해튼"), FTileIndex::ManhattanDistance(Origin, Horizontal), 3);
		TestEqual(TEXT("가로 3칸 체비셰프"), FTileIndex::ChebyshevDistance(Origin, Horizontal), 3);
		TestEqual(TEXT("세로 3칸 맨해튼"), FTileIndex::ManhattanDistance(Origin, Vertical), 3);
		TestEqual(TEXT("세로 3칸 체비셰프"), FTileIndex::ChebyshevDistance(Origin, Vertical), 3);
	}

	AddInfo(TEXT("대각 1칸: 두 거리가 갈리는 최소 사례"));
	{
		const FTileIndex Origin(2, 2);
		const FTileIndex Diagonal(3, 3);
		TestEqual(TEXT("대각 1칸 맨해튼"), FTileIndex::ManhattanDistance(Origin, Diagonal), 2);
		TestEqual(TEXT("대각 1칸 체비셰프"), FTileIndex::ChebyshevDistance(Origin, Diagonal), 1);
	}

	AddInfo(TEXT("가로 2, 세로 5: 맨해튼은 합, 체비셰프는 큰 값"));
	{
		const FTileIndex Origin(1, 1);
		const FTileIndex Target(3, 6);
		TestEqual(TEXT("가로 2 세로 5 맨해튼"), FTileIndex::ManhattanDistance(Origin, Target), 7);
		TestEqual(TEXT("가로 2 세로 5 체비셰프"), FTileIndex::ChebyshevDistance(Origin, Target), 5);
	}

	AddInfo(TEXT("음수 방향 변위: 절댓값 기준"));
	{
		const FTileIndex Origin(5, 5);
		const FTileIndex Target(3, 0);
		TestEqual(TEXT("음수 방향 맨해튼"), FTileIndex::ManhattanDistance(Origin, Target), 7);
		TestEqual(TEXT("음수 방향 체비셰프"), FTileIndex::ChebyshevDistance(Origin, Target), 5);
	}

	AddInfo(TEXT("음수 좌표: 무효 인덱스(-1,-1)가 들어와도 계산은 정의됨"));
	{
		const FTileIndex Target(2, 1);
		TestEqual(TEXT("음수 좌표 맨해튼"), FTileIndex::ManhattanDistance(FTileIndex::Invalid, Target), 5);
		TestEqual(TEXT("음수 좌표 체비셰프"), FTileIndex::ChebyshevDistance(FTileIndex::Invalid, Target), 3);
	}

	AddInfo(TEXT("대칭성: 인자 순서를 바꿔도 같은 값"));
	{
		const FTileIndex A(0, 7);
		const FTileIndex B(4, 2);
		TestEqual(TEXT("맨해튼 대칭"), FTileIndex::ManhattanDistance(A, B), FTileIndex::ManhattanDistance(B, A));
		TestEqual(TEXT("체비셰프 대칭"), FTileIndex::ChebyshevDistance(A, B), FTileIndex::ChebyshevDistance(B, A));
		TestEqual(TEXT("맨해튼 값"), FTileIndex::ManhattanDistance(A, B), 9);
		TestEqual(TEXT("체비셰프 값"), FTileIndex::ChebyshevDistance(A, B), 5);
	}

	return true;
}
