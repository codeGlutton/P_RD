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
class UBoardCombatTargetSnapshotData;
class UTacticalEffect;
class UStaticPassiveData;
enum class EPassiveEffectTarget : uint8;

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
	 * @brief 패시브 타이밍 라우팅 (리셋/캡처/발동/해제)
	 *
	 * @details
	 * 드라이버는 타이밍태그마다 패시브의 처리 내용을 모른 채 이 함수를 호출.
	 *
	 * 진입 시 상태 작업본을 준비(커밋본 복사 또는 InitializeState)한 뒤, 들어온 타이밍이
	 * - 카운터 리셋 시점(mCounterResetTimingTag)이면 OnCounterReset.
	 * - 캡처 시점(mCaptureTimingTag)이면 OnCapture.
	 * - 발동 시점(mActivateTimingTag)이면 EvaluateActivate로 적용 여부를 묻고,
	 *   참이면 NotifyPassive(발동=적용).
	 * - 해제 시점(mDeactivateTimingTag)이면 DeactivatePassive(해제=제거).
	 * 같은 태그가 여러 시점에 걸릴 수 있으므로 위 순서대로 전부 처리.
	 *
	 * 라우팅이 고정돼 있으므로 일반함수로 두고, 파생클래스에서는 시점별 가상 함수를 구현.
	 *
	 * @param TimingTag    타이밍 태그 (발동/해제 시점 분기에 사용)
	 * @param Ctx          소유자/대상 및 스냅샷
	 * @param PassiveState [in,out] 패시브 내부 상태 (드라이버가 받아서 CommitPassive 함수 호출에 사용)
	 */
	void ActivatePassive(
		IN const FGameplayTag& TimingTag,
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
	bool HasTimingTag(FGameplayTag Tag) const
	{
		return Tag == mActivateTimingTag || Tag == mDeactivateTimingTag
			|| Tag == mCounterResetTimingTag || Tag == mCaptureTimingTag;
	}

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
	 * 패시브별 추가 파라미터(임계 등)는 각자 EvaluateActivate에서 mStaticData를 직접 읽음.
	 */
	virtual void InitializeFromData();

	/**
	 * @brief 발동 시점에서 적용 여부 결정 + 다음 상태 계산 + 기여값 산출
	 *
	 * @details
	 * 발동 시점(mActivateTimingTag)에 베이스가 호출. 해제는 베이스가 라우팅하므로 여기선 발동만 판단.
	 * 내부상태는 인자로 받은 PassiveState를 사용하고 다음 내부상태도 여기에 써서 돌려줌.
	 *
	 * @param Ctx          소유자/대상 및 스냅샷
	 * @param PassiveState [in,out] 패시브 내부 상태
	 * @param OutMagnitude [out] 적용 시 대상에게 가할 수치 변화
	 * @return 이펙트를 적용하면 true, 스킵이면 false
	 */
	virtual bool EvaluateActivate(
		IN const FPassiveActivateContext& Ctx,
		IN OUT TInstancedStruct<FDynamicPassiveData>& PassiveState,
		OUT float& OutMagnitude)
		PURE_VIRTUAL(UTacticalPassive::EvaluateActivate, return false;);

	/**
	 * @brief 카운터 리셋 시점 처리
	 *
	 * @details
	 * 상태에 카운터를 가진 패시브가 override. 기본은 no-op.
	 *
	 * @param PassiveState [in,out] 패시브 내부 상태
	 */
	virtual void OnCounterReset(IN OUT TInstancedStruct<FDynamicPassiveData>& PassiveState) {}

	/**
	 * @brief 캡처 시점 처리 (비교용 값 저장)
	 *
	 * @details
	 * 캡처를 쓰는 패시브가 override. 기본은 no-op.
	 *
	 * @param Ctx          소유자/대상 및 스냅샷
	 * @param PassiveState [in,out] 패시브 내부 상태
	 */
	virtual void OnCapture(IN const FPassiveActivateContext& Ctx, IN OUT TInstancedStruct<FDynamicPassiveData>& PassiveState) {}

	/**
	 * @brief 내부 상태 최초 생성
	 *
	 * @details
	 * 상태 보유 패시브가 자기 상태 타입으로 초기화. 기본은 no-op(무상태).
	 * ActivatePassive 진입 시 작업본이 비어 있고 mState도 없을 때 호출됨.
	 *
	 * @param PassiveState [out] 초기화할 내부 상태
	 */
	virtual void InitializeState(OUT TInstancedStruct<FDynamicPassiveData>& PassiveState) const {}

	/**
	 * @brief 계산된 기여값을 드라이버가 원하는 형태로 출력
	 *
	 * @details
	 * 기본은 OUT 모드(TargetDelta를 그대로 둠). 이펙트 발사 등 출력 방식이
	 * 달라지면 베이스에서 이 함수만 교체하면 되고, 서브클래스는 영향 없음.
	 *
	 * @param Ctx         소유자/대상 및 현재 스냅샷
	 * @param Magnitude [in,out] 계산된 수치 변화
	 */
	virtual void NotifyPassive(
		IN const FPassiveActivateContext& Ctx,
		IN float Magnitude);

	/**
	 * @brief 이펙트 적용 (이펙트 클래스/대상 지정)
	 *
	 * @details
	 * EffectTarget이 Self면 소유자에게만, Targets면 Ctx.mTargets 전부에 적용.
	 * 적용한 핸들은 mActiveHandles에 저장.
	 *
	 * @param Ctx          소유자/대상 및 스냅샷
	 * @param EffectClass  적용할 이펙트 클래스
	 * @param Magnitude    적용 수치
	 * @param EffectTarget 적용 대상 (Self / Targets)
	 */
	void NotifyPassive(
		IN const FPassiveActivateContext& Ctx,
		IN TSubclassOf<UTacticalEffect> EffectClass,
		IN float Magnitude,
		IN EPassiveEffectTarget EffectTarget);

	/**
	 * @brief 수량 조건 판정
	 *
	 * @details
	 * mTargets를 순회하며 IsTargetQualified로 자격 타겟 수를 센 뒤,
	 * 수량 조건(Any/All)으로 발동 여부를 결정.
	 * 타겟이 비어 있으면 판정 대상이 없으므로 통과.
	 *
	 * @param Ctx   소유자/대상 및 스냅샷
	 * @param State 패시브 내부 상태 (자격 판정에 전달)
	 * @return 발동 허용이면 true
	 */
	bool PassesTargetQuantifier(IN const FPassiveActivateContext& Ctx, IN const TInstancedStruct<FDynamicPassiveData>& State) const;

	/**
	 * @brief 자격 조건 판정
	 *
	 * @details
	 * 기본은 무조건 통과(true).
	 * '체력 50% 이하' 같은 자격 조건이 있는 패시브가 override해서 스냅샷/상태로 판정.
	 *
	 * @param Ctx         소유자/대상 및 스냅샷
	 * @param TargetIndex Ctx.mTargets 인덱스
	 * @param State       패시브 내부 상태
	 * @return 자격을 갖췄으면 true
	 */
	virtual bool IsTargetQualified(IN const FPassiveActivateContext& Ctx, IN int32 TargetIndex, IN const TInstancedStruct<FDynamicPassiveData>& State) const { return true; }

	/**
	 * @brief 이 패시브가 적용할 이펙트
	 *
	 * @details
	 * - 대상 속성, 연산(op), 지속정책(Infinite/Instant)은 이 이펙트가 정의하고,
	 *   패시브 계산은 크기(magnitude)만 공급한다.
	 * - 모디파이어가 없는 태그형 이펙트는 태그의 값을 크기로 사용
	 */
	UPROPERTY(Category = "Passive", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Effect"))
	TSubclassOf<UTacticalEffect> mEffectClass;

	/**
	 * @brief 적용 중인 이펙트 핸들들 (대상별)
	 *
	 * @details
	 * NotifyPassive 적용 시 대상마다 하나씩 저장하고 DeactivatePassive 제거 시 전부 비움.
	 * 한 배치는 통째로 적용/해제된다는 전제(직렬 apply/deactivate).
	 */
	TArray<FActiveTacticalEffectHandle> mActiveHandles;

	/**
	 * @brief 발동 시점 태그 (단일)
	 *
	 * @details
	 * 이 패시브가 발동(이펙트 적용)할 시점(예: OnStartAttacking).
	 * 드라이버가 HasTimingTag로 모아서 ActivatePassive 호출 시,
	 * 이 태그와 일치하면 EvaluateActivate로 분기.
	 */
	UPROPERTY(Category = "Passive", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "ActivateTiming"))
	FGameplayTag mActivateTimingTag;

	/**
	 * @brief 해제 시점 태그 (단일)
	 *
	 * @details
	 * 이 패시브가 해제(이펙트 제거)할 시점(예: OnEndAttacking)
	 * 이 태그와 일치하면 DeactivatePassive로 분기.
	 */
	UPROPERTY(Category = "Passive", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "DeactivateTiming"))
	FGameplayTag mDeactivateTimingTag;

	/**
	 * @brief 카운터 리셋 시점 태그 (단일)
	 *
	 * @details
	 * 이 태그와 일치하면 OnCounterReset으로 분기. 비어 있으면 리셋 없음.
	 */
	UPROPERTY(Category = "Passive", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "CounterResetTiming"))
	FGameplayTag mCounterResetTimingTag;

	/**
	 * @brief 캡처 시점 태그 (단일)
	 *
	 * @details
	 * 이 태그와 일치하면 OnCapture로 분기. 비어 있으면 캡처 없음.
	 */
	UPROPERTY(Category = "Passive", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "CaptureTiming"))
	FGameplayTag mCaptureTimingTag;

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
