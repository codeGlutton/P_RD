/**
 * @file TacticalEffectType.cpp
 * @brief 전술 이펙트 모디파이어 "연산 종류"(ETacticalModOp) 유틸리티 구현.
 *
 * [마이그레이션 배경 / GAS 폐기]
 *  본 PR(#191)은 효과 모디파이어의 연산 종류 enum을 GAS의 EGameplayModOp에서
 *  자체 ETacticalModOp(TacticalEffectType.h)로 치환한 결과물이다. 즉 이 파일의
 *  TacticalEffectUtilities는 구 엔진 헬퍼 FGameplayEffectUtilities를 대체한다.
 *
 *  ★중요: ETacticalModOp의 "정수값"은 구 EGameplayModOp와 동일하게 유지된다.
 *         이름만 바뀌고 숫자는 그대로다. 그 이유는 다음 3가지다.
 *   (1) 직렬화 호환: 기존 .uasset/세이브에는 enum이 "정수"로 박혀 있다. 값을
 *       바꾸면 구 데이터가 엉뚱한 연산으로 해석되므로 정수값을 보존해야 한다.
 *   (2) 배열 인덱싱: Aggregator는 mMods[ETacticalModOp::Max]처럼 op 값 자체를
 *       배열 인덱스로 쓴다(본 파일의 ModifierOpBiases[] 도 동일). 값이 곧 슬롯
 *       번호이므로 연속·고정이어야 한다.
 *   (3) CoreRedirect: DefaultEngine.ini가 구 enum '이름'을 새 enum 이름으로
 *       매핑한다. 값이 같아야 리다이렉트 후에도 의미가 보존된다.
 *
 * [연산 종류와 정수값 — 구 EGameplayModOp 대응]
 *   AddBase(0)          : 합산.           구 Additive       와 동일 슬롯/값.
 *   MultiplyAdditive(1) : 배율 가산.      구 Multiplicitive 와 동일 슬롯/값.
 *   DivideAdditive(2)   : 나눗셈 가산.    구 Division       와 동일 슬롯/값.
 *   Override(3)         : 덮어쓰기.       구 Override       와 동일 슬롯/값.
 *   MultiplyCompound(4) : 거듭제곱 곱(복리식 곱셈). 신규 의미 슬롯.
 *   AddFinal(5)         : 최종 합산(다른 연산 적용 후 마지막에 더함). 신규 슬롯.
 *   Max(6)              : 무효/개수(enum 끝 표식, 배열 크기 = op 개수).
 *   ※ 헤더에는 구 GAS 이름(Additive/Multiplicitive/Division/Override) 별칭이
 *     같은 정수값으로 남아 하위호환을 보장한다.
 *
 * @author 박용수
 * @date 2026-06-26
 */

#include "TAS/Effect/TacticalEffectType.h"

/**
 * @brief 연산 종류별 "항등값(bias)"을 반환한다.
 *
 * 항등값이란 해당 연산에서 "효과 없음"을 뜻하는 기준값이다.
 *  - 곱셈 계열(MultiplyAdditive, ...)은 1.0 (1을 곱하면 변화 없음)
 *  - 합산 계열(AddBase, Override, ...)은 0.0 (0을 더하면 변화 없음)
 * 스택 크기 계산(ComputeStackedModifierMagnitude)에서 "순수 증분"만 스택만큼
 * 키우기 위해, 먼저 이 항등값을 빼고 연산 후 다시 더하는 용도로 쓰인다.
 *
 * @param ModOp 연산 종류(ETacticalModOp). 정수값이 곧 배열 인덱스다.
 * @return 해당 연산의 항등값(곱셈계열=1.f, 합산계열=0.f).
 */
float TacticalEffectUtilities::GetModifierBiasByModifierOp(ETacticalModOp::Type ModOp)
{
	// 인덱스 = ETacticalModOp 정수값(구 EGameplayModOp와 동일). op 값을 그대로 배열
	// 첨자로 쓰므로 값이 연속·고정이어야 한다. 배열 길이는 op 개수인 Max(6)와 일치.
	// 각 슬롯의 항등값(아래 리터럴 순서와 일대일 대응):
	//   [0] AddBase          = 0.f  (합산 → 0을 더하면 무변화)
	//   [1] MultiplyAdditive = 1.f  (배율 가산 → 1배면 무변화)
	//   [2] DivideAdditive   = 1.f  (나눗셈 가산 → 1로 나누면 무변화)
	//   [3] Override         = 0.f  (덮어쓰기 → 스택 계산에서 별도 제외, 값 무의미)
	//   [4] MultiplyCompound = 0.f  (거듭제곱 곱의 기준값으로 사용)
	//   [5] AddFinal         = 0.f  (최종 합산 → 0을 더하면 무변화)
	static const float ModifierOpBiases[ETacticalModOp::Max] = { 0.f, 1.f, 1.f, 0.f, 0.f, 0.f };
	// 인덱스 범위 보호: op는 [0, Max) 안이어야 배열 오버런이 없다.
	check(ModOp >= 0 && ModOp < ETacticalModOp::Max);

	return ModifierOpBiases[ModOp];
}

