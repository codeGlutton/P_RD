# Tactical Effect 최초 스택 0 유지 가능성 조사

## 배경

`FTacticalEffectSpec`의 스택 수 기본값은 0이다. 초기화 과정에서 최초 적용 스택을 1로 설정하는 처리가 보이지 않으며, 기존 Effect와 합칠 때는 기존 값과 입력 Spec 값을 더한다.

## 예상 시나리오

- `mFactorInStackCount`가 활성화된 스택형 Effect를 처음 적용한다.
- Spec의 스택 수가 0인 상태로 모디파이어 크기를 계산한다.
- Add 계열은 0, Compound 계열은 항등값으로 계산되어 Effect가 적용되지 않는 것처럼 보일 가능성이 있다.
- 동일 Effect를 재적용해도 `0 + 0`으로 유지될 가능성이 있다.

## 근거

- `Source/P_RD/TAS/Effect/TacticalEffect.h:145-148`
- `Source/P_RD/TAS/Effect/TacticalEffect.cpp:50-69`
- `Source/P_RD/TAS/Effect/ActiveTacticalEffectsContainer.cpp:336-343`

## 확인 항목

- AddBase 및 MultiplyCompound 스택 Effect를 1회와 2회 적용해 스택 수와 최종 수치를 확인한다.
- 최초 스택 기본 계약이 0인지 1인지 확정한다.
- C++ 기본 Effect와 BP/데이터 작성 Effect에서 `mFactorInStackCount` 사용 현황을 확인한다.

## 범위

이 문서는 신규 스택형 Effect에서 발생할 수 있는 문제를 조사하기 위한 초안이다. TAS 계산 및 기본값은 변경하지 않는다.
