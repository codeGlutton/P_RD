#include "AttributeSet/UnitAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UUnitAttributeSet::UUnitAttributeSet() : MaxHP(FLT_MAX)
{
}

void UUnitAttributeSet::PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue)
{
	/* 체력 변경 시, 체력 초과 방지 */
	if (Attribute == GetHPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHP());
	}

	Super::PreAttributeChange(Attribute, NewValue);
}

void UUnitAttributeSet::PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	/* 체력 최댓값 변경 시, 체력 초과 방지 */
	if (Attribute == GetMaxHPAttribute())
	{
		if (GetHP() > NewValue)
		{
			UAttributeSetComponentModel* ASC = GetOwningAttributeSetComponentModel();
			ASC->ApplyModToAttribute(GetHPAttribute(), ETacticalModOp::Override, NewValue);
		}
	}
}

UPlayerUnitAttributeSet::UPlayerUnitAttributeSet()
{
}

void UPlayerUnitAttributeSet::PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue)
{
	/* 경험치 감소 시, 마이너스 방지 */
	if (Attribute == GetExpAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}

	Super::PreAttributeChange(Attribute, NewValue);
}

void UPlayerUnitAttributeSet::PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	/* 경험치 초과 시, 레벨 증가 */
	if (Attribute == GetExpAttribute())
	{
		if (NewValue >= GetMaxExp())
		{
			UAttributeSetComponentModel* ASC = GetOwningAttributeSetComponentModel();

			// TODO : 레벨업 시도
		}
	}
}
