/*****************************************************************//**
 * @file   TacticalPassive.h
 * @brief  패시브 베이스 클래스 (standalone)
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "TAS/Passive/TacticalPassiveState.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "TacticalPassive.generated.h"

struct FPassiveActivateContext;
struct FBoardCombatTargetSnapshotData;
class UTacticalEffect;

/**
 * @brief 패시브 베이스 클래스
 *
 * @details
 * 전투 중 특정 시점(타격 전/후, 피격 전/후 등)에 발동하는 패시브의 베이스.
 * 액티브 스킬과 달리 플레이어가 직접 발동하지 않고, 타이밍 태그에 맞춰서 호출됨.
 *
 * 계산할 때는 내부 상태를 바꾸지 않으며 적용 함수가 호출될 때만 내부 상태를 변경.
 *
 * @par 상태 보유 여부
 * 스택 카운터 같은 내부 상태가 필요한 패시브만 mState를 사용함.
 * 상태가 없는 패시브는 mState를 비워두고 PassiveState를 무시하면 됨.
 */
UCLASS(Abstract, Blueprintable)
class P_RD_API UTacticalPassive : public UObject
{
	GENERATED_BODY()

	/**
	 * @brief 유닛테스트 모듈이 패시브 객체에 접근하기 위해 friend 선언
	 * @note 릴리즈 빌드에서는 테스트 모듈이 빠져도 전방선언만 했기 때문에 영향 없음
	 */ 
	// 자동화 테스트에서만 protected 상태(mTriggerTiming 등)에 접근하기 위한 friend
	friend class FPassiveComponentModelTests;
	// 패시브별 단위 테스트가 protected(EvaluatePassive/mState)에 접근하기 위한 friend
	friend class FTacticalPassiveAddAttackPointTests;
	friend class FTacticalPassiveNthAddAttackPointCalcTests;
	friend class FTacticalPassiveNthAddAttackPointCommitTests;

public:
	/**
	 * @brief 패시브 발동 (계산 -> 적용 연쇄, 고정)
	 *
	 * @details
	 * 내부적으로 EvaluatePassive(계산) -> NotifyPassive(적용) 순서로 호출.
	 * 계산 결과(기여값)는 내부 로컬에만 존재하고 외부로 반환하지 않는다 (이펙트로 바로 적용).
	 * 고정 체인이라 일반 함수(non-virtual)로 두며, 서브클래스는 이 함수가 아니라
	 * EvaluatePassive를 구현한다.
	 *
	 * @param Ctx          소유자/대상 및 현재 스냅샷
	 * @param PassiveState [in,out] 패시브 내부 상태 (러닝본, 무상태면 무시)
	 */
	void ActivatePassive(
		IN const FPassiveActivateContext& Ctx,
		IN OUT TInstancedStruct<FTacticalPassiveState>& PassiveState);

	/**
	 * @brief 계산된 패시브 내부 상태를 커밋
	 *
	 * @details
	 * 상태가 있는 패시브만 override 해서 구현
	 *
	 * @param PassiveState 커밋할 패시브 내부 상태
	 */
	virtual void CommitPassive(IN const TInstancedStruct<FTacticalPassiveState>& PassiveState) {}

	/**
	 * @brief 적용 중인 이펙트를 강제로 해제(제거)
	 *
	 * @details
	 * NotifyPassive에서 저장해 둔 핸들로 이펙트를 활성 집합에서 제거한다.
	 * 되돌리는 수치 계산은 없으며, 제거 후 TAS가 base에서 재계산해 기여가 빠진 값이 된다.
	 * 조건을 따지지 않고 즉시 내린다(예: 장비 장착 해제).
	 */
	void DeactivatePassive();

