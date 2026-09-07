#include "Misc/AutomationTest.h"

#include "GameplayTagType.h"
#include "Internationalization/Internationalization.h"
#include "UI/Combat/CombatStatusPresentation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatStatusPresentationCoverageTest,
	"P_RD.UI.CombatHUD.StatusPresentationCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatStatusPresentationCoverageTest::RunTest(const FString& Parameters)
{
	struct FScopedKoreanCulture
	{
		FString Original;
		FScopedKoreanCulture()
			: Original(FInternationalization::Get().GetCurrentCulture()->GetName())
		{
			FInternationalization::Get().SetCurrentCulture(TEXT("ko"));
		}
		~FScopedKoreanCulture()
		{
			FInternationalization::Get().SetCurrentCulture(Original);
		}
	} ScopedCulture;

	const FGameplayTag StatusTags[] = {
		EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Strength,
		EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Dexterity,
		EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Acumeny,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Vigor,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Fortification,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Haste,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Weakness,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Vulnerability,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Exhaustion,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Slow,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Frail,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Poison,
	};
	for (const FGameplayTag& StatusTag : StatusTags)
	{
		const CombatStatusUI::FPresentation Presentation =
			CombatStatusUI::Resolve(StatusTag);
		TestFalse(*FString::Printf(TEXT("%s 표시 이름"),
			*StatusTag.ToString()), Presentation.mDisplayName.IsEmpty());
		TestNotEqual(*FString::Printf(TEXT("%s 플로팅 아이콘"),
			*StatusTag.ToString()), Presentation.mFloatingIcon,
			EFloatingLogIconType::None);
	}

	TestEqual(TEXT("기절 한글 이름"), CombatStatusUI::Resolve(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun)
		.mDisplayName.ToString(), FString(TEXT("기절")));
	TestEqual(TEXT("속박 한글 이름"), CombatStatusUI::Resolve(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root)
		.mDisplayName.ToString(), FString(TEXT("속박")));
	TestTrue(TEXT("영구 상태 기간 분류"), CombatStatusUI::Resolve(
		EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Strength).mIsInfinite);
	TestTrue(TEXT("라운드 상태 기간 분류"), CombatStatusUI::Resolve(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun)
		.mIsRoundDuration);
	TestEqual(TEXT("플로팅 상태 증가 문구"), CombatStatusUI::FormatDelta(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun, 2)
		.ToString(), FString(TEXT("기절 +2")));
	TestEqual(TEXT("플로팅 상태 해제 문구"), CombatStatusUI::FormatDelta(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root, -1)
		.ToString(), FString(TEXT("속박 -1")));

	TArray<FStatusEffectUI> Statuses;
	const FGameplayTag UnsortedTags[] = {
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Vigor,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Weakness,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root,
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun,
	};
	for (const FGameplayTag& Tag : UnsortedTags)
	{
		FStatusEffectUI& Status = Statuses.AddDefaulted_GetRef();
		Status.mTag = Tag;
		Status.mStackCount = 1;
	}
	CombatStatusUI::SortForDisplay(Statuses);
	TestTrue(TEXT("기절 우선 정렬"), Statuses[0].mTag.MatchesTagExact(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun));
	TestTrue(TEXT("속박 둘째 정렬"), Statuses[1].mTag.MatchesTagExact(
		EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root));
	return true;
}
