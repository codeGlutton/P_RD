/*****************************************************************//**
 * @file   TacticalPassiveAddAttackPointTests.cpp
 * @brief  UTacticalPassive_AddAttackPoint 자동화 테스트
 * @details
 * 무상태 패시브가 TargetDelta의 공격력(AttackPoint)에 보너스를 누적하는지 검증.
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "TAS/Passive/TacticalPassive_AddAttackPoint.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "AttributeSet/UnitAttributeSet.h"

// AddAttackPoint 패시브가 AttackPoint 델타에 보너스를 누적하는지 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalPassiveAddAttackPointTests,
	"P_RD.TAS.Passive.AddAttackPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTacticalPassiveAddAttackPointTests::RunTest(const FString& Parameters)
{
	// 패시브 생성 + 보너스 설정
	UTacticalPassive_AddAttackPoint* Passive = NewObject<UTacticalPassive_AddAttackPoint>();
	TestNotNull(TEXT("패시브 생성"), Passive);
	if (Passive == nullptr)
	{
		return false;
	}
	Passive->mAttackBonus = 10.f;

	// 입력/출력 준비 (AddAttackPoint은 Ctx/State 미사용)
	FPassiveActivateContext Ctx;
	FBoardCombatTargetSnapshotData TargetDelta;
	TInstancedStruct<FDynamicPassiveData> State;

	// 계산만 검증 (적용/이펙트는 별도 효과 테스트).
	// EvaluatePassive는 protected이고 friend는 상속/서브클래스 override에 적용 안 되므로 베이스 타입으로 호출.
	UTacticalPassive* Base = Passive;
	Base->EvaluatePassive(Ctx, TargetDelta, State);

	// AttackPoint 델타가 보너스만큼 누적됐는지 확인
	const float* Value = TargetDelta.mAttributes.Find(UUnitAttributeSet::GetAttackPointAttribute());
	TestNotNull(TEXT("AttackPoint 델타 존재"), Value);
	if (Value != nullptr)
	{
		TestEqual(TEXT("AttackPoint 델타 == 보너스"), *Value, 10.f);
	}

	return true;
}