	/**
	 * @brief 조건부 해제: 패시브에게 빠질지 물어보고, 빠진다고 하면 해제
	 *
	 * @details
	 * 강제로 내리는 DeactivatePassive와 달리, 드라이버가 "빠질래?"라고 물으면
	 * 패시브가 ShouldDeactivate로 스스로 판단한다. true일 때만 DeactivatePassive 수행.
	 * 적용 중이 아니거나(핸들 무효) 유지 판단이면 아무 것도 하지 않는다.
	 *
	 * @return 실제로 해제됐으면 true
	 */
	bool TryDeactivatePassive();

public:
	// 발동 시점 태그 반환 (드라이버/컴포넌트가 시점별로 패시브를 모을 때 사용)
	const FGameplayTag& GetTriggerTiming() const { return mTriggerTiming; }

protected:
	/**
	 * @brief 패시브 효과 계산 (순수)
	 *
	 * @details
	 * TargetDelta에 변화값을 누적. 내부 상태는 읽기만 하고 실제로 바꾸지 않으며,
	 * 다음 상태가 어떻게 변해야 할지는 PassiveState(러닝본)에 기록 (커밋은 CommitPassive).
	 * 서브클래스가 구현한다.
	 *
	 * @param Ctx          소유자/대상 및 현재 스냅샷
	 * @param TargetDelta  [out] 대상에게 가할 수치 변화 누적
	 * @param PassiveState [in,out] 패시브 내부 상태 (러닝본, 무상태면 무시)
	 */
	virtual void EvaluatePassive(
		IN const FPassiveActivateContext& Ctx,
		OUT FBoardCombatTargetSnapshotData& TargetDelta,
		IN OUT TInstancedStruct<FTacticalPassiveState>& PassiveState)
		PURE_VIRTUAL(UTacticalPassive::EvaluatePassive, );

	/**
	 * @brief 계산된 기여값을 드라이버가 원하는 형태로 출력
	 *
	 * @details
	 * 기본은 OUT 모드(TargetDelta를 그대로 둠). 이펙트 발사 등 출력 방식이
	 * 달라지면 베이스에서 이 함수만 교체하면 되고, 서브클래스는 영향 없음.
	 *
	 * @param Ctx         소유자/대상 및 현재 스냅샷
	 * @param TargetDelta [in,out] 계산된 수치 변화
	 */
	virtual void NotifyPassive(
		IN const FPassiveActivateContext& Ctx,
		IN OUT FBoardCombatTargetSnapshotData& TargetDelta);

	/**
	 * @brief 조건부 해제 판단 (TryDeactivatePassive에서 사용)
	 *
	 * @details
	 * 기본은 false(유지). 만료·조건 종료 등으로 스스로 빠져야 하는 패시브만 override.
	 * 판단 근거(잔여 턴/스택, 현재 스냅샷 등)는 추후 입력으로 확장 가능.
	 */
	virtual bool ShouldDeactivate() const { return false; }

	/**
	 * @brief 이 패시브가 적용할 이펙트
	 *
	 * @details
	 * 대상 속성·연산(op)·지속정책(Infinite/Instant)은 이 이펙트가 정의하고,
	 * 패시브 계산은 크기(magnitude)만 공급한다.
	 * (이펙트의 기본 modifier 크기는 1로 두고, 계산된 값을 배율 mDynamicMagnitude로 주입)
	 * 미설정이면 NotifyPassive가 이펙트 적용을 건너뜀.
	 */
	UPROPERTY(Category = "Passive", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Effect"))
	TSubclassOf<UTacticalEffect> mEffectClass;

	/**
	 * @brief 적용 중인 이펙트 핸들 (단일)
	 *
	 * @details
	 * NotifyPassive 적용 시 저장하고 DeactivatePassive 제거 시 사용 후 리셋.
	 * 동시에 살아있는 이펙트는 하나라는 전제(직렬 apply/deactivate).
	 */
	FActiveTacticalEffectHandle mActiveHandle;

	/**
	 * @brief 발동 시점 태그
	 *
	 * @details
	 * 어느 시점 버킷에서 발동할지 표시(예: 타격 전, 피격 후).
	 * 드라이버가 이 태그로 패시브를 모아 호출하므로, 패시브 자신의 시점 검사는 불필요.
	 */
	UPROPERTY(Category = "Passive", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "TriggerTiming"))
	FGameplayTag mTriggerTiming;

	/**
	 * @brief 커밋된 패시브 내부 상태
	 *
	 * @details
	 * 스택 카운터 등 패시브마다 다른 내부 상태를 FTacticalPassiveState 파생형으로 저장.
	 */
	UPROPERTY(Category = "Passive", VisibleAnywhere, meta = (DisplayName = "State"))
	TInstancedStruct<FTacticalPassiveState> mState;
};
