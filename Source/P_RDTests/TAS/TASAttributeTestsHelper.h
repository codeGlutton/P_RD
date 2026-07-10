/*****************************************************************//**
 * @file   TASAttributeTestsHelper.h
 * @brief  TAS Attribute 테스트를 위한 Mock 모델 정의 헤더
 * @author 모호재
 * @date   2026-06-26
 *
 * @details
 *  [PR #191 맥락 - GAS 폐기/연산 enum 치환]
 *  이 헤더는 TAS(Tactical Attribute System) 테스트용 Mock 객체를 정의한다.
 *  본 PR은 이펙트 모디파이어의 "연산 종류"를 표현하던 GAS의 EGameplayModOp를
 *  자체 enum인 ETacticalModOp(TacticalEffectType.h)로 전면 치환하는 작업의 일부다.
 *  - 따라서 아래 Mock 이펙트들의 mModifierOp 필드 타입/값도 EGameplayModOp가 아닌
 *    ETacticalModOp(ETacticalModOp::Additive 등)을 사용하도록 바뀌었다.
 *  - ETacticalModOp의 정수값은 구 EGameplayModOp와 동일하게 유지된다:
 *      (1) 직렬화 호환 - 기존 에셋/세이브에 박힌 정수값을 그대로 보존,
 *      (2) Aggregator가 mMods[ETacticalModOp::Max]처럼 op 값으로 배열을 인덱싱,
 *      (3) DefaultEngine.ini의 CoreRedirect가 구 enum 이름을 새 enum으로 매핑.
 *  - 연산 의미: AddBase(0)=합산, MultiplyAdditive(1)=배율 가산,
 *    DivideAdditive(2)=나눗셈 가산, Override(3)=덮어쓰기,
 *    MultiplyCompound(4)=거듭제곱 곱, AddFinal(5)=최종 합산, Max(6)=무효/개수.
 *    하위호환 별칭(구 GAS 이름): Additive=0 / Multiplicitive=1 / Division=2 / Override=3.
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Actor/ActorModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/TacticalEffect.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "TASAttributeTestsHelper.generated.h"

/**
 * @brief USimulationSubsystem의 protected 멤버 접근용 우회 클래스
 *
 * @details
 *  테스트에서 USimulationSubsystem의 protected 멤버(mCurrentRoomContext 등)나
 *  protected 메서드(SetSimulationState)에 접근하기 위한 friend 대용 어댑터다.
 *  파생 클래스로 캐스팅하면 부모의 protected 멤버에 접근할 수 있는 C++ 관용구를 이용한다.
 *  실제 인스턴스를 새로 만들지 않고, 기존 Subsystem 포인터를 static_cast로 재해석한다.
 */
class FSimulationSubsystemTestAccessor : public USimulationSubsystem
{
public:
    /**
     * @brief Subsystem이 보유한 현재 방 컨텍스트의 RoomInstance를 꺼낸다.
     * @param Subsystem 대상 시뮬레이션 서브시스템(널 허용)
     * @return 현재 컨텍스트의 mRoomInstance, Subsystem이 널이면 nullptr
     */
    static URoomInstance* GetRoomInstance(USimulationSubsystem* Subsystem)
    {
        if (Subsystem == nullptr) 
        {
            return nullptr;
        }

        // 파생 타입으로 재해석하여 부모의 protected 멤버(mCurrentRoomContext)에 접근
        const FSimulationSubsystemTestAccessor* Accessor = static_cast<const FSimulationSubsystemTestAccessor*>(Subsystem);
        return Accessor->mCurrentRoomContext->mRoomInstance;
    }

    /**
     * @brief Subsystem의 시뮬레이션 상태를 강제로 설정한다(테스트 세팅용).
     * @param Subsystem 대상 시뮬레이션 서브시스템(널 허용)
     * @param State 설정할 시뮬레이션 상태
     */
    static void SetSimState(USimulationSubsystem* Subsystem, ESRPGSimulationState State)
    {
        if (Subsystem == nullptr) 
        {
            return;
        }

        // 파생 타입으로 재해석하여 부모의 protected 메서드(SetSimulationState) 호출
        FSimulationSubsystemTestAccessor* Accessor = static_cast<FSimulationSubsystemTestAccessor*>(Subsystem);
        Accessor->SetSimulationState(State);
    }
};

