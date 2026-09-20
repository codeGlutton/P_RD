/*****************************************************************//**
 * @file   StatusEffectRoundDurationTests.cpp
 * @brief  라운드 지속 상태이상이 라운드 진행에만 반응하는지 검증
 * @author 이문환
 * @date   2026-08-24
 *
 * @details
 *  [확정 규칙]
 *   - 한 명이라도 턴을 받는 라운드면 유효한 라운드이므로 기절 스택이 1 감소함.
 *     기절한 당사자가 그 라운드에 턴을 받았는지는 무관함.
 *   - 아무도 턴을 받지 못하는 사이클은 유저 입장에서 없는 라운드이므로
 *     라운드로 세지 않고, 기절 스택도 줄지 않음.
 *
 *  후자는 USRPGCombatModel::EvaluateRound가 턴 후보가 생길 때까지 스피드 충전만
 *  반복하고 BeginRound를 호출하지 않는 것으로 구현됨. 따라서 그 사이클 동안에는
 *  AdvanceRoundDuration이 호출되지 않아 만기 처리 자체가 일어나지 않음.
 *
 *  본 테스트는 TAS 계층(UTacticalFrameworkModel) 기준으로 위 규칙을 고정함.
 *  라운드 생략 판정 자체는 USRPGCombatModel 몫이라 여기서 다루지 않음.
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "TASAttributeTestsHelper.h"
#include "GameplayTagType.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Effect/Tag/TacticalEffect_Stun.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Simulation/RoomInstance.h"

namespace
{
	/**
	 * @brief 활성화된 월드(PIE, Game)를 탐색
	 * @return 현재 월드, 없으면 nullptr
	 */
	UWorld* FindAnyGameWorld()
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

	/**
	 * @brief 기절 스택을 지정 수만큼 부여
	 * @details 실제 스킬 경로(UTacticalEffect_AddStatus::OnExecuted)와 동일하게 지속형
	 *          기절 이펙트를 요청 스택 수로 적용함. 이벤트 로거 의존을 피하려고 부여
	 *          이펙트(GetStun)를 거치지 않고 지속 이펙트를 직접 적용함.
	 */
	FActiveTacticalEffectHandle ApplyStunStacks(UAttributeSetComponentModel* CompModel, UTacticalEffectContext* Context, int32 StackCount)
	{
		TSharedPtr<FTacticalEffectSpec> StunSpec = CompModel->MakeOutgoingSpec(UTacticalEffect_Stun::StaticClass(), Context);
		StunSpec->SetStackCount(StackCount);
		return CompModel->ApplyTacticalEffectSpecToSelf(*StunSpec);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStatusEffectRoundDurationTests,
	"P_RD.TAS.StatusEffectRoundDuration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FStatusEffectRoundDurationTests::RunTest(const FString& Parameters)
{
	/* 1. 월드 및 FrameworkModel 환경 준비 */

	UWorld* World = FindAnyGameWorld();
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

	const FGameplayTag StunTag = EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun;

	/* 테스트 1: 다른 유닛만 턴을 받은 라운드에서도 기절 스택이 라운드당 1씩 감소 */
	{
		UTASActorModelMock* MockActorModel = NewObject<UTASActorModelMock>(World);
		MockActorModel->Initialize();
		MockActorModel->BeginPlay();

		UAttributeSetComponentModel* CompModel = MockActorModel->GetAttributeComponent();
		if (TestNotNull(TEXT("CompModel이 유효해야 합니다."), CompModel) == false)
		{
			return false;
		}
		UTacticalEffectContext* EffectContext = CompModel->MakeEffectContext();

		int32 TurnCount = 0;
		FrameworkModel->AdvanceTurnDuration(TurnCount);
		FrameworkModel->AdvanceRoundDuration(0);

		// 기절 3스택 부여 (스킬의 TagGain 3에 해당)
		const FActiveTacticalEffectHandle StunHandle = ApplyStunStacks(CompModel, EffectContext, 3);
		if (TestTrue(TEXT("기절 이펙트가 적용되어야 합니다."), StunHandle.IsValid()) == false)
		{
			return false;
		}
		TestEqual(TEXT("부여 직후 스택 수는 3이어야 합니다."), CompModel->GetCurrentStackCount(StunHandle), 3);
		TestTrue(TEXT("부여 직후 기절 태그를 보유해야 합니다."), CompModel->HasMatchingGameplayTag(StunTag));

		// 라운드마다 소모되는 턴 수를 다르게 둠. 기절한 유닛은 느려서 턴을 못 받고,
		// 아래 턴들은 전부 다른 유닛이 소모한 것으로 간주함.
		// 감소량이 턴 수가 아니라 라운드 진행에만 좌우되는지 확인하기 위한 구성임.
		const int32 OtherUnitTurnCounts[] = { 3, 5, 2 };
		const int32 ExpectedStackCounts[] = { 2, 1, 0 };

		for (int32 RoundIndex = 0; RoundIndex < UE_ARRAY_COUNT(OtherUnitTurnCounts); ++RoundIndex)
		{
			for (int32 TurnIndex = 0; TurnIndex < OtherUnitTurnCounts[RoundIndex]; ++TurnIndex)
			{
				FrameworkModel->AdvanceTurnDuration(++TurnCount);
			}

			// 한 명이라도 턴을 받은 라운드이므로 라운드가 진행됨
			FrameworkModel->AdvanceRoundDuration(RoundIndex + 1);

			const int32 ExpectedStackCount = ExpectedStackCounts[RoundIndex];
			if (ExpectedStackCount > 0)
			{
				TestEqual(
					*FString::Printf(TEXT("라운드 %d 진행 후 스택 수가 %d이어야 합니다."), RoundIndex + 1, ExpectedStackCount),
					CompModel->GetCurrentStackCount(StunHandle), ExpectedStackCount);
				TestTrue(
					*FString::Printf(TEXT("라운드 %d 진행 후에도 기절 태그가 유지되어야 합니다."), RoundIndex + 1),
					CompModel->HasMatchingGameplayTag(StunTag));
			}
			else
			{
				TestTrue(
					*FString::Printf(TEXT("라운드 %d 진행 후 이펙트가 제거되어야 합니다."), RoundIndex + 1),
					CompModel->GetActiveTacticalEffect(StunHandle) == nullptr);
				TestFalse(
					TEXT("이펙트 제거 후 기절 태그가 해제되어야 합니다."),
					CompModel->HasMatchingGameplayTag(StunTag));
			}
		}
	}

	/* 테스트 2: 라운드가 진행되지 않으면 기절 스택이 줄지 않음 */
	{
		UTASActorModelMock* MockActorModel = NewObject<UTASActorModelMock>(World);
		MockActorModel->Initialize();
		MockActorModel->BeginPlay();

		UAttributeSetComponentModel* CompModel = MockActorModel->GetAttributeComponent();
		if (TestNotNull(TEXT("CompModel이 유효해야 합니다."), CompModel) == false)
		{
			return false;
		}
		UTacticalEffectContext* EffectContext = CompModel->MakeEffectContext();

		int32 TurnCount = 0;
		FrameworkModel->AdvanceTurnDuration(TurnCount);
		FrameworkModel->AdvanceRoundDuration(0);

		const FActiveTacticalEffectHandle StunHandle = ApplyStunStacks(CompModel, EffectContext, 2);
		if (TestTrue(TEXT("기절 이펙트가 적용되어야 합니다."), StunHandle.IsValid()) == false)
		{
			return false;
		}

		// 아무도 턴을 받지 못해 생략된 사이클은 AdvanceRoundDuration을 호출하지 않음.
		// 턴만 흘러가는 경우까지 포함해, 라운드가 진행되지 않는 한 감소가 없어야 함.
		for (int32 TurnIndex = 0; TurnIndex < 5; ++TurnIndex)
		{
			FrameworkModel->AdvanceTurnDuration(++TurnCount);
		}

		TestEqual(TEXT("라운드가 진행되지 않으면 스택 수가 그대로여야 합니다."), CompModel->GetCurrentStackCount(StunHandle), 2);
		TestTrue(TEXT("라운드가 진행되지 않으면 기절 태그가 유지되어야 합니다."), CompModel->HasMatchingGameplayTag(StunTag));

		// 유효한 라운드가 오면 그때 1 감소
		FrameworkModel->AdvanceRoundDuration(1);
		TestEqual(TEXT("유효한 라운드가 진행된 시점에만 스택이 1로 줄어야 합니다."), CompModel->GetCurrentStackCount(StunHandle), 1);
	}

	return true;
}
