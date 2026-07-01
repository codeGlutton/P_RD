/*****************************************************************//**
 * @file   TacticalPassiveAddStatTests.cpp
 * @brief  UTacticalPassive_AddStat 자동화 테스트
 * @details
 * 무상태 패시브가 TargetDelta의 이펙트 속성(EffectClass가 정의)에 데이터 수치(Magnitude)를 누적하는지 검증.
 * 속성/수치는 정적 데이터(UStaticPassiveData)에서 읽음(데이터 구동).
 * @author 이문환
 * @date   2026-06-28
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "TAS/Passive/TacticalPassive_AddStat.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackFactor.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "AttributeSet/UnitAttributeSet.h"

// AddStat 패시브가 이펙트 속성(여기선 AttackFactor) 델타에 데이터 수치를 누적하는지 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalPassiveAddStatTests,
	"P_RD.TAS.Passive.AddStat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTacticalPassiveAddStatTests::RunTest(const FString& Parameters)
{
	// 데이터 구동 패시브 구성 (EffectClass=AttackFactor → 속성 AttackFactor, Magnitude=10)
	UStaticPassiveData* Data = NewObject<UStaticPassiveData>();
	Data->mEffectClass = UTacticalEffect_AttackFactor::StaticClass();
	Data->mMagnitude = 10.f;

	UTacticalPassive_AddStat* Passive = NewObject<UTacticalPassive_AddStat>();
	TestNotNull(TEXT("패시브 생성"), Passive);
	if (Passive == nullptr)
	{
		return false;
	}
	Passive->SetStaticData(Data);

	// 입력/출력 준비 (AddStat은 Ctx/State 미사용)
	FPassiveActivateContext Ctx;
	FBoardCombatTargetSnapshotData TargetDelta;
	TInstancedStruct<FDynamicPassiveData> State;

	// 계산만 검증 (적용/이펙트는 별도 효과 테스트).
	// EvaluateActivate는 protected이고 friend는 서브클래스 override에 적용 안 되므로 베이스 타입으로 호출.
	UTacticalPassive* Base = Passive;
	const bool bApplied = Base->EvaluateActivate(Ctx, State, TargetDelta);

	// 무상태 고정 가산은 시점이 오면 항상 적용
	TestTrue(TEXT("적용(true) 반환"), bApplied);

	// AttackFactor 델타가 수치만큼 누적됐는지 확인
	const float* Value = TargetDelta.mAttributes.Find(UUnitAttributeSet::GetAttackFactorAttribute());
	TestNotNull(TEXT("AttackFactor 델타 존재"), Value);
	if (Value != nullptr)
	{
		TestEqual(TEXT("AttackFactor 델타 == 수치"), *Value, 10.f);
	}

	return true;
}
