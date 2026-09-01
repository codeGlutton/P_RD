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
	FPlayerExpExactThresholdTest,
	"P_RD.Player.Progression.ExactThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerExpExactThresholdTest::RunTest(const FString& Parameters)
{
	const TArray<float> Thresholds = { 100.f, 150.f, 200.f };
	const FPlayerExpProgression Result = UPlayerUnitModel::CalculateExperienceProgression(
		1, 75.f, 25.f, Thresholds);

	TestEqual(TEXT("정확한 임계치에서 한 레벨 증가"), Result.mLevelAfter, 2);
	TestEqual(TEXT("다음 레벨 경험치는 0"), Result.mExpAfter, 0.f);
	TestEqual(TEXT("다음 레벨 임계치 갱신"), Result.mMaxExpAfter, 150.f);
	TestEqual(TEXT("레벨업 구간과 새 레벨의 종착 구간 생성"), Result.mSteps.Num(), 2);
	if (Result.mSteps.Num() == 2)
	{
		TestTrue(TEXT("구간이 레벨업으로 표시"), Result.mSteps[0].mDidLevelUp);
		TestEqual(TEXT("막대는 기존 레벨 최대치까지 채움"), Result.mSteps[0].mExpAfter, 100.f);
		TestFalse(TEXT("새 레벨 종착 구간은 레벨업 아님"), Result.mSteps[1].mDidLevelUp);
		TestEqual(TEXT("새 레벨 종착 경험치는 0"), Result.mSteps[1].mExpAfter, 0.f);
		TestEqual(TEXT("새 레벨 종착 임계치"), Result.mSteps[1].mMaxExp, 150.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerExpMultiLevelTest,
	"P_RD.Player.Progression.MultiLevelCarry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerExpMultiLevelTest::RunTest(const FString& Parameters)
{
	const TArray<float> Thresholds = { 100.f, 150.f, 200.f };
	const FPlayerExpProgression Result = UPlayerUnitModel::CalculateExperienceProgression(
		1, 40.f, 250.f, Thresholds);

	TestEqual(TEXT("두 번 레벨업 후 상한 레벨"), Result.mLevelAfter, 3);
	TestEqual(TEXT("모든 임계치 차감 뒤 잔여 경험치 보존"), Result.mExpAfter, 40.f);
	TestEqual(TEXT("최종 레벨 임계치"), Result.mMaxExpAfter, 200.f);
	TestTrue(TEXT("상한 레벨 도달 표시"), Result.mReachedLevelCap);
	TestEqual(TEXT("두 레벨업과 마지막 잔여 구간"), Result.mSteps.Num(), 3);

	if (Result.mSteps.Num() == 3)
	{
		TestTrue(TEXT("첫 구간 레벨업"), Result.mSteps[0].mDidLevelUp);
		TestEqual(TEXT("첫 구간 이월 경험치"), Result.mSteps[0].mCarryExp, 190.f);
		TestTrue(TEXT("두 번째 구간 레벨업"), Result.mSteps[1].mDidLevelUp);
		TestEqual(TEXT("두 번째 구간 이월 경험치"), Result.mSteps[1].mCarryExp, 40.f);
		TestFalse(TEXT("마지막 구간은 레벨업 아님"), Result.mSteps[2].mDidLevelUp);
		TestEqual(TEXT("마지막 구간 잔여 경험치"), Result.mSteps[2].mExpAfter, 40.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerExpLevelCapTest,
	"P_RD.Player.Progression.LevelCap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerExpLevelCapTest::RunTest(const FString& Parameters)
{
	const TArray<float> Thresholds = { 100.f, 150.f, 200.f };
	const FPlayerExpProgression Result = UPlayerUnitModel::CalculateExperienceProgression(
		3, 190.f, 50.f, Thresholds);

	TestEqual(TEXT("상한에서 레벨이 증가하지 않음"), Result.mLevelAfter, 3);
	TestEqual(TEXT("상한 경험치가 막대 최대치로 고정"), Result.mExpAfter, 200.f);
	TestTrue(TEXT("상한 표시"), Result.mReachedLevelCap);
	TestEqual(TEXT("상한 구간 하나"), Result.mSteps.Num(), 1);
	if (Result.mSteps.Num() == 1)
	{
		TestFalse(TEXT("상한 구간은 레벨업 아님"), Result.mSteps[0].mDidLevelUp);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerExpInvalidThresholdTest,
	"P_RD.Player.Progression.InvalidThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerExpInvalidThresholdTest::RunTest(const FString& Parameters)
{
	const TArray<float> Thresholds = { 100.f, 0.f, 200.f };
	const FPlayerExpProgression Result = UPlayerUnitModel::CalculateExperienceProgression(
		1, 90.f, 50.f, Thresholds);

	TestEqual(TEXT("0 임계치 뒤 레벨을 사용하지 않음"), Result.mLevelAfter, 1);
	TestEqual(TEXT("유효 구간의 최대치에서 정지"), Result.mExpAfter, 100.f);
	TestEqual(TEXT("유효한 마지막 임계치"), Result.mMaxExpAfter, 100.f);
	TestEqual(TEXT("재귀 없이 한 구간으로 종료"), Result.mSteps.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerExpMissingCurveTest,
	"P_RD.Player.Progression.MissingCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerExpMissingCurveTest::RunTest(const FString& Parameters)
{
	const FPlayerExpProgression Result = UPlayerUnitModel::CalculateExperienceProgression(
		2, 30.f, 70.f, TArray<float>());

	TestEqual(TEXT("커브가 없으면 임의 레벨업을 하지 않음"), Result.mLevelAfter, 2);
	TestEqual(TEXT("커브가 없으면 경험치를 손실하지 않음"), Result.mExpAfter, 100.f);
	TestEqual(TEXT("잘못된 MaxExp를 사용하지 않음"), Result.mMaxExpAfter, 0.f);
	TestFalse(TEXT("구성되지 않은 상한으로 취급하지 않음"), Result.mReachedLevelCap);
	return true;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerLevelDeferredCreationTest,
	"P_RD.Player.Progression.DeferredCreationLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerLevelDeferredCreationTest::RunTest(const FString& Parameters)
{
	UWorld* World = GetProgressionGameWorld();
	if (TestNotNull(TEXT("진행 시스템 테스트 월드"), World) == false)
	{
		return false;
	}

	USimulationSubsystem* SimulationSubsystem =
		World->GetSubsystem<USimulationSubsystem>();
	UStaticPlayerUnitSpawnData* SpawnData = LoadObject<UStaticPlayerUnitSpawnData>(
		nullptr,
		TEXT("/Game/BP/DataAsset/Unit/PlayerUnit/DA_TestKnightPlayerUnit."
			"DA_TestKnightPlayerUnit"));
	UClass* ModelClass = SpawnData != nullptr
		? SpawnData->mModelClass.LoadSynchronous() : nullptr;
	if (TestNotNull(TEXT("시뮬레이션 서브시스템"), SimulationSubsystem) == false
		|| TestNotNull(TEXT("기사 스폰 데이터"), SpawnData) == false
		|| TestNotNull(TEXT("기사 모델 클래스"), ModelClass) == false)
	{
		return false;
	}
	UObjectModelFactory& Factory = SimulationSubsystem->GetModelFactory();

	// ShopGameMode의 고용 순서와 동일하다:
	// NewModelDeferred -> SetStaticSpawnData -> SetPlayerLevel -> FinishCreating.
	UPlayerUnitModel* Model =
		Factory.NewModelDeferred<UPlayerUnitModel>(ModelClass);
	if (TestNotNull(TEXT("deferred 플레이어 모델"), Model) == false)
	{
		return false;
	}
	Model->SetStaticSpawnData(SpawnData);
	const TArray<float> Thresholds = Model->GetExperienceThresholds();
	if (TestTrue(TEXT("기사 MaxExp 커브가 두 레벨 이상 존재"),
		Thresholds.Num() >= 2) == false)
	{
		Factory.DestroyModel(Model);
		return false;
	}

	UAttributeSetComponentModel* Attributes = Model->GetAttributeComponentModel();
	if (TestNotNull(TEXT("deferred 속성 컴포넌트"), Attributes) == false)
	{
		Factory.DestroyModel(Model);
		return false;
	}
	TestFalse(TEXT("FinishCreating 전에는 MaxExp AttributeSet 미등록"),
		Attributes->HasAttributeSetForAttribute(
			UPlayerUnitAttributeSet::GetMaxExpAttribute()));

	// 회귀 지점: 기존 코드는 여기서 미등록 AttributeSet에 modifier를 적용해
	// checkf로 중단됐다. 호출이 안전하게 끝나고 레벨 값은 보존돼야 한다.
	Model->SetPlayerLevel(2);
	TestEqual(TEXT("초기화 전 레벨 보존"), Model->GetPlayerLevel(), 2);
	Model->FinishCreating();

	TestTrue(TEXT("FinishCreating 후 MaxExp AttributeSet 등록"),
		Attributes->HasAttributeSetForAttribute(
			UPlayerUnitAttributeSet::GetMaxExpAttribute()));
	Model->SetPlayerLevel(2);
	TestEqual(TEXT("초기화 후 레벨별 MaxExp 반영"),
		Attributes->GetAttributeCurrentValue(
			UPlayerUnitAttributeSet::GetMaxExpAttribute()), Thresholds[1]);

	Factory.DestroyModel(Model);
	return true;
}
