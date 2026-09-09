#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Effect/Tag/TacticalEffect_Dead.h"

UCombatTargetAttributeSet::UCombatTargetAttributeSet() : MaxHP(FLT_MAX)
{
}

void UCombatTargetAttributeSet::PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue)
{
	/* 체력 변경 시, 체력 초과 방지 */
	if (Attribute == GetHPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHP());
	}

	Super::PreAttributeChange(Attribute, NewValue);
}

void UCombatTargetAttributeSet::PreAttributeBaseChange(const FTacticalAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	/* 체력 기본값 변경 시, 체력 초과 및 음수 방지 */
	if (Attribute == GetHPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHP());
	}
}

void UCombatTargetAttributeSet::PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	/* 체력 최댓값 변경 시, 체력 초과 방지 */
	if (Attribute == GetMaxHPAttribute())
	{
		if (GetHP() > NewValue)
		{
			UAttributeSetComponentModel* ASC = GetOwningAttributeSetComponentModel();
			if (ASC != nullptr)
			{
				ASC->ApplyModToAttribute(GetHPAttribute(), ETacticalModOp::Override, NewValue);
			}
		}
	}

	if (Attribute == GetHPAttribute() && OldValue > 0.f && NewValue <= 0.f)
	{
		UAttributeSetComponentModel* ASC = GetOwningAttributeSetComponentModel();
		if (ASC != nullptr)
		{
			UTacticalEffectContext* EffectContext = ASC->MakeEffectContext();

			TSharedPtr<FTacticalEffectSpec> InfiniteEffect = ASC->MakeOutgoingSpec(UTacticalEffect_GetDead::StaticClass(), EffectContext);
			FActiveTacticalEffectHandle ActiveHandle = ASC->ApplyTacticalEffectSpecToSelf(*InfiniteEffect);
		}
	}
}

void UCombatTargetAttributeSet::PostAttributeBaseChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);

	/* 체력 최댓값 기본값 변경 시, 체력 초과 방지 */
	if (Attribute == GetMaxHPAttribute())
	{
		if (GetHP() > NewValue)
		{
			UAttributeSetComponentModel* ASC = GetOwningAttributeSetComponentModel();
			if (ASC != nullptr)
			{
				ASC->ApplyModToAttribute(GetHPAttribute(), ETacticalModOp::Override, NewValue);
			}
		}
	}

	if (Attribute == GetHPAttribute() && OldValue > 0.f && NewValue <= 0.f)
	{
		UAttributeSetComponentModel* ASC = GetOwningAttributeSetComponentModel();
		if (ASC != nullptr)
		{
			UTacticalEffectContext* EffectContext = ASC->MakeEffectContext();

			TSharedPtr<FTacticalEffectSpec> InfiniteEffect = ASC->MakeOutgoingSpec(UTacticalEffect_GetDead::StaticClass(), EffectContext);
			FActiveTacticalEffectHandle ActiveHandle = ASC->ApplyTacticalEffectSpecToSelf(*InfiniteEffect);
		}
	}
}

