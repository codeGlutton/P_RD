#include "AttributeSet/UnitAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "TAS/Effect/TacticalEffectContext.h"

/**
 * @brief 플레이어 어트리뷰트 반영 직전 보정. Exp가 음수로 내려가는 것을 막는다.
 * @param Attribute 변경 대상 어트리뷰트.
 * @param NewValue  반영 예정 값(참조).
 */
void UPlayerUnitAttributeSet::PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue)
{
	/* 경험치 감소 시, 마이너스 방지 */
	if (Attribute == GetExpAttribute())
	{
		// Exp 하한을 0으로 고정 — 경험치 차감 효과가 누적되어 음수가 되는 것을 방지한다.
		NewValue = FMath::Max(NewValue, 0.f);
	}

	Super::PreAttributeChange(Attribute, NewValue);
}

/**
 * @brief 플레이어 어트리뷰트 반영 직후 처리. Exp가 MaxExp에 도달하면 레벨업을 트리거한다.
 * @param Attribute 변경된 어트리뷰트.
 * @param OldValue  변경 전 값.
 * @param NewValue  변경 후 값(이미 반영됨).
 */
void UPlayerUnitAttributeSet::PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	/* 경험치 초과 시, 레벨 증가 */
	if (Attribute == GetExpAttribute())
	{
		// 현재 Exp가 레벨업 임계치(MaxExp) 이상이면 레벨업 처리 진입.
		if (NewValue >= GetMaxExp())
		{
			UAttributeSetComponentModel* ASC = GetOwningAttributeSetComponentModel();

			// TODO : 레벨업 시도 (ASC를 통해 Level 증가 및 Exp 초기화 모디파이어 적용 예정)
		}
	}
}
