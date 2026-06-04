/*****************************************************************//**
 * @file   SampleTests.cpp
 * @brief  자동화 테스트 작성 예제
 * @details
 * 팀원들이 테스트 작성 패턴을 익힐 수 있도록 기본 예제를 제공한다.
 * 새 테스트 파일 작성 시 이 파일을 참고한다.
 * @author 이문환
 * @date   2026-04-30
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

// 테스트 등록 매크로
// 인자1: 테스트 클래스 이름 (프로젝트 내 고유해야 함)
// 인자2: 에디터 Automation 탭에 표시될 경로 (점으로 구분)
// 인자3: 실행 컨텍스트 및 필터 플래그 (일반적으로 이 설정을 그대로 사용)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSampleTest,
    "P_RD.Sample.SampleTest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 샘플 테스트 실행 함수
 * @details
 * TestTrue, TestEqual 등의 검증 매크로로 조건을 확인한다.
 * false 반환 시 테스트 실패로 처리된다.
 */
bool FSampleTest::RunTest(const FString& Parameters)
{
    // 조건이 참인 지 확인
    TestTrue("1 + 1 should be 2", 1 + 1 == 2);

    // 두 값이 같은 지 확인
    // 실패 시 출력 예시: "Expected 'Initial health value' to be 50.000000, but it was 100.000000 and outside tolerance 0.000100."
    constexpr float Actual = 100.f;
    constexpr float Expected = 100.f;
    TestEqual("Initial health value", Actual, Expected);

    // 포인터가 유효한 지 확인
    FVector* Vec = new FVector(1.f, 2.f, 3.f);
    TestNotNull("FVector should be valid", Vec);
    delete Vec;

    return true;
}