/**
 * @brief 스택(중첩) 수에 따라 모디파이어 크기를 보정해 반환한다.
 *
 * 1스택 기준 크기(BaseComputedMagnitude)를 StackCount 만큼 적용했을 때의 합성
 * 크기를 구한다. 핵심 아이디어: 연산의 "항등값(bias)"을 기준으로 한 "순수 증분"만
 * 스택 수에 비례시키고, 마지막에 항등값을 복원한다.
 *   예) MultiplyAdditive(항등 1.0)에서 1스택이 1.2배라면 증분은 0.2.
 *       3스택이면 (1.2-1)*3 + 1 = 1.6배.  → 합산식 분기.
 *   예) MultiplyCompound(거듭제곱 곱)는 증분을 더하는 대신 거듭제곱한다.
 *       1스택이 1.2배라면 3스택은 1.2^3.  → Pow 분기.
 *
 * @param BaseComputedMagnitude 1스택 기준 모디파이어 크기.
 * @param StackCount            적용 스택 수(아래에서 0 미만으로 떨어지지 않게 클램프).
 * @param ModOp                 연산 종류(ETacticalModOp). 분기 기준.
 * @return 스택이 반영된 최종 모디파이어 크기.
 */
float TacticalEffectUtilities::ComputeStackedModifierMagnitude(float BaseComputedMagnitude, int32 StackCount, ETacticalModOp::Type ModOp)
{
	// 이 연산의 항등값(곱셈계열=1, 합산계열=0). 증분 분리/복원의 기준이 된다.
	const float OperationBias = GetModifierBiasByModifierOp(ModOp);

	StackCount = FMath::Clamp<int32>(StackCount, 0, StackCount);

	float StackMag = BaseComputedMagnitude;

	// Override(덮어쓰기)는 스택과 무관하게 항상 같은 값을 강제하므로 보정하지 않는다.
	// 나머지는 "항등값을 빼서 순수 증분만 남긴 뒤" 스택을 적용하고 다시 항등값을 더한다.
	if (ModOp != ETacticalModOp::Override)
	{
		StackMag -= OperationBias;					// 증분만 분리 (곱셈계열은 -1, 합산계열은 -0)
		if (ModOp == ETacticalModOp::MultiplyCompound)
		{
			// 거듭제곱 곱: 스택마다 곱이 누적(복리)되므로 증분을 StackCount 제곱한다.
			StackMag = FMath::Pow(StackMag, StackCount);
		}
		else
		{
			// 그 외 모든 연산: 증분을 스택 수에 선형 비례시킨다.
			StackMag *= StackCount;
		}
		StackMag += OperationBias;					// 항등값 복원 → 다시 "배율/합산값" 형태로
	}

	return StackMag;
}

/**
 * @brief 연산 종류(ETacticalModOp) 값을 디버그/로그용 문자열로 변환한다.
 *
 * 구 GAS의 enum→문자열 헬퍼를 대체한다. case 라벨에 ETacticalModOp 이름을 쓰지만
 * 실제 비교는 정수값으로 이뤄지므로(=구 EGameplayModOp 값과 동일), 정수로 박힌
 * 구 데이터/로그와도 매칭된다. 정의되지 않은 값은 "Invalid"로 처리한다.
 *
 * @param Type 연산 종류의 정수값.
 * @return 사람이 읽을 수 있는 연산 이름. 알 수 없으면 "Invalid".
 */
FString TacticalEffectUtilities::TacticalModOpToString(int32 Type)
{
	switch (Type)
	{
	case ETacticalModOp::AddBase:			return TEXT("AddBase");
	case ETacticalModOp::MultiplyAdditive:	return TEXT("MultiplyAdditive");
	case ETacticalModOp::DivideAdditive:	return TEXT("DivideAdditive");
	case ETacticalModOp::Override:			return TEXT("Override");
	case ETacticalModOp::MultiplyCompound:	return TEXT("MultiplyCompound");
	case ETacticalModOp::AddFinal:			return TEXT("AddFinal");
	default:								return TEXT("Invalid");
	}
}
