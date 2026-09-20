/*****************************************************************//**
 * @file   SkillComponentModelTests.cpp
 * @brief  스킬 컴포넌트 모델의 상태이상에 따른 스킬 사용 가능 판정 유닛테스트
 * @details
 * 속박은 스킬 사용을 막지 않고, 기절은 막는지 검증
 * @author 이문환
 * @date   2026-09-15
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h"

#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "GameplayTagType.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// @brief 테스트용 월드 획득 (PIE/Game 우선)
	UWorld* GetAnyGameWorldForSkillTests()
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

	// @brief 테스트용 더미 스킬 생성. 직업 무관, 시전 비용 0
	UStaticSkillData* MakeDummySkill(UWorld* World)
	{
		UStaticUnitSkillData* Skill = NewObject<UStaticUnitSkillData>(World);
		Skill->mJobType = EUnitJobType::Common;
		Skill->mRequiredActionPoint = 0;
		// 모션 레이어 없으면 미장착으로 판정하므로 더미 1개 추가
		Skill->mSkillPhaseLayers.AddDefaulted();
		return Skill;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkillComponentStatusEffectTests,
	"P_RD.SRPG.SkillComponent.StatusEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSkillComponentStatusEffectTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForSkillTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	// 유닛 생성 후 슬롯 0에 더미 스킬 장착
	UMockEnemyUnitModel* Unit = NewObject<UMockEnemyUnitModel>(World);
	Unit->Initialize();
	Unit->BeginPlay();

	USkillComponentModel* SkillComp = Unit->GetSkillComponentModel();
	if (TestNotNull(TEXT("스킬 컴포넌트 모델"), SkillComp) == false)
	{
		return false;
	}
	UAttributeSetComponentModel* AttrComp = Unit->GetAttributeComponentModel();
	if (TestNotNull(TEXT("속성 컴포넌트 모델"), AttrComp) == false)
	{
		return false;
	}

	SkillComp->SetSkillFrom(TArray<TSoftObjectPtr<UStaticSkillData>>());
	SkillComp->SetSkill(0, MakeDummySkill(World));

	/* [1] 기본 상태: 사용 가능 */
	TestTrue(TEXT("기본: 스킬 사용 가능"), SkillComp->CanActiveSkill(0));

	/* [2] 속박: 사용 가능 */
	AttrComp->AddLooseGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root);
	TestTrue(TEXT("속박: 스킬 사용 가능"), SkillComp->CanActiveSkill(0));

	/* [3] 기절 추가: 사용 불가 */
	AttrComp->AddLooseGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun);
	TestFalse(TEXT("기절: 스킬 사용 불가"), SkillComp->CanActiveSkill(0));

	/* [4] 기절 제거, 속박만 남음: 사용 가능 */
	AttrComp->RemoveLooseGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun);
	TestTrue(TEXT("기절 해제 후 속박만: 스킬 사용 가능"), SkillComp->CanActiveSkill(0));

	return true;
}
