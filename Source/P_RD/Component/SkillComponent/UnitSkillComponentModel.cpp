#include "Component/SkillComponent/UnitSkillComponentModel.h"

#include "DataAsset/SkillData/StaticUnitSkillData.h"

#include "Actor/ActorModel.h"
#include "Pawn/UnitModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Effect/Cooldown/TacticalEffect_Cooldown.h"
#include "TAS/Effect/Stat/TacticalEffect_ActionPoint.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UnitSkillComponentModel)

bool UUnitSkillComponentModel::IsAcquirableSkill_Internal(UStaticSkillData* SkillData) const
{
	UStaticUnitSkillData* UnitSkillData = Cast<UStaticUnitSkillData>(SkillData);
	if (UnitSkillData == nullptr)
	{
		/* 유닛 스킬이 아님 */
		return false;
	}

	UUnitModel* OwnerUnitModel = GetOwnerModel<UUnitModel>();
	checkf(OwnerUnitModel != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	if (UnitSkillData->mJobType != EUnitJobType::Common && OwnerUnitModel->GetUnitJobType() != UnitSkillData->mJobType)
	{
		/* 유닛 직업이 일치하지 않음 */
		return false;
	}

	return true;
}

bool UUnitSkillComponentModel::CanActiveSkill_Internal(int32 SkillIndex) const
{
	const bool CanSuperActivateSkill = Super::CanActiveSkill_Internal(SkillIndex);
	const bool HasEnoughMovement = HasRequiredActionPoint(SkillIndex);

	return CanSuperActivateSkill == true && HasEnoughMovement == true;
}

void UUnitSkillComponentModel::ConsumeResources_Internal(int32 SkillIndex)
{
	Super::ConsumeResources_Internal(SkillIndex);

	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[SkillIndex];
	const UStaticSkillData* SkillData = SkillEntry.mData;

	IBoardCombatTarget* OwnerCombatTarget = GetOwnerModel<IBoardCombatTarget>();
	checkf(OwnerCombatTarget != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	UAttributeSetComponentModel* AttributeSetCompModel = OwnerCombatTarget->GetAttributeComponentModel();
	checkf(AttributeSetCompModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	UTacticalEffectContext* EffectContext = AttributeSetCompModel->MakeEffectContext();

	/* 행동력 소모 */

	{
		TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetCompModel->MakeOutgoingSpec(UTacticalEffect_ActionPoint::StaticClass(), EffectContext);
		EffectSpec->mDynamicMagnitude = -GetRequiredActionPoint(SkillIndex);
		AttributeSetCompModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
	}
}

bool UUnitSkillComponentModel::HasRequiredActionPoint(int32 SkillIndex) const
{
	const FSkillEntry* SkillEntry = GetSkill(SkillIndex);
	if (SkillEntry == nullptr || SkillEntry->IsValid() == false)
	{
		return false;
	}

	const IBoardCombatTarget* OwnerCombatTarget = GetOwnerModel<IBoardCombatTarget>();
	if (OwnerCombatTarget == nullptr)
	{
		return false;
	}

	UAttributeSetComponentModel* AttributeSetCompModel = OwnerCombatTarget->GetAttributeComponentModel();
	if (AttributeSetCompModel == nullptr)
	{
		return false;
	}

	return StaticCast<UStaticUnitSkillData*>(SkillEntry->mData)->mRequiredActionPoint <= AttributeSetCompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute());
}

int32 UUnitSkillComponentModel::GetRequiredActionPoint(int32 SkillIndex) const
{
	const FSkillEntry* SkillEntry = GetSkill(SkillIndex);
	if (SkillEntry == nullptr || SkillEntry->IsValid() == false)
	{
		return false;
	}

	return StaticCast<UStaticUnitSkillData*>(SkillEntry->mData)->mRequiredActionPoint;
}

