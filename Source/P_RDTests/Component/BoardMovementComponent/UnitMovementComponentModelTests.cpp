/*****************************************************************//**
 * @file   UnitMovementComponentModelTests.cpp
 * @brief  유닛 이동 컴포넌트 모델의 속박(이동불가) 판정 유닛테스트
 * @details
 * 속박 태그 부여/턴 경과에 따른 IsMoveable 상태 전이를 검증
 * @author 이문환
 * @date   2026-08-10
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h"

#include "Component/BoardMovementComponent/UnitMovementComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "GameplayTagType.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// 월드
	UWorld* GetAnyGameWorldForRootTests()
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnitRootTests,
	"P_RD.SRPG.UnitRoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FUnitRootTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForRootTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	// 유닛 생성 (스폰 데이터 없이 도는 플래너용 Mock 재사용)
	UMockEnemyUnitModel* Unit = NewObject<UMockEnemyUnitModel>(World);
	Unit->Initialize();
	Unit->BeginPlay();

	UUnitMovementComponentModel* MovementCompModel = Cast<UUnitMovementComponentModel>(Unit->GetBoardMovementComponentModel());
	if (TestNotNull(TEXT("유닛 이동 컴포넌트 모델"), MovementCompModel) == false)
	{
		return false;
	}
	UAttributeSetComponentModel* AttrComp = Unit->GetAttributeComponentModel();
	if (TestNotNull(TEXT("속성 컴포넌트 모델"), AttrComp) == false)
	{
		return false;
	}

	/* [1] 기본 상태: 이동 가능 */
	TestTrue(TEXT("기본: 이동 가능"), MovementCompModel->IsMoveable());

	/* [2] 속박 2스택 부여: 이동 불가 */
	AttrComp->AddLooseGameplayTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Root, 2);
	TestFalse(TEXT("속박 2스택: 이동 불가"), MovementCompModel->IsMoveable());

	/* [3] 턴 종료 1회: 1스택 남아 여전히 이동 불가 (기존 턴제 상태이상 감소 메커니즘 재사용) */
	Unit->OnEndTurn(0);
	TestFalse(TEXT("턴 종료 1회(1스택): 이동 불가"), MovementCompModel->IsMoveable());

	/* [4] 턴 종료 2회: 스택 소진으로 이동 가능 */
	Unit->OnEndTurn(1);
	TestTrue(TEXT("턴 종료 2회(0스택): 이동 가능"), MovementCompModel->IsMoveable());

	return true;
}
