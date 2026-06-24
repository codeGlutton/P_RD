/*****************************************************************//**
 * @file   TacticalPassiveNthAddAttackTests.cpp
 * @brief  UTacticalPassive_NthAddAttack 자동화 테스트
 * @details
 * N번째 발동마다 공격력 보너스가 터지고 리셋되는지 두 주기(6번째까지) 검증.
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "TAS/Passive/TacticalPassive_NthAddAttack.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "AttributeSet/UnitAttributeSet.h"

namespace
{
	// 패시브를 한 번 발동(Activate + Commit)시키고 대상 공격 보너스(DamagePoint 델타)를 반환
	// sim/live 모두 동일하게 둘 다 수행하므로 분기 없음
	float RunNthAddAttackPass(UTacticalPassive_NthAddAttack* Passive)
	{
		FPassiveActivateContext Ctx;
		FBoardCombatTargetSnapshotData TargetDelta;
		TInstancedStruct<FTacticalPassiveState> Running;   // 빈 러닝 (패시브가 mState에서 시드)

		Passive->ActivatePassive(Ctx, TargetDelta, Running);
		Passive->CommitPassive(Running);

		const float* Value = TargetDelta.mAttributes.Find(UUnitAttributeSet::GetDamagePointAttribute());
		return Value != nullptr ? *Value : 0.f;
	}
}

// 임계값 N마다 보너스가 터지고 리셋되는지 (두 주기 = 6번째까지)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalPassiveNthAddAttackTests,
	"P_RD.TAS.Passive.NthAddAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTacticalPassiveNthAddAttackTests::RunTest(const FString& Parameters)
{
	UTacticalPassive_NthAddAttack* Passive = NewObject<UTacticalPassive_NthAddAttack>();
	if (!TestNotNull(TEXT("패시브 생성"), Passive))
	{
		return false;
	}
	Passive->mThreshold = 3;
	Passive->mAttackBonus = 50.f;

	// 1주기: 1·2 무발동, 3 발동
	TestEqual(TEXT("1번째 무발동"), RunNthAddAttackPass(Passive), 0.f);
	TestEqual(TEXT("2번째 무발동"), RunNthAddAttackPass(Passive), 0.f);
	TestEqual(TEXT("3번째 발동"), RunNthAddAttackPass(Passive), 50.f);

	// 2주기: 리셋되어 4·5 무발동, 6 발동
	TestEqual(TEXT("4번째 무발동(리셋)"), RunNthAddAttackPass(Passive), 0.f);
	TestEqual(TEXT("5번째 무발동"), RunNthAddAttackPass(Passive), 0.f);
	TestEqual(TEXT("6번째 발동"), RunNthAddAttackPass(Passive), 50.f);

	return true;
}
