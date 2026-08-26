/*****************************************************************//**
 * @file   TacticalPassiveActivationTests.cpp
 * @brief  패시브 발동/해제 테스트
 * @details
 * 패시브를 타이밍태그로 구동하고, 속성 변화를 체크해서 발동/해제의 정상동작을 검증.
 *  - 즉시형: AddStat + HP(Instant), OnEndTurn 발동 (해제 없음)
 *  - 주기형: AddStat + AttackFactor(Infinite), OnStartTurn 발동 / OnEndTurn 해제
 *  - 스택형: NthAddStat + AttackFactor, OnStartUsingSkill 발동 / OnEndUsingSkill 해제, 매 3타 발동
 *  - 태그형: AddStat + Vulnerability(Instant, 모디파이어 없음), OnEndTurn 발동, Magnitude가 태그 카운트로 부여
 *  - 태그 지속형: AddStat + GrantedTags 목 이펙트(Infinite), OnStartTurn 발동 / OnEndTurn 해제 시 태그 회수
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "TAS/TASAttributeTestsHelper.h"   // FSimulationSubsystemTestAccessor
#include "TAS/Passive/PassiveTestsHelper.h"

#include "TAS/Passive/TacticalPassive_AddStat.h"
#include "TAS/Passive/TacticalPassive_NthAddStat.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"
#include "TAS/Effect/Stat/TacticalEffect_HP.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackFactor.h"
#include "TAS/Effect/Tag/TacticalEffect_Vulnerability.h"
#include "GameplayTagType.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "TAS/Effect/TacticalEffectQuery.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Simulation/RoomInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// 월드 획득 (유닛테스트용 월드)
	UWorld* GetAnyGameWorld()
	{
		if (GEngine == nullptr)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World() != nullptr)
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	// P_RD 모듈의 타이밍태그가 export 안되므로 스트링으로 찾을 수 있게 우회
	FGameplayTag PassiveTiming(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName));
	}

	// 보드액터 목 생성
	UMockBoardActorModel* MakeMockActor(FAutomationTestBase& Test)
	{
		UWorld* World = GetAnyGameWorld();
		if (World == nullptr)
		{
			World = GWorld;
		}
		if (Test.TestNotNull(TEXT("유효한 UWorld"), World) == false)
		{
			return nullptr;
		}

		USimulationSubsystem* SimSubsystem = World->GetSubsystem<USimulationSubsystem>();
		if (Test.TestNotNull(TEXT("USimulationSubsystem"), SimSubsystem) == false)
		{
			return nullptr;
		}

		URoomInstance* RoomInstance = FSimulationSubsystemTestAccessor::GetRoomInstance(SimSubsystem);
		if (Test.TestNotNull(TEXT("RoomInstance"), RoomInstance) == false)
		{
			return nullptr;
		}

		// UTacticalFrameworkModel 등록: 이펙트 적용/해제에 필요
		UTacticalFrameworkModel* FrameworkModel = Cast<UTacticalFrameworkModel>(RoomInstance->mAliveSubsystemModels.FindRef(UTacticalFrameworkModel::StaticClass()));
		if (FrameworkModel == nullptr)
		{
			FrameworkModel = NewObject<UTacticalFrameworkModel>(RoomInstance);
			RoomInstance->mAliveSubsystemModels.Add(UTacticalFrameworkModel::StaticClass(), FrameworkModel);
		}

		UMockBoardActorModel* Actor = NewObject<UMockBoardActorModel>(World);
		Actor->Initialize();
		Actor->BeginPlay();
		return Actor;
	}

	// 유닛테스트용 패시브용 데이터 구성 (이펙트/수치/타이밍태그/임계값 등)
	UStaticPassiveData* MakePassiveData(TSubclassOf<UTacticalEffect> EffectClass, float Magnitude, FGameplayTag Activate, FGameplayTag Deactivate, int32 Threshold)
	{
		UStaticPassiveData* Data = NewObject<UStaticPassiveData>();
		Data->mEffectClass = EffectClass;
		Data->mMagnitude = Magnitude;
		Data->mActivateTimingTag = Activate;
		Data->mDeactivateTimingTag = Deactivate;
		Data->mThreshold = Threshold;
		return Data;
	}

	// ActivatePassive + CommitPassive 호출을 단순하게 래핑
	void DriveTiming(UTacticalPassive* Passive, const FGameplayTag& Timing, const FPassiveActivateContext& Ctx)
	{
		TInstancedStruct<FDynamicPassiveData> Work;
		Passive->ActivatePassive(Timing, Ctx, Work);
		Passive->CommitPassive(Work);
	}
}

/**
 * @brief 즉시형 테스트
 * AddStat + HP(Instant), OnEndTurn 발동
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveInstantHealTest,
	"P_RD.TAS.Passive.InstantHeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveInstantHealTest::RunTest(const FString& Parameters)
{
	UMockBoardActorModel* Actor = MakeMockActor(*this);
	if (Actor == nullptr)
	{
		return false;
	}
	UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();

	// 최대체력, 현재체력 설정
	Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetMaxHPAttribute(), 100.f);
	Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute(), 50.f);

	const FGameplayTag OnStartTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));
	const FGameplayTag OnEndTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndTurn"));
	// 패시브 값 설정: 체력 +5 회복
	UStaticPassiveData* Data = MakePassiveData(UTacticalEffect_HP::StaticClass(), 5.f, OnEndTurn, FGameplayTag(), 0);

	UTacticalPassive_AddStat* Passive = NewObject<UTacticalPassive_AddStat>();
	Passive->SetStaticData(Data);

	FPassiveActivateContext Ctx;
	Ctx.mOwner = Actor;
	Ctx.mTargets.Add(Actor);

	// OnStartTurn에 발동 테스트: 반응 안해야 됨
	DriveTiming(Passive, OnStartTurn, Ctx);
	TestEqual(TEXT("타이밍태그가 없으므로 발동 안 함: 체력은 50 유지"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHPAttribute()), 50.f);

	// OnEndTurn에 발동 테스트: +5 즉시 회복 → HP 55
	DriveTiming(Passive, OnEndTurn, Ctx);
	TestEqual(TEXT("OnEndTurn에 발동 함: 체력은 55로 증가"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHPAttribute()), 55.f);

	return true;
}

/**
 * @brief 주기형 테스트
 * AddStat + AttackFactor(Infinite), OnStartTurn 발동 / OnEndTurn 해제
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveInfiniteBuffTest,
	"P_RD.TAS.Passive.InfiniteBuff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveInfiniteBuffTest::RunTest(const FString& Parameters)
{
	UMockBoardActorModel* Actor = MakeMockActor(*this);
	if (Actor == nullptr)
	{
		return false;
	}
	UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();

	// 패시브 적용 전 AttackFactor 10
	Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);

	const FGameplayTag OnStartTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));
	const FGameplayTag OnEndTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndTurn"));
	// 패시브 값 설정: AttackFactor +5
	UStaticPassiveData* Data = MakePassiveData(UTacticalEffect_AttackFactor_AddBase::StaticClass(), 5.f, OnStartTurn, OnEndTurn, 0);

	UTacticalPassive_AddStat* Passive = NewObject<UTacticalPassive_AddStat>();
	Passive->SetStaticData(Data);

	FPassiveActivateContext Ctx;
	Ctx.mOwner = Actor;
	Ctx.mTargets.Add(Actor);

	// 발동 테스트: OnStartTurn에 기본 AttackFactor에 패시브 AttackFactor +5 합산
	DriveTiming(Passive, OnStartTurn, Ctx);
	TestEqual(TEXT("OnStartTurn에 발동 함: AttackFactor는 15로 증가"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 15.f);

	// 해제 테스트: OnEndTurn에 패시브 AttackFactor +5 감산
	DriveTiming(Passive, OnEndTurn, Ctx);
	TestEqual(TEXT("OnEndTurn에 해제 함: AttackFactor는 10으로 감소"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 10.f);

	return true;
}

/**
 * @brief 스택형 테스트
 * NthAddStat + AttackFactor, OnStartUsingSkill 발동 / OnEndUsingSkill 해제, 매 3번째 타격때만 발동
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveStackTest,
	"P_RD.TAS.Passive.Stack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveStackTest::RunTest(const FString& Parameters)
{
	UMockBoardActorModel* Actor = MakeMockActor(*this);
	if (Actor == nullptr)
	{
		return false;
	}
	UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();

	// 패시브 적용 전 AttackFactor 10
	Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);

	const FGameplayTag OnStartUsingSkill = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartUsingSkill"));
	const FGameplayTag OnEndUsingSkill = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndUsingSkill"));
	// 패시브 값 설정: AttackFactor +5, 임계치 3(매 3번째에만 패시브 효과 발동)
	UStaticPassiveData* Data = MakePassiveData(UTacticalEffect_AttackFactor_AddBase::StaticClass(), 5.f, OnStartUsingSkill, OnEndUsingSkill, 3);

	UTacticalPassive_NthAddStat* Passive = NewObject<UTacticalPassive_NthAddStat>();
	Passive->SetStaticData(Data);

	FPassiveActivateContext Ctx;
	Ctx.mOwner = Actor;
	Ctx.mTargets.Add(Actor);

	// 총 8번 타격하고 발동/해제 여부를 검증
	for (int32 Hit = 1; Hit <= 8; ++Hit)
	{
		// 기대 발동 여부: 3번째마다 발동
		const bool bExpectFire = (Hit % 3 == 0);
		// 기대 AttackFactor: 발동했으면 15, 아니면 10
		const float ExpectedOnHit = bExpectFire ? 15.f : 10.f;

		DriveTiming(Passive, OnStartUsingSkill, Ctx);
		TestEqual(FString::Printf(TEXT("%d번째 타격 시작, 기대 AttackFactor %.0f"), Hit, ExpectedOnHit), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), ExpectedOnHit);

		DriveTiming(Passive, OnEndUsingSkill, Ctx);
		TestEqual(FString::Printf(TEXT("%d번째 타격 종료, 기대 AttackFactor 10"), Hit), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 10.f);
	}

	return true;
}

/**
 * @brief 다중 타겟 테스트
 * AddStat + AttackFactor(Infinite), 대상 2명, OnStartTurn 발동 / OnEndTurn 해제
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveMultiTargetBuffTest,
	"P_RD.TAS.Passive.MultiTargetBuff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveMultiTargetBuffTest::RunTest(const FString& Parameters)
{
	// 시전자(플레이어) 1명 + 타겟 2명 생성
	UMockBoardActorModel* Player = MakeMockActor(*this);
	UMockBoardActorModel* Target1 = MakeMockActor(*this);
	UMockBoardActorModel* Target2 = MakeMockActor(*this);
	if (Player == nullptr || Target1 == nullptr || Target2 == nullptr)
	{
		return false;
	}
	UAttributeSetComponentModel* TargetComp1 = Target1->GetAttributeComponentModel();
	UAttributeSetComponentModel* TargetComp2 = Target2->GetAttributeComponentModel();

	// 두 타겟 모두 적용 전 AttackFactor 10
	TargetComp1->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);
	TargetComp2->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);

	const FGameplayTag OnStartTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));
	const FGameplayTag OnEndTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndTurn"));
	// 패시브 값 설정: AttackFactor +5
	UStaticPassiveData* Data = MakePassiveData(UTacticalEffect_AttackFactor_AddBase::StaticClass(), 5.f, OnStartTurn, OnEndTurn, 0);

	UTacticalPassive_AddStat* Passive = NewObject<UTacticalPassive_AddStat>();
	Passive->SetStaticData(Data);

	// 시전자는 Player, 대상은 Target1/Target2
	FPassiveActivateContext Ctx;
	Ctx.mOwner = Player;
	Ctx.mTargets.Add(Target1);
	Ctx.mTargets.Add(Target2);

	// 발동 테스트: OnStartTurn에 두 타겟 모두 +5 (핸들 2개 적용)
	DriveTiming(Passive, OnStartTurn, Ctx);
	TestEqual(TEXT("OnStartTurn 발동: 타겟1 AttackFactor는 15로 증가"), TargetComp1->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 15.f);
	TestEqual(TEXT("OnStartTurn 발동: 타겟2 AttackFactor는 15로 증가"), TargetComp2->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 15.f);

	// 해제 테스트: OnEndTurn에 두 타겟 모두 -5 (핸들 2개 제거)
	DriveTiming(Passive, OnEndTurn, Ctx);
	TestEqual(TEXT("OnEndTurn 해제: 타겟1 AttackFactor는 10으로 감소"), TargetComp1->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 10.f);
	TestEqual(TEXT("OnEndTurn 해제: 타겟2 AttackFactor는 10으로 감소"), TargetComp2->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 10.f);

	return true;
}

/**
 * @brief 태그형 테스트
 * AddStat + Vulnerability(Instant, 모디파이어 없음), OnEndTurn 발동 (해제 없음)
 * Magnitude가 태그 카운트로 부여되는지 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveAddTagTest,
	"P_RD.TAS.Passive.AddTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveAddTagTest::RunTest(const FString& Parameters)
{
	UMockBoardActorModel* Actor = MakeMockActor(*this);
	if (Actor == nullptr)
	{
		return false;
	}
	UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();

	// 검증 대상 태그: 취약
	const FGameplayTag VulnerabilityTag = EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Vulnerability;
	TestFalse(TEXT("적용 전: 취약 태그 없음"), Comp->HasMatchingGameplayTag(VulnerabilityTag));

	const FGameplayTag OnStartTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));
	const FGameplayTag OnEndTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndTurn"));
	// 패시브 값 설정: 취약 3 (태그형 이펙트는 Magnitude가 태그 카운트로 들어감)
	UStaticPassiveData* Data = MakePassiveData(UTacticalEffect_GetVulnerability::StaticClass(), 3.f, OnEndTurn, FGameplayTag(), 0);

	UTacticalPassive_AddStat* Passive = NewObject<UTacticalPassive_AddStat>();
	Passive->SetStaticData(Data);

	FPassiveActivateContext Ctx;
	Ctx.mOwner = Actor;
	Ctx.mTargets.Add(Actor);

	// 타이밍 불일치: 발동 안 함
	DriveTiming(Passive, OnStartTurn, Ctx);
	TestFalse(TEXT("타이밍태그가 없으므로 발동 안 함: 취약 태그 없음"), Comp->HasMatchingGameplayTag(VulnerabilityTag));

	// OnEndTurn에 발동: 취약 3스택 부여
	// 상태이상은 스택형 지속 효과라 태그는 1개만 붙고, 수치는 스택 수로 확인
	DriveTiming(Passive, OnEndTurn, Ctx);
	TestTrue(TEXT("OnEndTurn에 발동 함: 취약 태그 보유"), Comp->HasMatchingGameplayTag(VulnerabilityTag));
	const FTacticalEffectQuery VulnerabilityQuery = FTacticalEffectQuery::MakeQuery_MatchAnyEffectTags(FGameplayTagContainer(VulnerabilityTag));
	TestEqual(TEXT("취약 스택은 3"), Comp->GetAggregatedStackCount(VulnerabilityQuery), 3);

	return true;
}

/**
 * @brief 태그 지속형 테스트
 * AddStat + GrantedTags 목 이펙트(Infinite), OnStartTurn 발동 / OnEndTurn 해제
 * 핸들 해제 시 부여 태그가 회수되는지(패시브 해제 계약) 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveGrantedTagBuffTest,
	"P_RD.TAS.Passive.GrantedTagBuff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveGrantedTagBuffTest::RunTest(const FString& Parameters)
{
	UMockBoardActorModel* Actor = MakeMockActor(*this);
	if (Actor == nullptr)
	{
		return false;
	}
	UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();

	// 검증 대상 태그: 취약 (목 이펙트의 GrantedTags)
	const FGameplayTag VulnerabilityTag = EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Vulnerability;
	TestFalse(TEXT("적용 전: 취약 태그 없음"), Comp->HasMatchingGameplayTag(VulnerabilityTag));

	const FGameplayTag OnStartTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));
	const FGameplayTag OnEndTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndTurn"));
	// 패시브 값 설정: GrantedTags는 카운트 고정(+1)이라 Magnitude는 0 검사만 통과하면 됨
	UStaticPassiveData* Data = MakePassiveData(UMockGrantedTagTacticalEffect::StaticClass(), 1.f, OnStartTurn, OnEndTurn, 0);

	UTacticalPassive_AddStat* Passive = NewObject<UTacticalPassive_AddStat>();
	Passive->SetStaticData(Data);

	FPassiveActivateContext Ctx;
	Ctx.mOwner = Actor;
	Ctx.mTargets.Add(Actor);

	// 발동 테스트: OnStartTurn에 취약 태그 부여
	DriveTiming(Passive, OnStartTurn, Ctx);
	TestTrue(TEXT("OnStartTurn에 발동 함: 취약 태그 보유"), Comp->HasMatchingGameplayTag(VulnerabilityTag));

	// 해제 테스트: OnEndTurn에 핸들 제거와 함께 취약 태그 회수
	DriveTiming(Passive, OnEndTurn, Ctx);
	TestFalse(TEXT("OnEndTurn에 해제 함: 취약 태그 회수"), Comp->HasMatchingGameplayTag(VulnerabilityTag));

	return true;
}

/**
 * @brief 수량 조건과 자격 조건이 추가된 다중 타겟 테스트
 * AddStat + AttackFactor, 자격 조건 HP<50. 타겟 2명의 자격 수 × Any/All로 발동 판정.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveQuantifierMultiTargetBuffTest,
	"P_RD.TAS.Passive.QuantifierMultiTargetBuff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveQuantifierMultiTargetBuffTest::RunTest(const FString& Parameters)
{
	const FGameplayTag OnStartTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));

	// 수량조건, 타겟1 HP, 타겟2 HP로 케이스 하나를 만듦.
	// HP<50이면 자격 조건 만족.
	auto RunCase = [&](EPassiveTargetQuantifier Quantifier, float HP1, float HP2) -> float
	{
		// 시전자 1명 + 타겟 2명 생성
		UMockBoardActorModel* Player = MakeMockActor(*this);
		UMockBoardActorModel* Target1 = MakeMockActor(*this);
		UMockBoardActorModel* Target2 = MakeMockActor(*this);
		if (Player == nullptr || Target1 == nullptr || Target2 == nullptr)
		{
			return -1.f;
		}
		UAttributeSetComponentModel* TargetComp1 = Target1->GetAttributeComponentModel();
		UAttributeSetComponentModel* TargetComp2 = Target2->GetAttributeComponentModel();

		// HP로 자격을 조절, AttackFactor는 기준 10
		TargetComp1->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute(), HP1);
		TargetComp2->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute(), HP2);
		TargetComp1->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);
		TargetComp2->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);

		// 값: AttackFactor +5, 수량조건 지정, 발동 시점 OnStartTurn (해제 없음)
		UStaticPassiveData* Data = MakePassiveData(UTacticalEffect_AttackFactor_AddBase::StaticClass(), 5.f, OnStartTurn, FGameplayTag(), 0);
		Data->mTargetQuantifier = Quantifier;

		UMockConditionAddStatPassive* Passive = NewObject<UMockConditionAddStatPassive>();
		Passive->SetStaticData(Data);

		// 자격 판정이 읽을 타겟 스냅샷 준비 (DriveTiming 동안 살아있어야 함)
		UBoardCombatTargetSnapshotData* Snapshot1 = Target1->MakeSnapshotData();
		UBoardCombatTargetSnapshotData* Snapshot2 = Target2->MakeSnapshotData();

		FPassiveActivateContext Ctx;
		Ctx.mOwner = Player;
		Ctx.mTargets.Add(Target1);
		Ctx.mTargets.Add(Target2);
		Ctx.mTargetSnapshots.Add(Snapshot1);
		Ctx.mTargetSnapshots.Add(Snapshot2);

		DriveTiming(Passive, OnStartTurn, Ctx);

		// 발동했으면 15, 미발동이면 10
		return TargetComp1->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute());
	};

	// Any: 자격 1명(HP 40)이라도 있으면 발동
	TestEqual(TEXT("Any/자격1 → 발동"), RunCase(EPassiveTargetQuantifier::Any, 40.f, 80.f), 15.f);
	// Any: 자격 0명이면 미발동
	TestEqual(TEXT("Any/자격0 → 미발동"), RunCase(EPassiveTargetQuantifier::Any, 80.f, 90.f), 10.f);
	// All: 전원 자격이면 발동
	TestEqual(TEXT("All/자격2 → 발동"), RunCase(EPassiveTargetQuantifier::All, 40.f, 30.f), 15.f);
	// All: 일부만 자격이면 미발동
	TestEqual(TEXT("All/자격1 → 미발동"), RunCase(EPassiveTargetQuantifier::All, 40.f, 80.f), 10.f);

	return true;
}
