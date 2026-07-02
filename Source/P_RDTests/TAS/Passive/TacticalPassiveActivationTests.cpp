/*****************************************************************//**
 * @file   TacticalPassiveActivationTests.cpp
 * @brief  패시브 발동/해제 테스트
 * @details
 * 패시브를 타이밍태그로 구동하고, 속성 변화를 체크해서 발동/해제의 정상동작을 검증.
 *  - 즉시형: AddStat + HP(Instant), OnEndTurn 발동 (해제 없음)
 *  - 주기형: AddStat + AttackFactor(Infinite), OnStartTurn 발동 / OnEndTurn 해제
 *  - 스택형: NthAddStat + AttackFactor, OnStartUsingSkill 발동 / OnEndUsingSkill 해제, 매 3타 발동
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
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
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
	Comp->SetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute(), 100.f);
	Comp->SetAttributeBaseValue(UUnitAttributeSet::GetHPAttribute(), 50.f);

	const FGameplayTag OnStartTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));
	const FGameplayTag OnEndTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndTurn"));
	// 패시브 값 설정: 체력 +5 회복
	UStaticPassiveData* Data = MakePassiveData(UTacticalEffect_HP::StaticClass(), 5.f, OnEndTurn, FGameplayTag(), 0);

	UTacticalPassive_AddStat* Passive = NewObject<UTacticalPassive_AddStat>();
	Passive->SetStaticData(Data);

	FPassiveActivateContext Ctx;
	Ctx.mOwner = Actor;
	Ctx.mTarget = Actor;

	// OnStartTurn에 발동 테스트: 반응 안해야 됨
	DriveTiming(Passive, OnStartTurn, Ctx);
	TestEqual(TEXT("타이밍태그가 없으므로 발동 안 함: 체력은 50 유지"), Comp->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()), 50.f);

	// OnEndTurn에 발동 테스트: +5 즉시 회복 → HP 55
	DriveTiming(Passive, OnEndTurn, Ctx);
	TestEqual(TEXT("OnEndTurn에 발동 함: 체력은 55로 증가"), Comp->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()), 55.f);

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
	Comp->SetAttributeBaseValue(UUnitAttributeSet::GetAttackFactorAttribute(), 10.f);

	const FGameplayTag OnStartTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));
	const FGameplayTag OnEndTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndTurn"));
	// 패시브 값 설정: AttackFactor +5
	UStaticPassiveData* Data = MakePassiveData(UTacticalEffect_AttackFactor::StaticClass(), 5.f, OnStartTurn, OnEndTurn, 0);

	UTacticalPassive_AddStat* Passive = NewObject<UTacticalPassive_AddStat>();
	Passive->SetStaticData(Data);

	FPassiveActivateContext Ctx;
	Ctx.mOwner = Actor;
	Ctx.mTarget = Actor;

	// 발동 테스트: OnStartTurn에 기본 AttackFactor에 패시브 AttackFactor +5 합산
	DriveTiming(Passive, OnStartTurn, Ctx);
	TestEqual(TEXT("OnStartTurn에 발동 함: AttackFactor는 15로 증가"), Comp->GetAttributeCurrentValue(UUnitAttributeSet::GetAttackFactorAttribute()), 15.f);

	// 해제 테스트: OnEndTurn에 패시브 AttackFactor +5 감산
	DriveTiming(Passive, OnEndTurn, Ctx);
	TestEqual(TEXT("OnEndTurn에 해제 함: AttackFactor는 10으로 감소"), Comp->GetAttributeCurrentValue(UUnitAttributeSet::GetAttackFactorAttribute()), 10.f);

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
	Comp->SetAttributeBaseValue(UUnitAttributeSet::GetAttackFactorAttribute(), 10.f);

	const FGameplayTag OnStartUsingSkill = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartUsingSkill"));
	const FGameplayTag OnEndUsingSkill = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndUsingSkill"));
	// 패시브 값 설정: AttackFactor +5, 임계치 3(매 3번째에만 패시브 효과 발동)
	UStaticPassiveData* Data = MakePassiveData(UTacticalEffect_AttackFactor::StaticClass(), 5.f, OnStartUsingSkill, OnEndUsingSkill, 3);

	UTacticalPassive_NthAddStat* Passive = NewObject<UTacticalPassive_NthAddStat>();
	Passive->SetStaticData(Data);

	FPassiveActivateContext Ctx;
	Ctx.mOwner = Actor;
	Ctx.mTarget = Actor;

	// 총 8번 타격하고 발동/해제 여부를 검증
	for (int32 Hit = 1; Hit <= 8; ++Hit)
	{
		// 기대 발동 여부: 3번째마다 발동
		const bool bExpectFire = (Hit % 3 == 0);
		// 기대 AttackFactor: 발동했으면 15, 아니면 10
		const float ExpectedOnHit = bExpectFire ? 15.f : 10.f;

		DriveTiming(Passive, OnStartUsingSkill, Ctx);
		TestEqual(FString::Printf(TEXT("%d번째 타격 시작, 기대 AttackFactor %.0f"), Hit, ExpectedOnHit), Comp->GetAttributeCurrentValue(UUnitAttributeSet::GetAttackFactorAttribute()), ExpectedOnHit);

		DriveTiming(Passive, OnEndUsingSkill, Ctx);
		TestEqual(FString::Printf(TEXT("%d번째 타격 종료, 기대 AttackFactor 10"), Hit), Comp->GetAttributeCurrentValue(UUnitAttributeSet::GetAttackFactorAttribute()), 10.f);
	}

	return true;
}
