/*****************************************************************//**
 * @file   TacticalPassiveNthAddStatTests.cpp
 * @brief  UTacticalPassive_NthAddStat 자동화 테스트
 * @details
 * 계산(EvaluatePassive): 이번 차수(완료+1)가 임계값이면 발동, 러닝본(NextState)에 발동=리셋(0)/미발동=차수 전진 기록.
 * 커밋(CommitPassive): 러닝본을 mState에 그대로 확정(리셋은 Evaluate 담당).
 * 속성/수치/임계는 정적 데이터(UStaticPassiveData)에서 읽음(데이터 구동).
 * @author 이문환
 * @date   2026-06-28
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "TAS/Passive/TacticalPassive_NthAddStat.h"
#include "TAS/Passive/DynamicPassiveData_NthCounter.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackPoint.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "AttributeSet/UnitAttributeSet.h"

namespace
{
	// 데이터 구동 NthAddStat 패시브 생성 (EffectClass=AttackPoint → 속성 AttackPoint, Magnitude=50, Threshold=3)
	UTacticalPassive_NthAddStat* MakeNthAddStat()
	{
		UStaticPassiveData* Data = NewObject<UStaticPassiveData>();
		Data->mEffectClass = UTacticalEffect_AttackPoint::StaticClass();
		Data->mMagnitude = 50.f;
		Data->mThreshold = 3;

		UTacticalPassive_NthAddStat* Passive = NewObject<UTacticalPassive_NthAddStat>();
		Passive->SetStaticData(Data);
		return Passive;
	}
}

// ===== 계산: EvaluatePassive가 임계 차수에서 발동하고, 러닝본(NextState)을 전진/리셋시키는지 =====
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalPassiveNthAddStatCalcTests,
	"P_RD.TAS.Passive.NthAddStat.Calc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTacticalPassiveNthAddStatCalcTests::RunTest(const FString& Parameters)
{
	UTacticalPassive_NthAddStat* Passive = MakeNthAddStat();
	if (!TestNotNull(TEXT("패시브 생성"), Passive))
	{
		return false;
	}

	// 러닝본 하나를 이어서 굴리며 사이클 검증 (threshold=3 → 3·6회차 발동)
	TInstancedStruct<FDynamicPassiveData> Running;
	Running.InitializeAs<FDynamicPassiveData_NthCounter>();   // 완료 0에서 시작

	// 한 회차 계산 → 이번 회차 발동 여부 반환 (러닝본은 NextState로 갱신되어 다음 회차로 이어짐)
	// EvaluatePassive는 protected라 friend(테스트 클래스)로 직접 호출
	auto Step = [&]() -> bool
	{
		FPassiveActivateContext Ctx;
		FBoardCombatTargetSnapshotData Delta;

		// friend는 서브클래스 override에 적용 안 되므로 베이스 타입으로 호출
		UTacticalPassive* Base = Passive;
		Base->EvaluatePassive(Ctx, Delta, Running);

		const float* V = Delta.mAttributes.Find(UUnitAttributeSet::GetAttackPointAttribute());
		return (V != nullptr) && FMath::IsNearlyEqual(*V, 50.f);
	};

	// 7회차 순차 실행: 발동된 회차(1-base) 수집
	TArray<int32> FiredAt;
	for (int32 Turn = 1; Turn <= 7; ++Turn)
	{
		if (Step())
		{
			FiredAt.Add(Turn);
		}
	}

	// 7회 중 정확히 2번(3·6회차) 발동
	TestEqual(TEXT("7회 중 발동 횟수 2"), FiredAt.Num(), 2);
	if (FiredAt.Num() == 2)
	{
		TestEqual(TEXT("첫 발동 = 3회차"), FiredAt[0], 3);
		TestEqual(TEXT("둘째 발동 = 6회차"), FiredAt[1], 6);
	}

	// 7회차 후 완료횟수 1 (6회차 리셋→0, 7회차 전진→1)
	const int32 FinalCount = Running.Get<FDynamicPassiveData_NthCounter>().mCount;
	TestEqual(TEXT("7회차 후 완료횟수 1"), FinalCount, 1);

	return true;
}

// ===== 커밋: CommitPassive가 러닝본을 mState에 그대로 확정하는지 (리셋은 Evaluate 담당) =====
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalPassiveNthAddStatCommitTests,
	"P_RD.TAS.Passive.NthAddStat.Commit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTacticalPassiveNthAddStatCommitTests::RunTest(const FString& Parameters)
{
	UTacticalPassive_NthAddStat* Passive = MakeNthAddStat();
	if (!TestNotNull(TEXT("패시브 생성"), Passive))
	{
		return false;
	}

	// 러닝 카운터를 RunningCount로 만들어 커밋 → 커밋된 mState 카운터 반환
	// mState는 protected라 friend(테스트 클래스)로 직접 접근
	auto Commit = [&](int32 RunningCount) -> int32
	{
		TInstancedStruct<FDynamicPassiveData> Running;
		Running.InitializeAs<FDynamicPassiveData_NthCounter>();
		Running.GetMutable<FDynamicPassiveData_NthCounter>().mCount = RunningCount;

		Passive->CommitPassive(Running);

		// mState는 protected라 베이스 타입으로 접근 (friend 적용)
		const UTacticalPassive* Base = Passive;
		const TInstancedStruct<FDynamicPassiveData>& Committed = Base->mState;
		return Committed.IsValid() ? Committed.Get<FDynamicPassiveData_NthCounter>().mCount : -1;
	};

	// 커밋은 러닝본을 그대로 mState에 확정 (리셋은 Evaluate가 NextState에 이미 반영하므로 여기선 안 함)
	TestEqual(TEXT("커밋: 러닝본 1 -> mState 1"), Commit(1), 1);
	TestEqual(TEXT("커밋: 러닝본 2 -> mState 2 (리셋 없음)"), Commit(2), 2);

	return true;
}
