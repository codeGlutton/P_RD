/*****************************************************************//**
 * @file   TacticalEffectDurationTests.cpp
 * @brief  Duration Effect 및 지속시간 만기(CheckDurationExpired) 자동화 테스트
 * @author 모호재
 * @date   2026-07-23
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "TASAttributeTestsHelper.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/AttributeSetMinimal.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Simulation/RoomInstance.h"

/**
 * @brief 활성화된 월드(PIE, Game)를 탐색하는 헬퍼 함수
 * @return 현재 월드
 */
static UWorld* GetAnyGameWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTacticalEffectDurationTests,
	"P_RD.TAS.EffectDuration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTacticalEffectDurationTests::RunTest(const FString& Parameters)
{
	/* 1. 월드 및 FrameworkModel 환경 준비 */
	UWorld* World = GetAnyGameWorld();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("UWorld 환경이 유효해야 합니다."), World) == false)
	{
		return false;
	}

	USimulationSubsystem* SimSubsystem = World->GetSubsystem<USimulationSubsystem>();
	if (TestNotNull(TEXT("USimulationSubsystem이 존재해야 합니다."), SimSubsystem) == false)
	{
		return false;
	}

	URoomInstance* RoomInstance = FSimulationSubsystemTestAccessor::GetRoomInstance(SimSubsystem);
	if (TestNotNull(TEXT("RoomInstance가 존재해야 합니다."), RoomInstance) == false)
	{
		return false;
	}

	UTacticalFrameworkModel* FrameworkModel = Cast<UTacticalFrameworkModel>(RoomInstance->mAliveSubsystemModels.FindRef(UTacticalFrameworkModel::StaticClass()));
	if (FrameworkModel == nullptr)
	{
		FrameworkModel = NewObject<UTacticalFrameworkModel>(RoomInstance);
		RoomInstance->mAliveSubsystemModels.Add(UTacticalFrameworkModel::StaticClass(), FrameworkModel);
	}

	/* 2. Mock ActorModel 및 AttributeSetComponentModel 생성 */
	UTASActorModelMock* MockActorModel = NewObject<UTASActorModelMock>(World);
	MockActorModel->Initialize();
	MockActorModel->BeginPlay();

	UAttributeSetComponentModel* CompModel = MockActorModel->GetAttributeComponent();
	if (TestNotNull(TEXT("CompModel이 유효해야 합니다."), CompModel) == false)
	{
		return false;
	}

	UTacticalEffectContext* EffectContext = CompModel->MakeEffectContext();

	/* 테스트 1: 턴/라운드 지속시간 단위 격리 검증 (EveryTurn vs EveryRound) */
	{
		FrameworkModel->AdvanceTurnDuration(0);
		FrameworkModel->AdvanceRoundDuration(0);

		// 2턴 지속 이펙트 적용
		

		TSharedPtr<FTacticalEffectSpec> TurnEffectSpec = CompModel->MakeOutgoingSpec(UTestTurnDurationTacticalEffect::StaticClass(), EffectContext);
		FActiveTacticalEffectHandle TurnHandle = CompModel->ApplyTacticalEffectSpecToSelf(*TurnEffectSpec);
		TestTrue(TEXT("2턴 지속 이펙트가 적용되어야 합니다."), TurnHandle.IsValid());

		// 라운드를 진행했을 때 (AdvanceRoundDuration) 턴 지속 이펙트가 삭제되지 않고 유지되는지 검증
		FrameworkModel->AdvanceRoundDuration(1);
		FrameworkModel->AdvanceRoundDuration(2);
		TestTrue(TEXT("라운드 진행 시에는 턴 단위 이펙트가 만기되지 않아야 합니다."), CompModel->GetActiveTacticalEffect(TurnHandle) != nullptr);

		// 1턴 진행 시 유지 검증 (Duration = 2턴)
		FrameworkModel->AdvanceTurnDuration(1);
		TestTrue(TEXT("1턴 진행 시(Duration=2) 이펙트가 살아있어야 합니다."), CompModel->GetActiveTacticalEffect(TurnHandle) != nullptr);

		// 2턴 도달 시 만기 및 제거 검증
		FrameworkModel->AdvanceTurnDuration(2);
		TestTrue(TEXT("2턴 진행 시 이펙트가 만기되어 제거되어야 합니다."), CompModel->GetActiveTacticalEffect(TurnHandle) == nullptr);
	}

	/* 테스트 2: 스택 단일 차감 및 Duration 재시작 검증 (RemoveSingleStackAndRefreshDuration) */
	{
		FrameworkModel->AdvanceTurnDuration(0);

		// 2턴 지속, RemoveSingleStackAndRefreshDuration 이펙트 적용 (2스택)
		TSharedPtr<FTacticalEffectSpec> StackEffectSpec = CompModel->MakeOutgoingSpec(UTestStackRemoveSingleDurationTacticalEffect::StaticClass(), EffectContext);
		FActiveTacticalEffectHandle StackHandle1 = CompModel->ApplyTacticalEffectSpecToSelf(*StackEffectSpec);
		FActiveTacticalEffectHandle StackHandle2 = CompModel->ApplyTacticalEffectSpecToSelf(*StackEffectSpec);

		const FActiveTacticalEffect* ActiveEffect = CompModel->GetActiveTacticalEffect(StackHandle1);
		if (TestNotNull(TEXT("스택형 활성 이펙트가 존재해야 합니다."), ActiveEffect) == true)
		{
			TestEqual(TEXT("초기 스택 수가 2이어야 합니다."), ActiveEffect->mSpec.GetStackCount(), 2);
		}

		// 2턴 경과 -> 1스택 차감 후 1스택으로 남고, Duration이 새 턴(턴 2)으로 재시작되는지 검증
		FrameworkModel->AdvanceTurnDuration(2);

		ActiveEffect = CompModel->GetActiveTacticalEffect(StackHandle1);
		if (TestNotNull(TEXT("1스택 차감 후에도 이펙트가 유지되어야 합니다."), ActiveEffect) == true)
		{
			TestEqual(TEXT("스택 수가 1로 줄어야 합니다."), ActiveEffect->mSpec.GetStackCount(), 1);
			TestEqual(TEXT("StartTime이 턴 2로 갱신되어야 합니다."), ActiveEffect->mStartTime, 2);
		}

		// 다시 2턴 경과(총 턴 4) -> 남은 1스택마저 소멸하여 최종 제거되는지 검증
		FrameworkModel->AdvanceTurnDuration(4);
		TestTrue(TEXT("마지막 스택 차감 후 이펙트가 완전히 제거되어야 합니다."), CompModel->GetActiveTacticalEffect(StackHandle1) == nullptr);
	}

	return true;
}
