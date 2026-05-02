/*****************************************************************//**
 * @file   UnitAttributeSetTests.cpp
 * @brief  UUnitAttributeSet 자동화 테스트
 * @details
 * UUnitAttributeSet 초기값 및 동작을 검증한다.
 * @author 이문환
 * @date   2026-04-30
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "GAS/Attribute/UnitAttributeSet.h"

// MaxHP 초기값 테스트
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUnitAttributeSetMaxHPTests,
    "P_RD.GAS.Attribute.UnitAttributeSet.MaxHP",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FUnitAttributeSetMaxHPTests::RunTest(const FString& Parameters)
{
    const UUnitAttributeSet* AttributeSet = NewObject<UUnitAttributeSet>();
    TestNotNull("UUnitAttributeSet should be created", AttributeSet);

    // 생성자에서 MaxHP(FLT_MAX)로 초기화 되는지 확인
    TestEqual("MaxHP should be initialized to FLT_MAX", AttributeSet->GetMaxHP(), FLT_MAX);

    return true;
}

// HP 초기값 테스트
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUnitAttributeSetHPTests,
    "P_RD.GAS.Attribute.UnitAttributeSet.HP",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FUnitAttributeSetHPTests::RunTest(const FString& Parameters)
{
    const UUnitAttributeSet* AttributeSet = NewObject<UUnitAttributeSet>();
    TestNotNull("UUnitAttributeSet should be created", AttributeSet);

    // 명시적 초기화가 없으므로 HP 기본값은 0이어야 한다
    TestEqual("HP should be initialized to 0", AttributeSet->GetHP(), 0.f);

    return true;
}
