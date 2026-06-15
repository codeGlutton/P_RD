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
    {
        TCircularDoubleLinkedList<int32> HeadTestObj;

        AddInfo(TEXT("객체 생성 검증"));
        {
            TestNotNull(TEXT("TCircularDoubleLinkedList<int32> 반드시 생성"), &HeadTestObj);
        }

        AddInfo(TEXT("초기화 검증"));
        {
            TestEqual(TEXT("TCircularDoubleLinkedList<int32> 초기 노드 0개"), HeadTestObj.IsEmpty(), true);
            TestEqual(TEXT("TCircularDoubleLinkedList<int32> 초기 헤드 노드 nullptr"), HeadTestObj.GetHead() == nullptr, true);
            TestEqual(TEXT("TCircularDoubleLinkedList<int32> 초기 테일 노드 nullptr"), HeadTestObj.GetTail() == nullptr, true);
        }

        const int32 HeadFirstNumber = 0;
        HeadTestObj.AddHead(HeadFirstNumber);

        AddInfo(TEXT("AddHead 및 순환 구조 검증"));
        for (int32 i = 1; i <= 30; ++i)
        {
            HeadTestObj.AddHead(i);
            TestEqual(TEXT("AddHead 헤드 값 검증"), HeadTestObj.GetHead()->GetValue(), i);
            TestEqual(TEXT("AddHead 다음 값 검증"), HeadTestObj.GetHead()->GetNextNode()->GetValue(), i - 1);
            TestEqual(TEXT("AddHead 이전 값 검증"), HeadTestObj.GetHead()->GetPrevNode()->GetValue(), HeadFirstNumber);
        }
    }

    {
        TCircularDoubleLinkedList<int32> TailTestObj;
        TailTestObj.AddTail(10);
        TailTestObj.AddTail(20);
        TailTestObj.AddTail(30);

        AddInfo(TEXT("AddTail 및 순환 구조 검증"));
        {
            TestEqual(TEXT("AddTail: 개수 검증"), TailTestObj.Num(), 3);
            TestEqual(TEXT("AddTail: 헤드 값 검증"), TailTestObj.GetHead()->GetValue(), 10);
            TestEqual(TEXT("AddTail: 테일 값 검증"), TailTestObj.GetTail()->GetValue(), 30);
        }

        AddInfo(TEXT("순환성 검증"));
        {

            TestEqual(TEXT("순환성: Tail->Next == Head"), TailTestObj.GetTail()->GetNextNode(), TailTestObj.GetHead());
            TestEqual(TEXT("순환성: Head->Prev == Tail"), TailTestObj.GetHead()->GetPrevNode(), TailTestObj.GetTail());
        }

        AddInfo(TEXT("InsertNode 검증"));
        {
            auto Node20 = TailTestObj.FindNode(20);
            TailTestObj.InsertNode(15, Node20); // 10 -> 15 -> 20 -> 30
            TestEqual(TEXT("InsertNode: 중간 삽입 후 값 검증"), Node20->GetPrevNode()->GetValue(), 15);
            TestEqual(TEXT("InsertNode: 삽입된 노드의 앞 노드 검증"), TailTestObj.FindNode(15)->GetPrevNode()->GetValue(), 10);
        }

        AddInfo(TEXT("RemoveNode 검증"));
        {
            TailTestObj.RemoveNode(15); // 10 -> 20 -> 30
            TestFalse(TEXT("RemoveNode: 삭제된 노드 미포함 확인"), TailTestObj.Contains(15));
            TestEqual(TEXT("RemoveNode: 삭제 후 개수 확인"), TailTestObj.Num(), 3);
            TestEqual(TEXT("RemoveNode: 삭제 후 연결 확인"), TailTestObj.FindNode(10)->GetNextNode()->GetValue(), 20);
        }

        AddInfo(TEXT("Iterator 검증"));
        {
            auto Iter = TCircularDoubleLinkedList<int32>::TIterator(TailTestObj.GetHead());
            TestEqual(TEXT("Iterator: 첫 번째"), *Iter, 10);
            ++Iter;
            TestEqual(TEXT("Iterator: 두 번째"), *Iter, 20);
            ++Iter;
            TestEqual(TEXT("Iterator: 세 번째"), *Iter, 30);
            ++Iter;
            TestEqual(TEXT("Iterator: 순환 후 첫 번째"), *Iter, 10); // 다시 10으로 돌아옴
            --Iter;
            TestEqual(TEXT("Iterator: 역순 순환 후 마지막"), *Iter, 30); // 다시 30으로 돌아감
        }

        AddInfo(TEXT("Empty 검증"));
        {
            TailTestObj.Empty();
            TestTrue(TEXT("Empty: 비어있음 확인"), TailTestObj.IsEmpty());
            TestNull(TEXT("Empty: 헤드 nullptr 확인"), TailTestObj.GetHead());
        }
    }

    return true;
}

