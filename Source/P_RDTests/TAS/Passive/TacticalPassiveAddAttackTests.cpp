/*****************************************************************//**
 * @file   TacticalPassiveAddAttackTests.cpp
 * @brief  UTacticalPassive_AddAttack 자동화 테스트
 * @details
 * 무상태 패시브가 TargetDelta의 공격력(DamagePoint)에 보너스를 누적하는지 검증.
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "TAS/Passive/TacticalPassive_AddAttack.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "AttributeSet/UnitAttributeSet.h"

// AddAttack 패시브가 DamagePoint 델타에 보너스를 누적하는지 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalPassiveAddAttackTests,
	"P_RD.TAS.Passive.AddAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTacticalPassiveAddAttackTests::RunTest(const FString& Parameters)
{
	// 패시브 생성 + 보너스 설정
	UTacticalPassive_AddAttack* Passive = NewObject<UTacticalPassive_AddAttack>();
	TestNotNull(TEXT("패시브 생성"), Passive);
	if (Passive == nullptr)
	{
		return false;
	}
	Passive->mAttackBonus = 10.f;

	// 입력/출력 준비 (AddAttack은 Ctx/State 미사용)
	FPassiveActivateContext Ctx;
	FBoardCombatTargetSnapshotData TargetDelta;
	TInstancedStruct<FTacticalPassiveState> State;

	// 계산
	Passive->ActivatePassive(Ctx, TargetDelta, State);

	// DamagePoint 델타가 보너스만큼 누적됐는지 확인
	const float* Value = TargetDelta.mAttributes.Find(UUnitAttributeSet::GetDamagePointAttribute());
	TestNotNull(TEXT("DamagePoint 델타 존재"), Value);
	if (Value != nullptr)
	{
		TestEqual(TEXT("DamagePoint 델타 == 보너스"), *Value, 10.f);
	}

	return true;
}
