#include "Misc/AutomationTest.h"

#include "Engine/Texture2D.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/Combat/SkillDetailUIBuilder.h"
#include "UI/RoomViewTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkillDetailDtoFallbackTest,
	"P_RD.UI.SkillDetail.DtoFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkillDetailDtoFallbackTest::RunTest(const FString& Parameters)
{
	UTexture2D* RosterIcon = NewObject<UTexture2D>(GetTransientPackage());
	FPartyRosterSkillView RosterSkill;
	RosterSkill.mSlotIndex = 3;
	RosterSkill.mName = FText::FromString(TEXT("명단 스킬"));
	RosterSkill.mIcon = RosterIcon;
	RosterSkill.mActionPointCost = INDEX_NONE;

	FSkillDetailUI RosterDetail;
	TestTrue(TEXT("파티 명단 View로 최소 상세 생성"),
		SkillDetailUIBuilder::FillFallbackFromPartyRosterSkillView(
			RosterSkill, RosterDetail));
	TestEqual(TEXT("파티 명단 슬롯 index 보존"), RosterDetail.mSkillIndex, 3);
	TestEqual(TEXT("파티 명단 이름 보존"), RosterDetail.mName, RosterSkill.mName);
	TestEqual(TEXT("파티 명단 아이콘 보존"),
		RosterDetail.mIcon.Get(), RosterIcon);
	TestEqual(TEXT("표시 불가 AP는 0으로 정규화"),
		RosterDetail.mActionPointCost, 0);
	TestFalse(TEXT("파티 명단 fallback 설명 제공"),
		RosterDetail.mDescription.IsEmpty());

	UTexture2D* CombatIcon = NewObject<UTexture2D>(GetTransientPackage());
	FSkillUI CombatSkill;
	CombatSkill.mSkillIndex = 5;
	CombatSkill.mName = FText::FromString(TEXT("전투 카드 스킬"));
	CombatSkill.mIcon = CombatIcon;
	CombatSkill.mActionPointCost = 7;

	FSkillDetailUI PartialDetail;
	PartialDetail.mSkillIndex = 5;
	PartialDetail.mDamageMin = 12;
	PartialDetail.mDamageMax = 18;
	TestTrue(TEXT("같은 슬롯 전투 카드 View로 부분 상세 보완"),
		SkillDetailUIBuilder::FillFallbackFromCombatSkillView(
			CombatSkill, PartialDetail));
	TestEqual(TEXT("전투 카드 이름 보완"), PartialDetail.mName, CombatSkill.mName);
	TestEqual(TEXT("전투 카드 아이콘 보완"),
		PartialDetail.mIcon.Get(), CombatIcon);
	TestEqual(TEXT("전투 카드 AP 보완"), PartialDetail.mActionPointCost, 7);
	TestFalse(TEXT("전투 카드 fallback 설명 제공"),
		PartialDetail.mDescription.IsEmpty());
	TestEqual(TEXT("생산자가 준 피해 최소값 유지"), PartialDetail.mDamageMin, 12);
	TestEqual(TEXT("생산자가 준 피해 최대값 유지"), PartialDetail.mDamageMax, 18);

	FSkillDetailUI MismatchedDetail;
	MismatchedDetail.mSkillIndex = 9;
	TestFalse(TEXT("다른 슬롯 View는 상세를 보완하지 않음"),
		SkillDetailUIBuilder::FillFallbackFromCombatSkillView(
			CombatSkill, MismatchedDetail));
	TestTrue(TEXT("다른 슬롯 상세 이름은 비어 있음"),
		MismatchedDetail.mName.IsEmpty());

	return !HasAnyErrors();
}

#endif
