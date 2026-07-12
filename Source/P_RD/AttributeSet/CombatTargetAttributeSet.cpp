#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Effect/Tag/TacticalEffect_Dead.h"

/**
 * @brief 기본 생성자.
 *        MaxHP를 FLT_MAX로 초기화하여, 별도 데이터로 덮어쓰기 전까지는
 *        PreAttributeChange의 HP 클램프 상한이 사실상 무제한이 되도록 한다.
 */
UCombatTargetAttributeSet::UCombatTargetAttributeSet() : MaxHP(FLT_MAX)
{
}

/**
 * @brief 어트리뷰트 값이 실제로 반영되기 "직전"에 호출되어 NewValue를 보정한다.
 * @param Attribute 변경 대상 어트리뷰트.
 * @param NewValue  반영 예정 값(참조). 여기서 클램프하면 그 값이 최종 반영된다.
 */
void UCombatTargetAttributeSet::PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue)
{
	/* 체력 변경 시, 체력 초과 방지 */
	if (Attribute == GetHPAttribute())
	{
		// HP는 [0, MaxHP] 범위로 클램프 — 음수(과다 피해) 및 MaxHP 초과(과다 회복)를 모두 차단한다.
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHP());
	}

	Super::PreAttributeChange(Attribute, NewValue);
}

/**
 * @brief 어트리뷰트 값이 반영된 "직후"에 호출되어 파생 효과를 처리한다.
 * @param Attribute 변경된 어트리뷰트.
 * @param OldValue  변경 전 값.
 * @param NewValue  변경 후 값(이미 반영됨).
 */
void UCombatTargetAttributeSet::PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	/* 체력 최댓값 변경 시, 체력 초과 방지 */
	if (Attribute == GetMaxHPAttribute())
	{
		// MaxHP가 줄어들어 현재 HP가 새 상한을 넘어선 경우에만 HP를 끌어내린다.
		if (GetHP() > NewValue)
		{
			UAttributeSetComponentModel* ASC = GetOwningAttributeSetComponentModel();
			// [PR #191 enum 치환 지점] 구 EGameplayModOp::Override -> ETacticalModOp::Override(정수값 3).
			// Override는 "덮어쓰기" 연산이므로, 기존 HP를 무시하고 새 MaxHP 값(NewValue)으로 강제 설정한다.
			// (정수값 3은 구 GAS와 동일하게 유지되므로 직렬화/배열 인덱싱/CoreRedirect 호환이 보장된다.)
			ASC->ApplyModToAttribute(GetHPAttribute(), ETacticalModOp::Override, NewValue);
		}
	}

	if (Attribute == GetHPAttribute() && OldValue > 0.f && NewValue <= 0.f)
	{
		UAttributeSetComponentModel* ASC = GetOwningAttributeSetComponentModel();

		UTacticalEffectContext* EffectContext = ASC->MakeEffectContext();
		EffectContext->SetInstigator(GetOwningActor());
		EffectContext->SetAttributeSetComponentModel(ASC);

		TSharedPtr<FTacticalEffectSpec> InfiniteEffect = ASC->MakeOutgoingSpec(UTacticalEffect_Dead::StaticClass(), EffectContext);
		FActiveTacticalEffectHandle ActiveHandle = ASC->ApplyTacticalEffectSpecToSelf(*InfiniteEffect);
	}
}