/**
 * @brief Attribute 테스트용 ActorModel Mock
 *
 * @details
 *  UAttributeSetComponentModel과 UUnitAttributeSet을 기본 서브오브젝트로 묶어,
 *  테스트가 어트리뷰트/이펙트 적용 대상으로 쓸 수 있는 최소 액터 모델을 제공한다.
 *  초기화 훅(Pre/PostInitializeComponentModels)은 의도적으로 빈 구현으로 둔다.
 */
UCLASS()
class UTASActorModelMock : public UActorModel
{
	GENERATED_BODY()

public:
	UTASActorModelMock()
	{
		// 어트리뷰트 컴포넌트 모델과 유닛 어트리뷰트 세트를 기본 서브오브젝트로 생성
		mAttributeCompModel = CreateDefaultSubobject<UAttributeSetComponentModel>(TEXT("AttributeSetComponentModel"));
		mUnitAttributeSet = CreateDefaultSubobject<UUnitAttributeSet>(TEXT("UnitAttributeSet"));
	}

public:
	// 테스트 Mock이므로 컴포넌트 모델 초기화 단계는 비워 둔다(실제 배선 불필요).
	virtual void PreInitializeComponentModels() override {}
	virtual void PostInitializeComponentModels() override {}

public:
	/**
	 * @brief 보유 중인 어트리뷰트 컴포넌트 모델을 반환한다.
	 * @return mAttributeCompModel 포인터
	 */
	UAttributeSetComponentModel* GetAttributeComponent() const
	{
		return mAttributeCompModel;
	}

private:
	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "UnitAttributeSet"))
	TObjectPtr<UUnitAttributeSet> mUnitAttributeSet;

	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AttributeCompModel"))
	TObjectPtr<UAttributeSetComponentModel> mAttributeCompModel;
};

/**
 * @brief 즉발(Instant) 이펙트 테스트용 Mock
 *
 * @details
 *  MaxHP를 +1 즉시 합산하는 1회성 이펙트.
 *  Instant 정책은 베이스값에 즉시 반영되며 스택을 쌓지 않는다(None).
 */
UCLASS()
class UTestInstantTacticalEffect : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTestInstantTacticalEffect()
	{
		mDurationPolicy = ETacticalEffectDurationType::Instant;
		mStackingType = ETacticalEffectStackingType::None;

		FTacticalModifierInfo Info;
		Info.mAttribute = UUnitAttributeSet::GetMaxHPAttribute();
		// [PR #191] 구 EGameplayModOp::Additive -> ETacticalModOp::Additive 치환.
		// Additive(=AddBase, 정수값 0)는 합산 연산. 정수값은 구 enum과 동일하게 유지된다.
		Info.mModifierOp = ETacticalModOp::Additive;
		Info.mModifierMagnitude = 1.f;

		mModifiers.Add(Info);
	}
};

/**
 * @brief 무한(Infinite) 지속 이펙트 테스트용 Mock
 *
 * @details
 *  DefensePoint를 +1 합산하는 영구 지속 이펙트.
 *  Infinite 정책은 제거 전까지 유지되며, 베이스가 아닌 현재값(Current)에 합산된다.
 */
UCLASS()
class UTestInfiniteTacticalEffect : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTestInfiniteTacticalEffect()
	{
		mDurationPolicy = ETacticalEffectDurationType::Infinite;
		mStackingType = ETacticalEffectStackingType::None;

		FTacticalModifierInfo Info;
		Info.mAttribute = UUnitAttributeSet::GetDefensePointAttribute();
		// [PR #191] 구 EGameplayModOp::Additive -> ETacticalModOp::Additive 치환(합산, 정수값 0).
		Info.mModifierOp = ETacticalModOp::Additive;
		Info.mModifierMagnitude = 1.f;

		mModifiers.Add(Info);
	}
};

/**
 * @brief 무한 지속 + 태그 부여 이펙트 테스트용 Mock
 *
 * @details
 *  어트리뷰트 모디파이어 없이 GameplayTag만 부여하는 영구 이펙트.
 *  이펙트가 살아 있는 동안 대상에게 OnEndAttacking 패시브 태그를 부여하는지를 검증한다.
 */
UCLASS()
class UTestInfiniteTagEffect : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTestInfiniteTagEffect()
	{
		mDurationPolicy = ETacticalEffectDurationType::Infinite;
		mStackingType = ETacticalEffectStackingType::None;

		// 모디파이어가 아닌 부여 태그만 설정: 이펙트 적용 시 대상에 붙는 태그 검증용
		mCachedGrantedTags.AddTag(AbilityTags::GameplayAbility_Passive_OnEndApplyingEffect);
	}
};