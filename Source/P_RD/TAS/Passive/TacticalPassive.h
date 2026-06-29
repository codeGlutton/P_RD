/*****************************************************************//**
 * @file   TacticalPassive.h
 * @brief  패시브 베이스 클래스 (standalone)
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "TAS/Passive/DynamicPassiveData.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "TacticalPassive.generated.h"

struct FPassiveActivateContext;
struct FBoardCombatTargetSnapshotData;
class UTacticalEffect;
class UStaticPassiveData;

/**
 * @brief 패시브의 발동/해제 선택
 *
 * @details
 * 드라이버는 패시브의 발동/해제를 모르고 ActivatePassive 함수를 호출하므로,
 * 패시브가 DecideAction에서 자신의 행동을 선택
 */
enum class EPassiveAction : uint8
{
	None,			// 무시 (효과 변화 없음, 내부 상태만 전진할 수 있음)
	Activate,		// 발동 (이펙트 적용)
	Deactivate		// 해제 (이펙트 제거)
};

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
	friend class FPassiveComponentModelTests;
	friend class FTacticalPassiveAddStatTests;
	friend class FTacticalPassiveNthAddStatCalcTests;
	friend class FTacticalPassiveNthAddStatCommitTests;

public:
	/**
	 * @brief 패시브 발동/해제/무시 요청
	 *
	 * @details
	 * 드라이버는 타이밍태그마다 패시브의 발동/해제 여부를 모른 채 이 함수를 호출.
	 * 
	 * 내부적으로 발동/해제/무시를 결정하고,
	 * 발동이면 NotifyPassive(발동=적용), 해제면 DeactivatePassive(해제=제거)로 분기.
	 * 
	 * 동작이 고정돼 있으므로 일반함수로 두고, 파생클래스에서는 DecideAction을 구현.
	 *
	 * @param Timing       타이밍 태그 (패시브가 발동/해제 분기에 사용)
	 * @param Ctx          소유자/대상 및 스냅샷
	 * @param PassiveState [in,out] 패시브 내부 상태 (드라이버가 받아서 CommitPassive 함수 호출에 사용)
	 */
	void ActivatePassive(
		IN const FGameplayTag& Timing,
		IN const FPassiveActivateContext& Ctx,
		IN OUT TInstancedStruct<FDynamicPassiveData>& PassiveState);

	/**
	 * @brief 계산된 패시브 내부 상태를 커밋
	 *
	 * @details
	 * 상태가 있는 패시브만 override 해서 구현
	 *
	 * @param PassiveState 커밋할 패시브 내부 상태
	 */
	virtual void CommitPassive(IN const TInstancedStruct<FDynamicPassiveData>& PassiveState) {}

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
	 * @brief 패시브가 해당 타이밍태그를 갖고 있는 지 회신 
	 */
	bool HasTimingTag(FGameplayTag Tag) const { return mTimingTags.HasTagExact(Tag); }

	/**
	 * @brief 정적 데이터(UStaticPassiveData) 주입 + 데이터 기반 공통 필드 초기화
	 *
	 * @details
	 * 데이터 구동 패시브는 이펙트/시점/수치를 클래스가 아니라 mStaticData에서 읽는다.
	 * 컴포넌트가 패시브 생성 직후 호출. 내부적으로 InitializeFromData로 공통 필드를 채움.
	 */
	void SetStaticData(UStaticPassiveData* InStaticData);

	// 주입된 정적 데이터 (클래스 구동 패시브는 null)
	UStaticPassiveData* GetStaticData() const { return mStaticData; }

protected:
	/**
	 * @brief 정적 데이터로 공통 필드(이펙트 클래스/타이밍 태그) 초기화
	 *
	 * @details
	 * 기본은 mStaticData를 베이스 멤버로 복사.
	 * 패시브별 추가 파라미터(임계 등)는 각자 DecideAction에서 mStaticData를 직접 읽음.
	 */
	virtual void InitializeFromData();

	/**
	 * @brief 발동/해제/무시 결정 + 다음 상태 계산 + 발동 시 기여값 산출
	 *
	 * @details
	 * 타이밍태그와 내부상태를 보고 패시브의 발동/해제/무시 여부 결정.
	 * 내부상태는 인자로 받은 PassiveState를 사용하고 다음 내부상태도 여기에 써서 돌려줌.
	 *
	 * @param Timing       타이밍태그
	 * @param Ctx          소유자/대상 및 스냅샷
	 * @param PassiveState [in,out] 패시브 내부 상태
	 * @param TargetDelta  [out] 발동 시 대상에게 가할 수치 변화
	 * @return 패시브가 수행할 동작 (발동/해제/무시)
	 */
	virtual EPassiveAction DecideAction(
		IN const FGameplayTag& Timing,
		IN const FPassiveActivateContext& Ctx,
		IN OUT TInstancedStruct<FDynamicPassiveData>& PassiveState,
		OUT FBoardCombatTargetSnapshotData& TargetDelta)
		PURE_VIRTUAL(UTacticalPassive::DecideAction, return EPassiveAction::None;);

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
	 * @brief 패시브가 관여하는 타이밍 태그들
	 *
	 * @details
	 * 시작·끝 등 이 패시브가 반응할 시점들을 모두 담는다(예: {OnStartTurn, OnEndTurn}).
	 * 드라이버가 HasTimingTag 함수로 해당 타이밍태그를 갖고 있는 패시브를 모아서 ActivatePassive를 호출할 때 사용
	 */
	UPROPERTY(Category = "Passive", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "TimingTags"))
	FGameplayTagContainer mTimingTags;

	/**
	 * @brief 커밋된 패시브 내부 상태
	 *
	 * @details
	 * 스택 카운터 등 패시브마다 다른 내부 상태를 FDynamicPassiveData 파생형으로 저장.
	 */
	UPROPERTY(Category = "Passive", VisibleAnywhere, meta = (DisplayName = "State"))
	TInstancedStruct<FDynamicPassiveData> mState;

	/**
	 * @brief 이 패시브의 정적 설정 데이터 (데이터 구동 시 주입)
	 *
	 * @details
	 * 클래스 구동(생성자에서 직접 세팅) 패시브는 null. 데이터 구동 아키타입은 여기서 수치를 읽음.
	 */
	UPROPERTY(Category = "Passive", VisibleAnywhere, meta = (DisplayName = "StaticData"))
	TObjectPtr<UStaticPassiveData> mStaticData = nullptr;
};
