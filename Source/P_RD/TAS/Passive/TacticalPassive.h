/*****************************************************************//**
 * @file   TacticalPassive.h
 * @brief  패시브 베이스 클래스 (standalone)
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "TAS/Passive/TacticalPassiveState.h"
#include "TacticalPassive.generated.h"

struct FPassiveActivateContext;
struct FBoardCombatTargetSnapshotData;

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

	// 자동화 테스트에서만 protected 상태(mTriggerTiming 등)에 접근하기 위한 friend
	// 릴리즈 빌드에 테스트 모듈이 빠져도 이름 선언일 뿐이라 무해
	friend class FPassiveComponentModelTests;

public:
	/**
	 * @brief 패시브 효과 계산
	 *
	 * @details
	 * TargetDelta에 변화값을 누적.
	 * 패시브 내부 상태가 있으면 PassiveState를 읽어 갱신 (커밋은 CommitPassive에서).
	 *
	 * @param Ctx          소유자/대상 및 현재 스냅샷
	 * @param TargetDelta  [out] 대상에게 가할 수치 변화 누적
	 * @param PassiveState [in,out] 패시브 내부 상태 (러닝본, 무상태면 무시)
	 */
	virtual void ActivatePassive(
		IN const FPassiveActivateContext& Ctx,
		OUT FBoardCombatTargetSnapshotData& TargetDelta,
		IN OUT TInstancedStruct<FTacticalPassiveState>& PassiveState)
		PURE_VIRTUAL(UTacticalPassive::ActivatePassive, );

	/**
	 * @brief 계산된 패시브 내부 상태를 커밋
	 *
	 * @details
	 * 상태가 있는 패시브만 override 해서 구현
	 *
	 * @param PassiveState 커밋할 패시브 내부 상태
	 */
	virtual void CommitPassive(IN const TInstancedStruct<FTacticalPassiveState>& PassiveState) {}

public:
	// 발동 시점 태그 반환 (드라이버/컴포넌트가 시점별로 패시브를 모을 때 사용)
	const FGameplayTag& GetTriggerTiming() const { return mTriggerTiming; }

protected:
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
