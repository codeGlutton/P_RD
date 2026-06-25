/*****************************************************************//**
 * @file   TacticalPassiveNthAddAttackTests.cpp
 * @brief  UTacticalPassive_NthAddAttack 자동화 테스트
 * @details
 * 계산(EvaluatePassive): 임계값에서 발동 + 러닝 카운터 전진(리셋 안 함).
 * 커밋(CommitPassive): 러닝 카운터를 mState에 반영 + 임계 도달 시 0으로 리셋.
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "TAS/Passive/TacticalPassive_NthAddAttack.h"
#include "TAS/Passive/TacticalPassiveState_NthCounter.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "AttributeSet/UnitAttributeSet.h"

// ===== 계산: EvaluatePassive가 임계값에서 발동하고 러닝 카운터를 전진(리셋 X)시키는지 =====
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalPassiveNthAddAttackCalcTests,
	"P_RD.TAS.Passive.NthAddAttack.Calc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTacticalPassiveNthAddAttackCalcTests::RunTest(const FString& Parameters)
{
	UTacticalPassive_NthAddAttack* Passive = NewObject<UTacticalPassive_NthAddAttack>();
	if (!TestNotNull(TEXT("패시브 생성"), Passive))
	{
		return false;
	}
	Passive->mThreshold = 3;
	Passive->mAttackBonus = 50.f;

	// 러닝 카운터를 StartCount로 세팅해 한 번 계산 → (델타, 계산 후 카운터) 반환
	// EvaluatePassive는 protected라 friend(테스트 클래스)로 직접 호출
	auto Eval = [&](int32 StartCount, float& OutDelta, int32& OutCount)
	{
		FPassiveActivateContext Ctx;
		FBoardCombatTargetSnapshotData Delta;
		TInstancedStruct<FTacticalPassiveState> Running;
		Running.InitializeAs<FTacticalPassiveState_NthCounter>();
		Running.GetMutable<FTacticalPassiveState_NthCounter>().mCount = StartCount;

		// friend는 서브클래스 override에 적용 안 되므로 베이스 타입으로 호출
		UTacticalPassive* Base = Passive;
		Base->EvaluatePassive(Ctx, Delta, Running);

		const float* V = Delta.mAttributes.Find(UUnitAttributeSet::GetDamagePointAttribute());
		OutDelta = (V != nullptr) ? *V : 0.f;
		OutCount = Running.Get<FTacticalPassiveState_NthCounter>().mCount;
	};

	float Delta = 0.f;
	int32 Count = 0;

	// 카운터 0 -> 1, 임계 미만이라 무발동
	Eval(0, Delta, Count);
	TestEqual(TEXT("임계 미만 무발동"), Delta, 0.f);
	TestEqual(TEXT("카운터 0 -> 1"), Count, 1);

	// 카운터 2 -> 3, 임계 도달이라 발동 (리셋은 커밋 몫이라 계산 단계선 3 유지)
	Eval(2, Delta, Count);
	TestEqual(TEXT("임계 도달 발동"), Delta, 50.f);
	TestEqual(TEXT("카운터 2 -> 3 (계산은 리셋 안 함)"), Count, 3);

	return true;
}

// ===== 커밋: CommitPassive가 러닝 카운터를 mState에 반영하고 임계 도달 시 0으로 리셋하는지 =====
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalPassiveNthAddAttackCommitTests,
	"P_RD.TAS.Passive.NthAddAttack.Commit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTacticalPassiveNthAddAttackCommitTests::RunTest(const FString& Parameters)
{
	UTacticalPassive_NthAddAttack* Passive = NewObject<UTacticalPassive_NthAddAttack>();
	if (!TestNotNull(TEXT("패시브 생성"), Passive))
	{
		return false;
	}
	Passive->mThreshold = 3;
	Passive->mAttackBonus = 50.f;

	// 러닝 카운터를 RunningCount로 만들어 커밋 → 커밋된 mState 카운터 반환
	// mState는 protected라 friend(테스트 클래스)로 직접 접근
	auto Commit = [&](int32 RunningCount) -> int32
	{
		TInstancedStruct<FTacticalPassiveState> Running;
		Running.InitializeAs<FTacticalPassiveState_NthCounter>();
		Running.GetMutable<FTacticalPassiveState_NthCounter>().mCount = RunningCount;

		Passive->CommitPassive(Running);

		// mState는 protected라 베이스 타입으로 접근 (friend 적용)
		const UTacticalPassive* Base = Passive;
		const TInstancedStruct<FTacticalPassiveState>& Committed = Base->mState;
		return Committed.IsValid() ? Committed.Get<FTacticalPassiveState_NthCounter>().mCount : -1;
	};

	// 임계 미만(1)은 그대로 커밋
	TestEqual(TEXT("커밋: 카운터 1 유지"), Commit(1), 1);
	// 임계 도달(3)은 0으로 리셋
	TestEqual(TEXT("커밋: 카운터 3 -> 리셋 0"), Commit(3), 0);

	return true;
}
