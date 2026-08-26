#include "Misc/AutomationTest.h"

#include "HAL/IConsoleManager.h"
#include "UI/Combat/CombatUIDebugFixture.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatUIDebugFixtureTest,
	"P_RD.UI.CombatHUD.DebugFixture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCombatUIDebugFixtureTest::RunTest(const FString& Parameters)
{
	IConsoleVariable* Fixture = IConsoleManager::Get().FindConsoleVariable(
		TEXT("rd.Debug.CombatUIFixture"));
	if (TestNotNull(TEXT("fixture console variable"), Fixture) == false)
	{
		return false;
	}
	const int32 OriginalMode = Fixture->GetInt();
	IConsoleVariable* ActualHPFixture = IConsoleManager::Get().FindConsoleVariable(
		TEXT("rd.Debug.ForceCombatHPOne"));
	if (TestNotNull(TEXT("all-unit actual HP console variable"), ActualHPFixture) == false)
	{
		return false;
	}
	const int32 OriginalActualHPFixture = ActualHPFixture->GetInt();
	ActualHPFixture->Set(0, ECVF_SetByConsole);
	Fixture->Set(3, ECVF_SetByConsole);
	TestEqual(TEXT("fixture mode 3"), CombatUIDebugFixture::GetMode(), 3);
	TestFalse(TEXT("mode 3 never mutates actual HP/persist state"),
		CombatUIDebugFixture::ShouldMutateActualHPOne());

	const float ActualAttributeHP = 37.f;
	const float PersistedHP = 37.f;
	FUnitUI FixtureUnitUI;
	FixtureUnitUI.mHP = ActualAttributeHP;
	FixtureUnitUI.mMaxHP = 100.f;
	CombatUIDebugFixture::ApplyDisplayHPOne(true, OUT FixtureUnitUI);
	TestEqual(TEXT("mode 3 displays HP one in DTO"), FixtureUnitUI.mHP, 1.f);
	TestEqual(TEXT("display fixture preserves max HP"), FixtureUnitUI.mMaxHP, 100.f);
	TestEqual(TEXT("display fixture leaves actual attribute value unchanged"),
		ActualAttributeHP, 37.f);
	TestEqual(TEXT("display fixture leaves persist value unchanged"), PersistedHP, 37.f);

	FUnitUI DeadUnitUI;
	DeadUnitUI.mHP = 0.f;
	DeadUnitUI.mMaxHP = 100.f;
	CombatUIDebugFixture::ApplyDisplayHPOne(false, OUT DeadUnitUI);
	TestEqual(TEXT("dead unit stays at zero"), DeadUnitUI.mHP, 0.f);

	for (int32 SideIndex = 0; SideIndex < 3; ++SideIndex)
	{
		TArray<FStatusEffectUI> Statuses;
		CombatUIDebugFixture::AppendStatuses(false, SideIndex, OUT Statuses);
		TestEqual(*FString::Printf(TEXT("enemy fixture slot %d count"), SideIndex),
			Statuses.Num(), SideIndex == 2 ? 2 : 1);
		if (SideIndex == 0 || SideIndex == 2)
		{
			TestTrue(TEXT("buff fixture present"), Statuses.ContainsByPredicate(
				[](const FStatusEffectUI& Status)
				{
					return Status.mTag.ToString().Contains(TEXT(".Buff."));
				}));
		}
		if (SideIndex == 1 || SideIndex == 2)
		{
			TestTrue(TEXT("debuff fixture present"), Statuses.ContainsByPredicate(
				[](const FStatusEffectUI& Status)
				{
					return Status.mTag.ToString().Contains(TEXT(".Debuff."));
				}));
		}
	}

	Fixture->Set(0, ECVF_SetByConsole);
	ActualHPFixture->Set(1, ECVF_SetByConsole);
	TestTrue(TEXT("development fixture can mutate every unit's actual HP"),
		CombatUIDebugFixture::ShouldMutateActualHPOne());
	ActualHPFixture->Set(0, ECVF_SetByConsole);
	FUnitUI DisabledUnitUI;
	DisabledUnitUI.mHP = 37.f;
	CombatUIDebugFixture::ApplyDisplayHPOne(true, OUT DisabledUnitUI);
	TestEqual(TEXT("mode 0 preserves DTO HP"), DisabledUnitUI.mHP, 37.f);
	TArray<FStatusEffectUI> DisabledStatuses;
	CombatUIDebugFixture::AppendStatuses(false, 2, OUT DisabledStatuses);
	TestTrue(TEXT("mode 0 does not mutate production DTO"), DisabledStatuses.IsEmpty());
	Fixture->Set(OriginalMode, ECVF_SetByConsole);
	ActualHPFixture->Set(OriginalActualHPFixture, ECVF_SetByConsole);
	return true;
}

#endif
