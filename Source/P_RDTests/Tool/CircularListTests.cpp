/*****************************************************************//**
 * @file   CircularListTests.cpp
 * @brief  TCircularList 자동화 테스트
 * @details
 * TCircularList API의 동작을 검증한다.
 * @author 모호재
 * @date   2026-06-04
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "Tool/CircularList.h"

// Int32 테스트
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCircularListInt32Tests,
    "P_RD.Tool.CircularList.Int32",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FCircularListInt32Tests::RunTest(const FString& Parameters)
{
    TCircularDoubleLinkedList<int32> TestObj;

    // 객체 생성 검증
    TestNotNull(TEXT("TCircularDoubleLinkedList<int32> 반드시 생성"), &TestObj);

    // 초기화 검증
    TestEqual(TEXT("TCircularDoubleLinkedList<int32> 초기 노드 0개"), TestObj.IsEmpty(), true);
    TestEqual(TEXT("TCircularDoubleLinkedList<int32> 초기 헤드 노드 nullptr"), TestObj.GetHead() == nullptr, true);
    TestEqual(TEXT("TCircularDoubleLinkedList<int32> 초기 테일 노드 nullptr"), TestObj.GetTail() == nullptr, true);


    // 빈 경우 Add Head 검증
    const int32 HeadFirstNumber = 0;
    TestObj.AddHead(HeadFirstNumber);

    AddInfo(TEXT("0 초기 AddHead 진행 후 검사"));
    TestEqual(TEXT("TCircularDoubleLinkedList<int32> 헤더 노드 값 0"), TestObj.GetHead()->GetValue(), HeadFirstNumber);
    TestEqual(TEXT("TCircularDoubleLinkedList<int32> 테일 노드 값 0"), TestObj.GetTail()->GetValue(), HeadFirstNumber);
    TestEqual(TEXT("TCircularDoubleLinkedList<int32> 헤더 다음 노드 값 0"), TestObj.GetHead()->GetNextNode()->GetValue(), HeadFirstNumber);
    TestEqual(TEXT("TCircularDoubleLinkedList<int32> 헤더 이전 노드 값 0"), TestObj.GetHead()->GetPrevNode()->GetValue(), HeadFirstNumber);
    TestEqual(TEXT("TCircularDoubleLinkedList<int32> 테일 다음 노드 값 0"), TestObj.GetTail()->GetNextNode()->GetValue(), HeadFirstNumber);
    TestEqual(TEXT("TCircularDoubleLinkedList<int32> 테일 이전 노드 값 0"), TestObj.GetTail()->GetPrevNode()->GetValue(), HeadFirstNumber);

    // 하나라도 채워진 경우 Add Head 검증
    AddInfo(TEXT("1부터 30 삽입 AddHead 진행 후 검사"));
    for (int32 i = 1; i <= 30; ++i)
    {
        TestObj.AddHead(i);
        TestEqual(FString::Printf(TEXT("TCircularDoubleLinkedList<int32> 헤더에 삽입 값 %d"), i), TestObj.GetHead()->GetValue(), i);
        TestEqual(FString::Printf(TEXT("TCircularDoubleLinkedList<int32> 헤더 다음 값"), i), TestObj.GetHead()->GetNextNode()->GetValue(), i - 1);
        TestEqual(FString::Printf(TEXT("TCircularDoubleLinkedList<int32> 헤더 이전 값"), i), TestObj.GetHead()->GetPrevNode()->GetValue(), HeadFirstNumber);
    }
    return true;
}

