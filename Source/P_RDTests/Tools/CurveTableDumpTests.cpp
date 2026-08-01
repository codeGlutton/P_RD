/*****************************************************************//**
 * @file   CurveTableDumpTests.cpp
 * @brief  커브 표를 CSV 로 뽑아 놓는다.
 * @details
 * 커브 표는 파이썬에서 행을 읽을 수도 쓸 수도 없다. 그런데 유닛의 스탯이
 * 전부 여기 들어 있고, 새 유닛을 만들려면 기존 행이 어떤 모양인지 봐야
 * 한다. 눈으로 보려고 에디터에서 표를 하나씩 여는 것은 비교가 안 된다.
 *
 * 시험이 아니라 도구다. 시험 틀을 빌린 이유는 C++ 에서만 닿는 함수를
 * 부르면서 게임을 띄우지 않아도 되는 자리가 여기뿐이기 때문이다.
 * @author 박용수
 * @date   2026-07-28
 *********************************************************************/

#include "Misc/AutomationTest.h"

#include "Engine/CurveTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectIterator.h"

#if WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCurveTableDumpTest,
	"P_RD.Tools.DumpCurveTables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCurveTableDumpTest::RunTest(const FString& Parameters)
{
	const FString OutDir = FPaths::ProjectSavedDir() / TEXT("CurveTables");
	int32 Count = 0;

	for (TObjectIterator<UCurveTable> It; It; ++It)
	{
		UCurveTable* Table = *It;
		if (Table == nullptr || Table->GetPathName().StartsWith(TEXT("/Engine")))
		{
			continue;
		}

		const FString Csv = Table->GetTableAsCSV();
		if (Csv.IsEmpty())
		{
			continue;
		}

		const FString Path = OutDir / (Table->GetName() + TEXT(".csv"));
		if (FFileHelper::SaveStringToFile(Csv, *Path,
			FFileHelper::EEncodingOptions::ForceUTF8))
		{
			++Count;
		}
	}

	AddInfo(FString::Printf(TEXT("커브 표 %d개를 %s 에 남김"), Count, *OutDir));
	TestTrue(TEXT("표를 하나라도 뽑았다"), Count > 0);
	return true;
}

#endif
