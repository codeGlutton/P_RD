# HP Base/Current 불일치 가능성 조사

## 배경

HP 변경 경로에서 Base 값과 Current 값에 서로 다른 보정 규칙이 적용될 가능성이 있다. `ApplyModToAttribute`는 기존 Base에 연산한 값을 `SetAttributeBaseValue`로 전달하며, 해당 경로에서는 `PreAttributeBaseChange` 이후 Base를 저장한다. 한편 `UCombatTargetAttributeSet`의 HP 범위 제한은 `PreAttributeChange`에 구현되어 있어 Current 값 갱신에는 반영되더라도 저장된 Base 값까지 같은 범위로 정규화되는지는 확인이 필요하다.

이 메모는 코드 흐름을 바탕으로 한 조사 가설이며, 실제 런타임에서 문제가 발생한다고 확정하지 않는다.

## 예상 시나리오

1. 최대 HP와 현재 HP가 100인 대상에게 50 회복을 적용한다.
2. Current는 100으로 제한되지만 Base가 150으로 남을 가능성이 있다.
3. 이후 30 피해를 적용하면 Base가 120으로 계산되고 Current가 다시 100으로 제한되어, 화면상 피해가 반영되지 않은 것처럼 보일 수 있다.

반대 방향으로는 HP 10인 대상에게 30 피해를 적용해 Base가 -20으로 남은 뒤 10 회복을 적용할 경우, Base가 -10으로 계산되어 Current가 계속 0에 머무르는 상황도 예상할 수 있다. 두 흐름 모두 재현 및 로그 확인이 필요하다.

## 근거

- `Source/P_RD/TAS/Effect/ActiveTacticalEffectsContainer.cpp:298-305`
  - 현재 Base에 모디파이어 연산을 적용한 결과를 별도 범위 제한 없이 `SetAttributeBaseValue`로 전달한다.
- `Source/P_RD/TAS/Effect/ActiveTacticalEffectsContainer.cpp:675-710`
  - `PreAttributeBaseChange` 호출 후 Base를 저장하고, Aggregator 유무에 따라 Current 재계산 경로가 나뉜다.
- `Source/P_RD/AttributeSet/CombatTargetAttributeSet.cpp:20-29`
  - HP의 `[0, MaxHP]` 제한은 `PreAttributeChange`에 구현되어 있다.

## 확인 항목

- 회복·피해 직후 HP의 Base와 Current를 함께 기록해 두 값이 범위를 벗어나 분리되는지 확인한다.
- Aggregator가 있는 경우와 없는 경우를 나눠 동일 시나리오를 재현한다.
- 오버힐 후 피해, 과잉 피해 후 회복을 연속 적용하는 자동화 테스트를 추가할 필요가 있는지 검토한다.
- HP Base에도 `[0, MaxHP]` 정책을 적용하는 것이 전투 및 지속 효과 설계와 일치하는지 확인한다.
- MaxHP 자체가 변경되는 상황에서 Base와 Current의 기대 동작도 함께 정리한다.

## 범위

조사를 위한 메모만 추가한다. 런타임 코드, 데이터, 테스트는 수정하지 않는다.
