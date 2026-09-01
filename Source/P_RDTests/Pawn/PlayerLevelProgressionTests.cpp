#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "Pawn/Player/PlayerUnitModel.h"
#include "Pawn/PlayerLevelProgressionTestsHelper.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "Simulation/Factory/ObjectModelFactory.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	UWorld* GetProgressionGameWorld()
	{
		if (GEngine != nullptr)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if ((Context.WorldType == EWorldType::PIE
					|| Context.WorldType == EWorldType::Game)
					&& Context.World() != nullptr)
				{
					return Context.World();
				}
			}
		}
		return GWorld;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerLevelPersistenceTrackingTest,
	"P_RD.Player.Progression.PersistenceTracking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerLevelPersistenceTrackingTest::RunTest(const FString& Parameters)
{
	UPlayerLevelProgressionTestModel* Model = NewObject<UPlayerLevelProgressionTestModel>();
	UPlayerUnitPersistData* PersistData = NewObject<UPlayerUnitPersistData>();
	if (TestNotNull(TEXT("테스트 플레이어 모델"), Model) == false
		|| TestNotNull(TEXT("테스트 영속 데이터"), PersistData) == false)
	{
		return false;
	}

	Model->Initialize();
	PersistData->RegisterPlayerUnit(Model);

	Model->SetPlayerLevel(2);
	Model->GetAttributeComponentModel()->ApplyModToAttribute(
		UPlayerUnitAttributeSet::GetExpAttribute(), ETacticalModOp::Override, 42.f);

	TestEqual(TEXT("레벨 변경을 영속 데이터가 추적"), PersistData->GetPlayerLevel(), 2);
	TestEqual(TEXT("경험치 변경을 영속 데이터가 추적"), PersistData->GetExperience(), 42.f);

	PersistData->UnregisterPlayerUnit(Model);
	Model->Uninitialize();
	return true;
}

