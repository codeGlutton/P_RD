/*****************************************************************//**
 * @file   TacticalPassiveGenericTests.cpp
 * @brief  UTacticalPassive_Generic 자동화 테스트
 * @details
 * DA만으로 조건/카운터/캡처/효과를 정의하는 제네릭 패시브를 유형별 대표 케이스로 검증.
 * @author 이문환
 * @date   2026-09-01
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "TAS/TASAttributeTestsHelper.h"
#include "TAS/Passive/PassiveTestsHelper.h"

#include "TAS/Passive/TacticalPassive_Generic.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/PassiveCondition.h"
#include "TAS/Passive/DynamicPassiveData_Generic.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"
#include "TAS/Effect/Stat/TacticalEffect_Defense.h"
#include "TAS/Effect/Stat/TacticalEffect_HealFactor.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackFactor.h"
#include "TAS/Effect/Stat/TacticalEffect_HP.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Simulation/RoomInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// 월드 획득
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

		// 이펙트 적용/해제에 필요한 프레임워크 모델 등록
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

	// 피연산자 조립: 고정값
	FPassiveOperand MakeConstOperand(float Value)
	{
		FPassiveOperand Operand;
		Operand.mKind = EPassiveOperandKind::Const;
		Operand.mConst = Value;
		return Operand;
	}

	// 피연산자 조립: 속성값
	FPassiveOperand MakeAttrOperand(const FTacticalAttribute& Attribute, EPassiveOperandSource Source, float Multiplier = 1.f)
	{
		FPassiveOperand Operand;
		Operand.mKind = EPassiveOperandKind::Attribute;
		Operand.mSource = Source;
		Operand.mAttribute = Attribute;
		Operand.mMultiplier = Multiplier;
		return Operand;
	}

	// 피연산자 조립: 카운터
	FPassiveOperand MakeCounterOperand()
	{
		FPassiveOperand Operand;
		Operand.mKind = EPassiveOperandKind::Counter;
		return Operand;
	}

	// 피연산자 조립: 캡처값
	FPassiveOperand MakeCapturedOperand(FName Key, EPassiveOperandSource Source)
	{
		FPassiveOperand Operand;
		Operand.mKind = EPassiveOperandKind::Captured;
		Operand.mSource = Source;
		Operand.mCaptureKey = Key;
		return Operand;
	}

	// 조건 조립
	FPassiveCondition MakeCondition(const FPassiveOperand& Lhs, EPassiveCompareOp Op, const FPassiveOperand& Rhs)
	{
		FPassiveCondition Condition;
		Condition.mLhs = Lhs;
		Condition.mOp = Op;
		Condition.mRhs = Rhs;
		return Condition;
	}

	// Generic용 DA 조립: 효과 1개 기본형. 조건/캡처/추가 효과는 호출부에서 채움
	UStaticPassiveData* MakeGenericData(FGameplayTag Activate, FGameplayTag Deactivate, TSubclassOf<UTacticalEffect> EffectClass, const FPassiveOperand& Magnitude)
	{
		UStaticPassiveData* Data = NewObject<UStaticPassiveData>();
		Data->mActivateTimingTag = Activate;
		Data->mDeactivateTimingTag = Deactivate;

		FPassiveEffectEntry Entry;
		Entry.mEffectClass = EffectClass.Get();
		Entry.mMagnitude = Magnitude;
		Data->mEffects.Add(Entry);
		return Data;
	}

	// Generic 패시브 생성
	UTacticalPassive_Generic* MakeGenericPassive(UStaticPassiveData* Data)
	{
		UTacticalPassive_Generic* Passive = NewObject<UTacticalPassive_Generic>();
		Passive->SetStaticData(Data);
		return Passive;
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
 * @brief 무조건 발동형 테스트
 * 조건 없는 DA로 Room 시작에 적용, Room 종료에 해제되는지 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveGenericBasicTest,
	"P_RD.TAS.Passive.Generic.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveGenericBasicTest::RunTest(const FString& Parameters)
{
	UMockBoardActorModel* Actor = MakeMockActor(*this);
	if (Actor == nullptr)
	{
		return false;
	}
	UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();

	// 적용 전 HealFactor 1
	Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHealFactorAttribute(), 1.f);

	const FGameplayTag OnStartRoom = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartRoom"));
	const FGameplayTag OnEndRoom = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndRoom"));

	// 조건 없이 힐 배율 1.25를 룸 동안 유지하는 DA
	UStaticPassiveData* Data = MakeGenericData(OnStartRoom, OnEndRoom, UTacticalEffect_HealFactor_MultiplyAdditive::StaticClass(), MakeConstOperand(1.25f));
	UTacticalPassive_Generic* Passive = MakeGenericPassive(Data);

	FPassiveActivateContext Ctx;
	Ctx.mOwner = Actor;
	Ctx.mTargets.Add(Actor);

	// 발동: 힐 배율 1.25 적용
	DriveTiming(Passive, OnStartRoom, Ctx);
	TestEqual(TEXT("OnStartRoom 발동: HealFactor 1.25"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHealFactorAttribute()), 1.25f);

	// 해제: 원래대로
	DriveTiming(Passive, OnEndRoom, Ctx);
	TestEqual(TEXT("OnEndRoom 해제: HealFactor 1"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHealFactorAttribute()), 1.f);

	return true;
}

/**
 * @brief 조건형 테스트
 * 체력 절반 이하 조건이 스냅샷으로 판정되는지 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveGenericConditionTest,
	"P_RD.TAS.Passive.Generic.Condition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveGenericConditionTest::RunTest(const FString& Parameters)
{
	UMockBoardActorModel* Actor = MakeMockActor(*this);
	if (Actor == nullptr)
	{
		return false;
	}
	UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();

	// 최대체력 100, 현재체력 80에서 시작
	Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetMaxHPAttribute(), 100.f);
	Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute(), 80.f);

	const FGameplayTag OnEndTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndTurn"));

	// 체력이 절반 이하면 방어도 4를 주는 DA
	UStaticPassiveData* Data = MakeGenericData(OnEndTurn, FGameplayTag(), UTacticalEffect_Defense::StaticClass(), MakeConstOperand(4.f));
	Data->mConditions.Add(MakeCondition(
		MakeAttrOperand(UCombatTargetAttributeSet::GetHPAttribute(), EPassiveOperandSource::Self),
		EPassiveCompareOp::LessEqual,
		MakeAttrOperand(UCombatTargetAttributeSet::GetMaxHPAttribute(), EPassiveOperandSource::Self, 0.5f)));
	UTacticalPassive_Generic* Passive = MakeGenericPassive(Data);

	// HP 80: 조건 거짓이라 미발동
	{
		FPassiveActivateContext Ctx;
		Ctx.mOwner = Actor;
		Ctx.mTargets.Add(Actor);
		Ctx.mOwnerSnapshot = Actor->MakeSnapshotData();
		DriveTiming(Passive, OnEndTurn, Ctx);
		TestEqual(TEXT("HP 80: 미발동, Defense 0"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetDefenseAttribute()), 0.f);
	}

	// HP 40: 조건 참이라 방어도 4
	Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute(), 40.f);
	{
		FPassiveActivateContext Ctx;
		Ctx.mOwner = Actor;
		Ctx.mTargets.Add(Actor);
		Ctx.mOwnerSnapshot = Actor->MakeSnapshotData();
		DriveTiming(Passive, OnEndTurn, Ctx);
		TestEqual(TEXT("HP 40: 발동, Defense 4"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetDefenseAttribute()), 4.f);
	}

	return true;
}

/**
 * @brief 카운터형 테스트
 * 매 3번째 발동 조건과 리셋 타이밍이 동작하는지 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveGenericCounterTest,
	"P_RD.TAS.Passive.Generic.Counter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveGenericCounterTest::RunTest(const FString& Parameters)
{
	UMockBoardActorModel* Actor = MakeMockActor(*this);
	if (Actor == nullptr)
	{
		return false;
	}
	UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();

	// 적용 전 AttackFactor 10
	Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);

	const FGameplayTag OnStartUsingSkill = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartUsingSkill"));
	const FGameplayTag OnEndUsingSkill = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndUsingSkill"));
	const FGameplayTag OnEndTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndTurn"));

	// 매 3번째 스킬 사용에 공격 보너스를 주는 DA
	auto MakeCounterData = [&]() -> UStaticPassiveData*
	{
		UStaticPassiveData* Data = MakeGenericData(OnStartUsingSkill, OnEndUsingSkill, UTacticalEffect_AttackFactor_AddBase::StaticClass(), MakeConstOperand(5.f));
		Data->mConditions.Add(MakeCondition(MakeCounterOperand(), EPassiveCompareOp::ModuloZero, MakeConstOperand(3.f)));
		return Data;
	};

	FPassiveActivateContext Ctx;
	Ctx.mOwner = Actor;
	Ctx.mTargets.Add(Actor);

	// 8회 반복: 3의 배수 회차만 발동
	{
		UTacticalPassive_Generic* Passive = MakeGenericPassive(MakeCounterData());
		for (int32 Hit = 1; Hit <= 8; ++Hit)
		{
			const bool bExpectFire = (Hit % 3 == 0);
			const float ExpectedOnHit = bExpectFire ? 15.f : 10.f;

			DriveTiming(Passive, OnStartUsingSkill, Ctx);
			TestEqual(FString::Printf(TEXT("%d번째 사용, 기대 AttackFactor %.0f"), Hit, ExpectedOnHit), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), ExpectedOnHit);

			DriveTiming(Passive, OnEndUsingSkill, Ctx);
			TestEqual(FString::Printf(TEXT("%d번째 종료, 기대 AttackFactor 10"), Hit), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 10.f);
		}
	}

	// 리셋 검증: 2회 진행 후 턴 종료로 리셋되면, 리셋 기준 3회째에 발동
	{
		UStaticPassiveData* Data = MakeCounterData();
		Data->mCounterResetTimingTag = OnEndTurn;
		UTacticalPassive_Generic* Passive = MakeGenericPassive(Data);

		// 발동 없이 2회 진행
		for (int32 Hit = 1; Hit <= 2; ++Hit)
		{
			DriveTiming(Passive, OnStartUsingSkill, Ctx);
			DriveTiming(Passive, OnEndUsingSkill, Ctx);
		}

		// 턴 종료: 카운터 리셋
		DriveTiming(Passive, OnEndTurn, Ctx);

		// 리셋 후 1, 2회째 미발동, 3회째 발동
		for (int32 Hit = 1; Hit <= 3; ++Hit)
		{
			const float ExpectedOnHit = (Hit == 3) ? 15.f : 10.f;
			DriveTiming(Passive, OnStartUsingSkill, Ctx);
			TestEqual(FString::Printf(TEXT("리셋 후 %d번째 사용, 기대 AttackFactor %.0f"), Hit, ExpectedOnHit), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), ExpectedOnHit);
			DriveTiming(Passive, OnEndUsingSkill, Ctx);
		}
	}

	return true;
}

/**
 * @brief 캡처형 테스트
 * 스킬 사용 전 체력을 캡처해 사용 후 감소 여부로 발동을 판정하는지 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveGenericCaptureTest,
	"P_RD.TAS.Passive.Generic.Capture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveGenericCaptureTest::RunTest(const FString& Parameters)
{
	const FGameplayTag OnStartUsingSkill = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartUsingSkill"));
	const FGameplayTag OnEndUsingSkill = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndUsingSkill"));

	// 체력이 캡처 시점보다 낮으면 방어도 4를 주는 케이스 하나를 실행
	// bDamaged: 캡처와 발동 사이에 체력을 깎을지
	auto RunCase = [&](bool bDamaged) -> float
	{
		UMockBoardActorModel* Actor = MakeMockActor(*this);
		if (Actor == nullptr)
		{
			return -1.f;
		}
		UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();
		Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetMaxHPAttribute(), 100.f);
		Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute(), 80.f);

		// 스킬 시작에 HP를 캡처하고, 종료에 현재 HP와 비교하는 DA
		UStaticPassiveData* Data = MakeGenericData(OnEndUsingSkill, FGameplayTag(), UTacticalEffect_Defense::StaticClass(), MakeConstOperand(4.f));
		Data->mCaptureTimingTag = OnStartUsingSkill;
		FPassiveCaptureEntry Capture;
		Capture.mKey = FName(TEXT("HP"));
		Capture.mOperand = MakeAttrOperand(UCombatTargetAttributeSet::GetHPAttribute(), EPassiveOperandSource::Self);
		Data->mCaptureOperands.Add(Capture);
		Data->mConditions.Add(MakeCondition(
			MakeAttrOperand(UCombatTargetAttributeSet::GetHPAttribute(), EPassiveOperandSource::Self),
			EPassiveCompareOp::Less,
			MakeCapturedOperand(FName(TEXT("HP")), EPassiveOperandSource::Self)));
		UTacticalPassive_Generic* Passive = MakeGenericPassive(Data);

		// 스냅샷을 새로 떠서 Ctx를 구성
		auto MakeCtx = [&]() -> FPassiveActivateContext
		{
			FPassiveActivateContext Ctx;
			Ctx.mOwner = Actor;
			Ctx.mTargets.Add(Actor);
			UBoardCombatTargetSnapshotData* Snapshot = Actor->MakeSnapshotData();
			Ctx.mOwnerSnapshot = Snapshot;
			Ctx.mTargetSnapshots.Add(Snapshot);
			return Ctx;
		};

		// 스킬 시작: HP 80 캡처
		DriveTiming(Passive, OnStartUsingSkill, MakeCtx());

		// 케이스에 따라 체력을 깎음
		if (bDamaged)
		{
			Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute(), 60.f);
		}

		// 스킬 종료: 캡처값과 비교해 발동 판정
		DriveTiming(Passive, OnEndUsingSkill, MakeCtx());
		return Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetDefenseAttribute());
	};

	// 체력이 깎였으면 발동
	TestEqual(TEXT("HP 감소: 발동, Defense 4"), RunCase(true), 4.f);
	// 체력이 그대로면 미발동
	TestEqual(TEXT("HP 유지: 미발동, Defense 0"), RunCase(false), 0.f);

	return true;
}

/**
 * @brief 수치 피연산자와 효과 배열 테스트
 * 효과 수치가 속성값으로 계산되는 것과 효과 2개 동시 적용을 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveGenericMagnitudeTest,
	"P_RD.TAS.Passive.Generic.Magnitude",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveGenericMagnitudeTest::RunTest(const FString& Parameters)
{
	const FGameplayTag OnEndTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndTurn"));

	// 속성값 수치: 공격력만큼 방어도를 얻는 DA
	{
		UMockBoardActorModel* Actor = MakeMockActor(*this);
		if (Actor == nullptr)
		{
			return false;
		}
		UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();
		Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 3.f);

		UStaticPassiveData* Data = MakeGenericData(OnEndTurn, FGameplayTag(), UTacticalEffect_Defense::StaticClass(),
			MakeAttrOperand(UCombatTargetAttributeSet::GetAttackFactorAttribute(), EPassiveOperandSource::Self));
		UTacticalPassive_Generic* Passive = MakeGenericPassive(Data);

		// 수치 계산이 스냅샷을 읽으므로 소유자 스냅샷 필요
		FPassiveActivateContext Ctx;
		Ctx.mOwner = Actor;
		Ctx.mTargets.Add(Actor);
		Ctx.mOwnerSnapshot = Actor->MakeSnapshotData();

		DriveTiming(Passive, OnEndTurn, Ctx);
		TestEqual(TEXT("수치 = AttackFactor: Defense 3"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetDefenseAttribute()), 3.f);
	}

	// 효과 2개: 방어도 +4와 체력 +5 동시 적용
	{
		UMockBoardActorModel* Actor = MakeMockActor(*this);
		if (Actor == nullptr)
		{
			return false;
		}
		UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();
		Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetMaxHPAttribute(), 100.f);
		Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute(), 50.f);

		UStaticPassiveData* Data = MakeGenericData(OnEndTurn, FGameplayTag(), UTacticalEffect_Defense::StaticClass(), MakeConstOperand(4.f));
		FPassiveEffectEntry Second;
		Second.mEffectClass = UTacticalEffect_HP::StaticClass();
		Second.mMagnitude = MakeConstOperand(5.f);
		Data->mEffects.Add(Second);
		UTacticalPassive_Generic* Passive = MakeGenericPassive(Data);

		FPassiveActivateContext Ctx;
		Ctx.mOwner = Actor;
		Ctx.mTargets.Add(Actor);

		DriveTiming(Passive, OnEndTurn, Ctx);
		TestEqual(TEXT("효과 1: Defense 4"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetDefenseAttribute()), 4.f);
		TestEqual(TEXT("효과 2: HP 55"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHPAttribute()), 55.f);
	}

	return true;
}

/**
 * @brief 효과 대상과 수량 조건 테스트
 * Self/Targets 적용 구분, 데이터 조건 기반 Any/All 게이트, Infinite 효과 2개의 갱신과 해제를 검증
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveGenericTargetTest,
	"P_RD.TAS.Passive.Generic.Target",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveGenericTargetTest::RunTest(const FString& Parameters)
{
	const FGameplayTag OnStartTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));
	const FGameplayTag OnEndTurn = PassiveTiming(TEXT("GameplayAbility.Passive.OnEndTurn"));

	// 효과 대상: Self면 소유자만, Targets면 타겟 전부
	{
		UMockBoardActorModel* Player = MakeMockActor(*this);
		UMockBoardActorModel* Target1 = MakeMockActor(*this);
		UMockBoardActorModel* Target2 = MakeMockActor(*this);
		if (Player == nullptr || Target1 == nullptr || Target2 == nullptr)
		{
			return false;
		}
		UAttributeSetComponentModel* PlayerComp = Player->GetAttributeComponentModel();
		UAttributeSetComponentModel* TargetComp1 = Target1->GetAttributeComponentModel();
		UAttributeSetComponentModel* TargetComp2 = Target2->GetAttributeComponentModel();
		PlayerComp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);
		TargetComp1->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);
		TargetComp2->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);

		FPassiveActivateContext Ctx;
		Ctx.mOwner = Player;
		Ctx.mTargets.Add(Target1);
		Ctx.mTargets.Add(Target2);

		// Self: 소유자만 +5
		{
			UStaticPassiveData* Data = MakeGenericData(OnStartTurn, OnEndTurn, UTacticalEffect_AttackFactor_AddBase::StaticClass(), MakeConstOperand(5.f));
			Data->mEffectTarget = EPassiveEffectTarget::Self;
			UTacticalPassive_Generic* Passive = MakeGenericPassive(Data);

			DriveTiming(Passive, OnStartTurn, Ctx);
			TestEqual(TEXT("Self 발동: 소유자 15"), PlayerComp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 15.f);
			TestEqual(TEXT("Self 발동: 타겟1은 10 유지"), TargetComp1->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 10.f);
			DriveTiming(Passive, OnEndTurn, Ctx);
			TestEqual(TEXT("Self 해제: 소유자 10"), PlayerComp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 10.f);
		}

		// Targets: 타겟 전부 +5, 소유자는 그대로
		{
			UStaticPassiveData* Data = MakeGenericData(OnStartTurn, OnEndTurn, UTacticalEffect_AttackFactor_AddBase::StaticClass(), MakeConstOperand(5.f));
			Data->mEffectTarget = EPassiveEffectTarget::Targets;
			UTacticalPassive_Generic* Passive = MakeGenericPassive(Data);

			DriveTiming(Passive, OnStartTurn, Ctx);
			TestEqual(TEXT("Targets 발동: 타겟1 15"), TargetComp1->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 15.f);
			TestEqual(TEXT("Targets 발동: 타겟2 15"), TargetComp2->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 15.f);
			TestEqual(TEXT("Targets 발동: 소유자는 10 유지"), PlayerComp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 10.f);
			DriveTiming(Passive, OnEndTurn, Ctx);
			TestEqual(TEXT("Targets 해제: 타겟1 10"), TargetComp1->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 10.f);
			TestEqual(TEXT("Targets 해제: 타겟2 10"), TargetComp2->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 10.f);
		}
	}

	// 수량 조건: 타겟 HP < 50 조건을 데이터로 걸고 Any/All 게이트 검증
	{
		auto RunCase = [&](EPassiveTargetQuantifier Quantifier, float HP1, float HP2) -> float
		{
			UMockBoardActorModel* Player = MakeMockActor(*this);
			UMockBoardActorModel* Target1 = MakeMockActor(*this);
			UMockBoardActorModel* Target2 = MakeMockActor(*this);
			if (Player == nullptr || Target1 == nullptr || Target2 == nullptr)
			{
				return -1.f;
			}
			UAttributeSetComponentModel* TargetComp1 = Target1->GetAttributeComponentModel();
			UAttributeSetComponentModel* TargetComp2 = Target2->GetAttributeComponentModel();
			TargetComp1->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute(), HP1);
			TargetComp2->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute(), HP2);
			TargetComp1->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);
			TargetComp2->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);

			UStaticPassiveData* Data = MakeGenericData(OnStartTurn, FGameplayTag(), UTacticalEffect_AttackFactor_AddBase::StaticClass(), MakeConstOperand(5.f));
			Data->mEffectTarget = EPassiveEffectTarget::Targets;
			Data->mTargetQuantifier = Quantifier;
			Data->mConditions.Add(MakeCondition(
				MakeAttrOperand(UCombatTargetAttributeSet::GetHPAttribute(), EPassiveOperandSource::Target),
				EPassiveCompareOp::Less,
				MakeConstOperand(50.f)));
			UTacticalPassive_Generic* Passive = MakeGenericPassive(Data);

			FPassiveActivateContext Ctx;
			Ctx.mOwner = Player;
			Ctx.mTargets.Add(Target1);
			Ctx.mTargets.Add(Target2);
			Ctx.mTargetSnapshots.Add(Target1->MakeSnapshotData());
			Ctx.mTargetSnapshots.Add(Target2->MakeSnapshotData());

			DriveTiming(Passive, OnStartTurn, Ctx);
			return TargetComp1->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute());
		};

		TestEqual(TEXT("Any/자격1 → 발동"), RunCase(EPassiveTargetQuantifier::Any, 40.f, 80.f), 15.f);
		TestEqual(TEXT("Any/자격0 → 미발동"), RunCase(EPassiveTargetQuantifier::Any, 80.f, 90.f), 10.f);
		TestEqual(TEXT("All/자격2 → 발동"), RunCase(EPassiveTargetQuantifier::All, 40.f, 30.f), 15.f);
		TestEqual(TEXT("All/자격1 → 미발동"), RunCase(EPassiveTargetQuantifier::All, 40.f, 80.f), 10.f);
	}

	// Infinite 효과 2개: 동시 적용, 재발동 갱신, 해제까지
	{
		UMockBoardActorModel* Actor = MakeMockActor(*this);
		if (Actor == nullptr)
		{
			return false;
		}
		UAttributeSetComponentModel* Comp = Actor->GetAttributeComponentModel();
		Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetAttackFactorAttribute(), 10.f);
		Comp->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHealFactorAttribute(), 1.f);

		UStaticPassiveData* Data = MakeGenericData(OnStartTurn, OnEndTurn, UTacticalEffect_AttackFactor_AddBase::StaticClass(), MakeConstOperand(5.f));
		FPassiveEffectEntry Second;
		Second.mEffectClass = UTacticalEffect_HealFactor_MultiplyAdditive::StaticClass();
		Second.mMagnitude = MakeConstOperand(1.25f);
		Data->mEffects.Add(Second);
		UTacticalPassive_Generic* Passive = MakeGenericPassive(Data);

		FPassiveActivateContext Ctx;
		Ctx.mOwner = Actor;
		Ctx.mTargets.Add(Actor);

		// 동시 적용: 핸들이 서로를 지우지 않아야 함
		DriveTiming(Passive, OnStartTurn, Ctx);
		TestEqual(TEXT("Infinite 2개 발동: AttackFactor 15"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 15.f);
		TestEqual(TEXT("Infinite 2개 발동: HealFactor 1.25"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHealFactorAttribute()), 1.25f);

		// 재발동: 갱신이므로 값이 누적되지 않아야 함
		DriveTiming(Passive, OnStartTurn, Ctx);
		TestEqual(TEXT("재발동: AttackFactor 15 유지"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 15.f);
		TestEqual(TEXT("재발동: HealFactor 1.25 유지"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHealFactorAttribute()), 1.25f);

		// 해제: 둘 다 원복
		DriveTiming(Passive, OnEndTurn, Ctx);
		TestEqual(TEXT("해제: AttackFactor 10"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackFactorAttribute()), 10.f);
		TestEqual(TEXT("해제: HealFactor 1"), Comp->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHealFactorAttribute()), 1.f);
	}

	return true;
}
