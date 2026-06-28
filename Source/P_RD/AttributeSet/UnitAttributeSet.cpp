/**
 * @file   UnitAttributeSet.cpp
 * @brief  유닛 어트리뷰트 셋 구현부.
 *         UUnitAttributeSet(공통 유닛: HP/MaxHP 클램프)과
 *         UPlayerUnitAttributeSet(플레이어: Exp 마이너스 방지/레벨업 훅)의
 *         Pre/PostAttributeChange 콜백을 정의한다.
 *
 *         [PR #191 마이그레이션 근거]
 *         이 PR은 효과 모디파이어의 "연산 종류" enum을 GAS의 EGameplayModOp에서
 *         자체 ETacticalModOp(TacticalEffectType.h)로 치환한다(GAS 폐기 작업의 일부).
 *         본 파일에서는 PostAttributeChange의 ApplyModToAttribute 호출 인자가
 *         EGameplayModOp::Override -> ETacticalModOp::Override 로 바뀐 것이 그 치환 지점이다.
 *
 *         ETacticalModOp의 정수값은 구 EGameplayModOp와 "동일하게" 유지한다. 이유 3가지:
 *           (1) 직렬화 호환  : 기존 에셋/세이브에 박힌 op의 정수값을 그대로 보존한다.
 *           (2) 배열 인덱싱  : Aggregator가 mMods[ETacticalModOp::Max]처럼 op 값으로 배열을 인덱싱한다.
 *           (3) CoreRedirect : DefaultEngine.ini의 CoreRedirect가 구 enum 이름을 새 enum으로 매핑한다.
 *
 *         연산 종류(정수값): AddBase(0)=합산, MultiplyAdditive(1)=배율 가산,
 *         DivideAdditive(2)=나눗셈 가산, Override(3)=덮어쓰기,
 *         MultiplyCompound(4)=거듭제곱 곱, AddFinal(5)=최종 합산, Max(6)=무효/개수.
 *         (+ 구 GAS 이름 하위호환 별칭: Additive=0 / Multiplicitive=1 / Division=2 / Override=3)
 *
 * @author 박용수
 * @date   2026-06-26
 */
#include "AttributeSet/UnitAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

/**
 * @brief 기본 생성자.
 *        MaxHP를 FLT_MAX로 초기화하여, 별도 데이터로 덮어쓰기 전까지는
 *        PreAttributeChange의 HP 클램프 상한이 사실상 무제한이 되도록 한다.
 */
UUnitAttributeSet::UUnitAttributeSet() : MaxHP(FLT_MAX)
{
}

/**
 * @brief 어트리뷰트 값이 실제로 반영되기 "직전"에 호출되어 NewValue를 보정한다.
 * @param Attribute 변경 대상 어트리뷰트.
 * @param NewValue  반영 예정 값(참조). 여기서 클램프하면 그 값이 최종 반영된다.
 */
void UUnitAttributeSet::PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue)
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
void UUnitAttributeSet::PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue)
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
}

/**
 * @brief 플레이어 전용 어트리뷰트 셋 기본 생성자.
 */
UPlayerUnitAttributeSet::UPlayerUnitAttributeSet()
{
}

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
