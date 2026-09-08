#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "Pawn/Enemy/EnemyUnitModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FEnemyBlueprintAttributeInitializationTest,
	"P_RD.Enemy.LegacyBlueprintAttributes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FEnemyBlueprintAttributeInitializationTest::GetTests(TArray<FString>& BeautifiedNames, TArray<FString>& TestCommands) const
{
	for (const TCHAR* Enemy : { TEXT("Werewolf"), TEXT("Spider"), TEXT("Slime"), TEXT("Mushroom"),
		TEXT("Leshy"), TEXT("Golem"), TEXT("Eagle") })
	{
		BeautifiedNames.Add(Enemy);
		TestCommands.Add(FString::Printf(TEXT("/Game/BP/Pawn/Enemy/Stage1/BP_%sUnitModel.BP_%sUnitModel_C"), Enemy, Enemy));
	}
}

bool FEnemyBlueprintAttributeInitializationTest::RunTest(const FString& Parameters)
{
	UClass* EnemyClass = LoadClass<UEnemyUnitModel>(nullptr, *Parameters);
	if (!TestNotNull(TEXT("Cook-warning enemy Blueprint loads"), EnemyClass)) return false;
	TStrongObjectPtr<UEnemyUnitModel> Enemy(NewObject<UEnemyUnitModel>(GetTransientPackage(), EnemyClass));
	if (!TestNotNull(TEXT("Enemy instance created"), Enemy.Get())) return false;

	// Follow the same initialization order as FinishCreatingModel, without creating a view or touching assets.
	Enemy->Initialize();
	UAttributeSetComponentModel* Attributes = Enemy->GetAttributeComponentModel();
	if (!TestNotNull(TEXT("Attribute component initialized"), Attributes))
	{
		Enemy->Uninitialize();
		return false;
	}

	int32 UnitSetCount = 0;
	bool bOwnsAllSets = true;
	for (const UTacticalAttributeSet* Set : Attributes->GetSpawnedAttributes())
	{
		if (!TestNotNull(TEXT("Registered attribute set is valid"), Set)) continue;
		AddInfo(FString::Printf(TEXT("Registered set: %s, class=%s, outer=%s"),
			*Set->GetName(), *Set->GetClass()->GetName(), *GetNameSafe(Set->GetOuter())));
		bOwnsAllSets &= TestTrue(TEXT("Registered set belongs to the instance"), Set->GetOuter() == Enemy.Get());
		bOwnsAllSets &= TestFalse(TEXT("Registered set is not a CDO/archetype"), Set->IsTemplate());
		if (Set->IsA<UUnitAttributeSet>()) ++UnitSetCount;
	}
	TestEqual(TEXT("Exactly one Unit attribute family is registered"), UnitSetCount, 1);
	const FObjectProperty* UnitProperty = FindFProperty<FObjectProperty>(UEnemyUnitModel::StaticClass(), TEXT("mUnitAttributeSet"));
	if (UnitProperty)
	{
		AddInfo(FString::Printf(TEXT("Native member reference: %s"),
			*GetNameSafe(UnitProperty->GetObjectPropertyValue_InContainer(Enemy.Get()))));
	}

	if (bOwnsAllSets && UnitSetCount > 0)
	{
		constexpr float ExpectedHP = 137.25f;
		constexpr float ExpectedSpeed = 17.5f;
		Attributes->ApplyModToAttribute(UUnitAttributeSet::GetMaxHPAttribute(), ETacticalModOp::Override, 300.f);
		Attributes->ApplyModToAttribute(UUnitAttributeSet::GetHPAttribute(), ETacticalModOp::Override, ExpectedHP);
		Attributes->ApplyModToAttribute(UUnitAttributeSet::GetRechargeSpeedPointAttribute(), ETacticalModOp::Override, ExpectedSpeed);
		TestEqual(TEXT("Live HP can be changed after initialization"), Attributes->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()), ExpectedHP);
		TStrongObjectPtr<UBoardCombatTargetSnapshotData> Snapshot(Enemy->MakeSnapshotData());
		if (TestNotNull(TEXT("Combat snapshot created"), Snapshot.Get()))
		{
			const float* HP = Snapshot->mAttributes.Find(UUnitAttributeSet::GetHPAttribute());
			const float* Speed = Snapshot->mAttributes.Find(UUnitAttributeSet::GetRechargeSpeedPointAttribute());
			if (TestNotNull(TEXT("Snapshot contains HP"), HP)) TestEqual(TEXT("Snapshot HP matches live HP"), *HP, ExpectedHP);
			if (TestNotNull(TEXT("Snapshot contains recharge speed"), Speed)) TestEqual(TEXT("Snapshot speed matches live speed"), *Speed, ExpectedSpeed);
		}
	}
	Enemy->Uninitialize();
	return true;
}
