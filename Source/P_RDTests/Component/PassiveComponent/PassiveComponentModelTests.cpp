/*****************************************************************//**
 * @file   PassiveComponentModelTests.cpp
 * @brief  UPassiveComponentModel 자동화 테스트
 * @details
 * 패시브 추가/제거와 발동 시점 태그별 조회가 올바른지 검증.
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"

#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "TAS/Passive/TacticalPassive_AddAttack.h"
#include "TAS/Passive/TacticalPassive_NthAddAttack.h"

namespace
{
	// 발동 시점 태그를 이름으로 조회 (P_RD의 extern 태그 심볼은 export 안 되므로 모듈 경계용)
	FGameplayTag PassiveTiming(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName));
	}
}

// 추가/제거 및 시점별 조회 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveComponentModelTests,
	"P_RD.Component.PassiveComponentModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveComponentModelTests::RunTest(const FString& Parameters)
{
	const FGameplayTag StartTiming = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartAttacking.Add"));
	const FGameplayTag EndTiming = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndAttacking"));
	const FGameplayTag UnusedTiming = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));

	UPassiveComponentModel* Component = NewObject<UPassiveComponentModel>();
	if (!TestNotNull(TEXT("컴포넌트 생성"), Component))
	{
		return false;
	}

	// 서로 다른 타입의 패시브 2개 추가 (무상태 / 상태형)
	UTacticalPassive* AddPassive = Component->AddPassive(UTacticalPassive_AddAttack::StaticClass());
	UTacticalPassive* NthPassive = Component->AddPassive(UTacticalPassive_NthAddAttack::StaticClass());
	if (!TestNotNull(TEXT("AddAttack 추가"), AddPassive) || !TestNotNull(TEXT("NthAddAttack 추가"), NthPassive))
	{
		return false;
	}

	// 서로 다른 발동 시점 설정 (friend로 protected 접근)
	AddPassive->mTriggerTiming = StartTiming;
	NthPassive->mTriggerTiming = EndTiming;

	// 보유 수 확인
	TestEqual(TEXT("보유 패시브 2개"), Component->GetPassives().Num(), 2);

	// 시점별 조회: 일치분만 반환
	TArray<UTacticalPassive*> StartList = Component->GetPassivesByTiming(StartTiming);
	TestEqual(TEXT("OnStartAttacking_Add 1개"), StartList.Num(), 1);
	if (StartList.Num() == 1)
	{
		TestEqual(TEXT("OnStartAttacking_Add 대상 일치"), StartList[0], AddPassive);
	}

	TArray<UTacticalPassive*> EndList = Component->GetPassivesByTiming(EndTiming);
	TestEqual(TEXT("OnEndAttacking 1개"), EndList.Num(), 1);
	if (EndList.Num() == 1)
	{
		TestEqual(TEXT("OnEndAttacking 대상 일치"), EndList[0], NthPassive);
	}

	// 미설정 시점 조회: 0개
	TArray<UTacticalPassive*> NoneList = Component->GetPassivesByTiming(UnusedTiming);
	TestEqual(TEXT("미설정 시점 0개"), NoneList.Num(), 0);

	// 제거: 성공 후 보유 수 감소, 같은 대상 재제거는 실패
	TestTrue(TEXT("AddAttack 제거 성공"), Component->RemovePassive(AddPassive));
	TestEqual(TEXT("제거 후 1개"), Component->GetPassives().Num(), 1);
	TestFalse(TEXT("이미 제거된 대상 재제거 실패"), Component->RemovePassive(AddPassive));

	return true;
}
