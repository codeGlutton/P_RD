/*****************************************************************//**
 * @file   UnitPolyLineTests.cpp
 * @brief  AUnit::BakePolyLinePoints (코너링 폴리라인 베이크) 유닛테스트
 * @details
 * 정확 비교: 수학적으로 유도 가능한 지점(시작/끝점, 컷 지점 P1/P2, 베지어 t=0.5, 텐션별 통과점)
 * 성질 검증: 누적거리/마커 단조 증가, 곡선 길이 범위, 코너 삼각형 내부 유지, 텐션 증가 시 코너 중점 접근
 * @author 이문환
 * @date   2026-07-11
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "Pawn/Unit.h"

namespace
{
	// @brief 배열이 순증가(단조 증가)하는지 검사
	bool IsMonotonicIncreasing(const TArray<float>& Values)
	{
		for (int32 i = 1; i < Values.Num(); ++i)
		{
			if (Values[i] < Values[i - 1])
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnitBakePolyLinePointsTests,
	"P_RD.Unit.BakePolyLinePoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FUnitBakePolyLinePointsTests::RunTest(const FString& Parameters)
{
	// 좌표 비교 허용 오차 (cm)
	constexpr float Tolerance = 0.01f;
	// 타일 간격 (cm)
	constexpr float TileSize = 300.0f;

	TArray<FVector> OutPoints;
	TArray<float> OutDistances;
	TArray<float> OutMarkers;

	/* [케이스 1] 컷 비율 0: 폴리라인이 원본 경로와 완전히 같아야 함 (직각 이동과 동일) */
	{
		const TArray<FVector> Path = {
			FVector(0.0, 0.0, 0.0),
			FVector(TileSize, 0.0, 0.0),
			FVector(TileSize, TileSize, 0.0) };
		AUnit::BakePolyLinePoints(Path, 0.0f, 1.0f, OutPoints, OutDistances, OutMarkers);

		TestEqual(TEXT("컷0: 점 개수 = 원본 경로 개수"), OutPoints.Num(), Path.Num());
		for (int32 i = 0; i < FMath::Min(OutPoints.Num(), Path.Num()); ++i)
		{
			TestTrue(TEXT("컷0: 각 점이 타일 중점과 일치"), OutPoints[i].Equals(Path[i], Tolerance));
		}
		TestEqual(TEXT("컷0: 전체 길이 = 직각 경로 길이"), OutDistances.Last(), TileSize * 2.0f, Tolerance);
		TestEqual(TEXT("컷0: 마커 개수 = 경로 개수"), OutMarkers.Num(), Path.Num());
	}

	/* [케이스 2] 직진 경로: 컷 비율이 있어도 커브가 없으므로 원본과 동일해야 함 */
	{
		const TArray<FVector> Path = {
			FVector(0.0, 0.0, 0.0),
			FVector(TileSize, 0.0, 0.0),
			FVector(TileSize * 2.0f, 0.0, 0.0) };
		AUnit::BakePolyLinePoints(Path, 0.35f, 1.0f, OutPoints, OutDistances, OutMarkers);

		TestEqual(TEXT("직진: 점 개수 = 원본 경로 개수"), OutPoints.Num(), Path.Num());
		for (int32 i = 0; i < FMath::Min(OutPoints.Num(), Path.Num()); ++i)
		{
			TestTrue(TEXT("직진: 각 점이 타일 중점과 일치"), OutPoints[i].Equals(Path[i], Tolerance));
		}
	}

	/* [케이스 3] L자 코너 (컷 비율 0.25): 정확 지점 + 성질 검증 */
	{
		// A(0,0) -> B(300,0) -> C(300,300). B에서 직각으로 꺾임
		const TArray<FVector> Path = {
			FVector(0.0, 0.0, 0.0),
			FVector(TileSize, 0.0, 0.0),
			FVector(TileSize, TileSize, 0.0) };
		constexpr float CutRatio = 0.25f;
		AUnit::BakePolyLinePoints(Path, CutRatio, 1.0f, OutPoints, OutDistances, OutMarkers);

		// 컷 거리 = 300 x 0.25 = 75
		const FVector CurveStart(TileSize - TileSize * CutRatio, 0.0, 0.0);   // P1 = (225, 0)
		const FVector CurveEnd(TileSize, TileSize * CutRatio, 0.0);           // P2 = (300, 75)
		const FVector CornerPoint = Path[1];                                  // C(컨트롤 포인트) = (300, 0)

		/* 정확 비교 */

		// 시작/끝점은 타일 중점과 정확히 일치
		TestTrue(TEXT("L자: 시작점 = 시작 타일 중점"), OutPoints[0].Equals(Path[0], Tolerance));
		TestTrue(TEXT("L자: 끝점 = 도착 타일 중점"), OutPoints.Last().Equals(Path.Last(), Tolerance));

		// 커브 시작점(P1)/끝점(P2)은 컷 비율에서 유도한 좌표와 일치
		TestTrue(TEXT("L자: 커브 시작점 = P1"), OutPoints[1].Equals(CurveStart, Tolerance));
		TestTrue(TEXT("L자: 커브 끝점 = P2"), OutPoints[OutPoints.Num() - 2].Equals(CurveEnd, Tolerance));

		// 베지어 t=0.5 지점 = (P1 + 2C + P2) / 4 (2차 베지어 공식의 해석값)
		const FVector BezierMidPoint = (CurveStart + CornerPoint * 2.0f + CurveEnd) / 4.0f;
		bool ContainsMidPoint = false;
		for (const FVector& Point : OutPoints)
		{
			if (Point.Equals(BezierMidPoint, Tolerance))
			{
				ContainsMidPoint = true;
				break;
			}
		}
		TestTrue(TEXT("L자: 베지어 t=0.5 지점 포함"), ContainsMidPoint);

		/* 성질 검증 */

		// 누적거리/마커는 순증가, 마커 개수 = 경로 개수
		TestTrue(TEXT("L자: 누적거리 순증가"), IsMonotonicIncreasing(OutDistances));
		TestTrue(TEXT("L자: 마커 순증가"), IsMonotonicIncreasing(OutMarkers));
		TestEqual(TEXT("L자: 마커 개수 = 경로 개수"), OutMarkers.Num(), Path.Num());

		// 곡선 전체 길이: 최단 하한(직선 구간 + P1~P2 직선 거리) < 길이 < 직각 경로 길이
		const float StraightSections = (TileSize - TileSize * CutRatio) * 2.0f;   // A~P1 + P2~C = 450
		const float CurveChord = FVector::Dist(CurveStart, CurveEnd);              // P1~P2 직선 거리
		const float TotalLength = OutDistances.Last();
		TestTrue(TEXT("L자: 길이 > 최단 하한"), TotalLength > StraightSections + CurveChord - Tolerance);
		TestTrue(TEXT("L자: 길이 < 직각 경로"), TotalLength < TileSize * 2.0f);

		// 코너 타일 마커는 커브 구간(P1~P2) 안에 위치
		TestTrue(TEXT("L자: 코너 마커 >= P1 거리"), OutMarkers[1] >= OutDistances[1] - Tolerance);
		TestTrue(TEXT("L자: 코너 마커 <= P2 거리"), OutMarkers[1] <= OutDistances[OutDistances.Num() - 2] + Tolerance);

		// 모든 커브 점이 삼각형 P1-C-P2 내부 (경로 이탈 없음)
		// 이 지오메트리에서 내부 조건: 225 <= X <= 300, 0 <= Y <= X - 225
		for (int32 i = 1; i < OutPoints.Num() - 1; ++i)
		{
			const FVector& Point = OutPoints[i];
			TestTrue(TEXT("L자: 커브 점 X 범위"), Point.X >= CurveStart.X - Tolerance && Point.X <= CornerPoint.X + Tolerance);
			TestTrue(TEXT("L자: 커브 점 Y 범위"), Point.Y >= 0.0 - Tolerance && Point.Y <= Point.X - CurveStart.X + Tolerance);
		}
	}

	/* [케이스 4] 180도 반전 경로: 크래시 없이 성질 유지 (베지어가 되돌아오기 곡선으로 퇴화) */
	{
		const TArray<FVector> Path = {
			FVector(0.0, 0.0, 0.0),
			FVector(TileSize, 0.0, 0.0),
			FVector(0.0, 0.0, 0.0) };
		AUnit::BakePolyLinePoints(Path, 0.25f, 1.0f, OutPoints, OutDistances, OutMarkers);

		TestTrue(TEXT("반전: 시작점 일치"), OutPoints[0].Equals(Path[0], Tolerance));
		TestTrue(TEXT("반전: 끝점 일치"), OutPoints.Last().Equals(Path.Last(), Tolerance));
		TestTrue(TEXT("반전: 누적거리 순증가"), IsMonotonicIncreasing(OutDistances));
		TestTrue(TEXT("반전: 마커 순증가"), IsMonotonicIncreasing(OutMarkers));
		TestEqual(TEXT("반전: 마커 개수 = 경로 개수"), OutMarkers.Num(), Path.Num());
	}

	/* [케이스 5] 경로 2칸 미만: 폴리라인 불성립 (빈 출력) */
	{
		const TArray<FVector> Path = { FVector(0.0, 0.0, 0.0) };
		AUnit::BakePolyLinePoints(Path, 0.25f, 1.0f, OutPoints, OutDistances, OutMarkers);

		TestEqual(TEXT("1칸: 점 없음"), OutPoints.Num(), 0);
		TestEqual(TEXT("1칸: 마커 없음"), OutMarkers.Num(), 0);
	}

	/* [케이스 6] 코너 텐션: 곡선을 코너 중점 쪽으로 당기는 정도 검증 (L자, 컷 비율 0.5) */
	{
		const TArray<FVector> Path = {
			FVector(0.0, 0.0, 0.0),
			FVector(TileSize, 0.0, 0.0),
			FVector(TileSize, TileSize, 0.0) };
		const FVector CornerPoint = Path[1];

		// 텐션 2: 곡선의 t=0.5 지점이 정확히 코너 중점을 통과 (해석값)
		AUnit::BakePolyLinePoints(Path, 0.5f, 2.0f, OutPoints, OutDistances, OutMarkers);
		bool ContainsCornerPoint = false;
		for (const FVector& Point : OutPoints)
		{
			if (Point.Equals(CornerPoint, Tolerance))
			{
				ContainsCornerPoint = true;
				break;
			}
		}
		TestTrue(TEXT("텐션2: 곡선이 코너 중점 통과"), ContainsCornerPoint);

		// 텐션 0: 커브 구간이 P1~P2 직선 (대각선 직진) -> 곡선 길이 = 직선 구간 + 현의 길이
		AUnit::BakePolyLinePoints(Path, 0.5f, 0.0f, OutPoints, OutDistances, OutMarkers);
		const FVector CurveStart(TileSize * 0.5f, 0.0, 0.0);
		const FVector CurveEnd(TileSize, TileSize * 0.5f, 0.0);
		const float ExpectedLength = TileSize + FVector::Dist(CurveStart, CurveEnd);   // 직선 150x2 + 현
		TestEqual(TEXT("텐션0: 전체 길이 = 직선 + 현"), OutDistances.Last(), ExpectedLength, Tolerance);

		// 텐션 0 < 1 < 2 순으로 곡선이 코너 중점에 가까워짐 (t=0.5 지점의 중점 거리 비교)
		float ApexDistances[3] = { 0.0f, 0.0f, 0.0f };
		const float Tensions[3] = { 0.0f, 1.0f, 2.0f };
		for (int32 i = 0; i < 3; ++i)
		{
			AUnit::BakePolyLinePoints(Path, 0.5f, Tensions[i], OutPoints, OutDistances, OutMarkers);
			// 곡선 점들 중 코너 중점과 가장 가까운 거리
			float MinDistance = TNumericLimits<float>::Max();
			for (const FVector& Point : OutPoints)
			{
				MinDistance = FMath::Min(MinDistance, static_cast<float>(FVector::Dist(Point, CornerPoint)));
			}
			ApexDistances[i] = MinDistance;
		}
		TestTrue(TEXT("텐션 증가 -> 코너 중점 접근"), ApexDistances[0] > ApexDistances[1] && ApexDistances[1] > ApexDistances[2]);
	}

	return true;
}
