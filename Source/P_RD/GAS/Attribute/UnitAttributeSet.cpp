#include "GAS/Attribute/UnitAttributeSet.h"

UUnitAttributeSet::UUnitAttributeSet() : MaxHP(FLT_MAX)
{
}

void UUnitAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	/* 체력 변경 시, 체력 초과 방지 */
	if (Attribute == GetHPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0, GetMaxHP());
	}

	Super::PreAttributeChange(Attribute, NewValue);
}

void UUnitAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	/* 체력 최댓값 변경 시, 체력 초과 방지 */
	if (Attribute == GetMaxHPAttribute())
	{
		if (GetHP() > NewValue)
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			ASC->ApplyModToAttribute(GetHPAttribute(), EGameplayModOp::Override, NewValue);
		}
	}
}
