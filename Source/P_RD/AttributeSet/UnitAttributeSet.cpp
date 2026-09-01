#include "AttributeSet/UnitAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "TAS/Effect/TacticalEffectContext.h"

#include "Pawn/Player/PlayerUnitModel.h"

void UUnitAttributeSet::PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue)
{
	/* 충전 값 감소 시, 마이너스 방지 */
	if (Attribute == GetRechargeActionPointAttribute() || Attribute == GetRechargeSpeedPointAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}

	Super::PreAttributeChange(Attribute, NewValue);
}

void UUnitAttributeSet::PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
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

	/* 레벨업 처리 */
	if (Attribute == GetExpAttribute())
	{
		UAttributeSetComponentModel* ASC = GetOwningAttributeSetComponentModel();
		if (ASC == nullptr)
		{
			return;
		}

		UPlayerUnitModel* PlayerModel = ASC->GetOwnerModel<UPlayerUnitModel>();
		if (PlayerModel == nullptr)
		{
			return;
		}

		PlayerModel->PostChangeExperience(OldValue, NewValue);
	}
}

