/*****************************************************************//**
 * @file   ActiveTacticalEffect.h
 * @brief  현재 활성화 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-06-23
 *********************************************************************/

#pragma once

// [마이그레이션 맥락 / PR #191]
//   이 헤더가 다루는 "활성 이펙트(런타임에 부착된 이펙트)"는 GAS(GameplayAbilitySystem) 폐기 작업의 일부로
//   재작성되었다. 핵심 변경점은 모디파이어의 "연산 종류"를 표현하는 enum이 GAS의 EGameplayModOp에서
//   자체 정의 ETacticalModOp(TacticalEffectType.h)로 치환된 것이다.
//   - 본 파일에서는 FTacticalModifierEvaluatedData::mModifierOp 가 그 enum을 들고 있는 지점이다.
//   - ETacticalModOp 의 "정수값"은 구 EGameplayModOp 와 동일하게 유지된다. 그래야
//       (1) 직렬화 호환: 기존 에셋/세이브에 이미 박혀 있는 정수값을 그대로 읽어도 같은 연산을 가리킨다.
//       (2) 배열 인덱싱: Aggregator 가 mMods[ETacticalModOp::Max] 형태로 op 값을 배열 인덱스로 쓰므로
//           값이 0..Max 로 연속·고정되어 있어야 한다.
//       (3) CoreRedirect: DefaultEngine.ini 의 CoreRedirect 가 구 enum "이름"을 새 enum 이름으로
//           매핑하는데, 값까지 동일해야 리다이렉트 후에도 의미가 보존된다.
#include "AttributeSet/AttributeSetMinimal.h"
// TacticalEffect.h 를 통해 ETacticalModOp / TacticalEffectUtilities(구 GameplayEffectUtilities 대체) 가 가시화된다.
#include "TAS/Effect/TacticalEffect.h"
#include "ActiveTacticalEffect.generated.h"

class UAttributeSetComponentModel;
struct FTacticalEffectRemovalInfo;

/**
 * @brief 활성화된 이펙트를 쉽게 탐색하기 위한 핸들
 *
 * @details
 *  - GAS 의 FActiveGameplayEffectHandle 에 대응하는 자체 구현. 활성 이펙트 본체(FActiveTacticalEffect)를
 *    값으로 들고 다니는 대신, 가벼운 정수 인덱스(mIndex) + 소속 월드(mWorld)로 간접 참조한다.
 *  - mIndex 가 전역 핸들 맵의 키이며, 이 맵을 통해 핸들 -> 소유 AttributeSetComponentModel 로 역추적한다.
 *  - 동등성/해시는 모두 mIndex 만으로 결정되므로, 같은 인덱스를 가리키는 핸들은 동일 이펙트를 가리킨다.
 */
USTRUCT()
struct FActiveTacticalEffectHandle
{
    GENERATED_BODY()

public:
    FActiveTacticalEffectHandle() = default;

    /**
     * @brief 인덱스/월드로부터 핸들을 직접 구성한다.
     * @param Index 전역 핸들 맵에서의 인덱스(키). INDEX_NONE 이면 무효 핸들.
     * @param World 이 핸들이 속한 월드(전역 맵은 월드 단위로 관리됨).
     */
    FActiveTacticalEffectHandle(int32 Index, UWorld* World);

public:
    /**
     * @brief 새 활성 이펙트용 핸들을 발급하고 전역 맵에 등록한다.
     * @param World 핸들이 속할 월드.
     * @param OwningModel 이 이펙트를 소유한 AttributeSetComponentModel(역추적 대상).
     * @return 새로 발급된 유효 핸들.
     */
    static FActiveTacticalEffectHandle GenerateNewHandle(UWorld* World, UAttributeSetComponentModel* OwningModel);

    /**
     * @brief 해당 월드의 전역 핸들 맵을 초기화한다(월드 전환/정리 시 사용).
     * @param World 초기화할 대상 월드.
     */
    static void ResetGlobalHandleMap(UWorld* World);

public:
    /**
     * @brief 이 핸들이 가리키는 이펙트를 소유한 AttributeSetComponentModel 을 전역 맵에서 역추적한다.
     * @return 소유 모델 포인터. 핸들이 무효하거나 이미 제거되었으면 nullptr.
     */
    UAttributeSetComponentModel* GetOwningAttributeSetComponentModel() const;

    /** @brief 이 핸들 엔트리를 전역 맵에서 제거한다(이펙트 소멸 시 호출). */
    void RemoveFromGlobalMap();

public:
    /** @brief 동등 비교(인덱스 기준). */
    bool operator==(const FActiveTacticalEffectHandle& Other) const
    {
        // 핸들의 정체성은 전역 맵 인덱스로만 결정한다(mWorld 는 비교에 미포함).
        return mIndex == Other.mIndex;
    }

    /** @brief 비동등 비교(인덱스 기준). */
    bool operator!=(const FActiveTacticalEffectHandle& Other) const
    {
        return mIndex != Other.mIndex;
    }

    /**
     * @brief TMap/TSet 키로 사용하기 위한 해시.
     * @param Other 해시 대상 핸들.
     * @return 인덱스를 그대로 사용한 해시값.
     */
    friend uint32 GetTypeHash(const FActiveTacticalEffectHandle& Other)
    {
        // 인덱스 자체가 고유 키이므로 추가 믹싱 없이 그대로 해시로 사용.
        return Other.mIndex;
    }

    /** @brief 유효 핸들 여부(인덱스가 INDEX_NONE 이 아니면 유효). */
    bool IsValid() const
    {
        return mIndex != INDEX_NONE;
    }

    /** @brief 핸들을 무효 상태(INDEX_NONE)로 되돌린다. */
    void Reset()
    {
        mIndex = INDEX_NONE;
    }

private:
    // @brief 전역 핸들 맵에서의 인덱스(키). 핸들 정체성의 유일한 근거. INDEX_NONE = 무효.
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "Index"))
    int32 mIndex = INDEX_NONE;

    // @brief 핸들이 속한 월드(전역 맵이 월드 단위라 약참조로 보관). 비교/해시에는 미포함.
    UPROPERTY(Category = "World", VisibleAnywhere, meta = (DisplayName = "World"))
    TWeakObjectPtr<UWorld> mWorld;
};

/**
 * @brief 평가가 끝난(=실제 적용 직전 상태의) 단일 모디파이어 데이터
 */
USTRUCT(BlueprintType)
struct FTacticalModifierEvaluatedData
{
    GENERATED_USTRUCT_BODY()

    FTacticalModifierEvaluatedData() :
        mAttribute(),
        mModifierOp(ETacticalModOp::AddBase),
        mMagnitude(0.f),
        mIsValid(false)
    {
    }

    /**
     * @brief 평가 결과 한 건을 채워 유효(mIsValid=true) 상태로 생성한다.
     * @param InAttribute 적용 대상 Attribute.
     * @param InModOp 적용 연산. ETacticalModOp(구 EGameplayModOp 치환)을 바이트 enum 으로 받는다.
     * @param InMagnitude 적용 크기(연산에 따라 가산값/배율/덮어쓸 값 등으로 해석됨).
     * @param InHandle 이 모디파이어를 낳은 활성 이펙트 핸들(출처 추적/제거용). 기본은 무효 핸들.
     */
    FTacticalModifierEvaluatedData(const FTacticalAttribute& InAttribute, TEnumAsByte<ETacticalModOp::Type> InModOp, float InMagnitude, FActiveTacticalEffectHandle InHandle = FActiveTacticalEffectHandle()) :
        mAttribute(InAttribute),
        mModifierOp(InModOp),
        mMagnitude(InMagnitude),
        mHandle(InHandle),
        mIsValid(true)
    {
    }

    /**
     * @brief 디버그용 한 줄 요약 문자열을 만든다.
     * @return "<Attribute> <Op> EvalMag: <Magnitude>" 형식.
     * @note 연산 이름 문자열화는 TacticalEffectUtilities::TacticalModOpToString 사용
     *       (구 GameplayEffectUtilities 의 동명 유틸 대체).
     */
    FString ToSimpleString() const
    {
        return FString::Printf(TEXT("%s %s EvalMag: %f"), *mAttribute.GetName(), *TacticalEffectUtilities::TacticalModOpToString(mModifierOp), mMagnitude);
    }

    // @brief 적용 대상 Attribute.
    UPROPERTY()
    FTacticalAttribute mAttribute;

    // @brief 적용 연산 종류. [PR #191] 구 EGameplayModOp 를 ETacticalModOp 로 치환한 지점.
    //        TEnumAsByte 로 바이트 직렬화하며, 정수값이 구 enum 과 동일해 기존 에셋과 호환된다.
    UPROPERTY()
    TEnumAsByte<ETacticalModOp::Type> mModifierOp;

    // @brief 적용 크기. 연산별 해석이 다름(합산=더할 값 / 배율=1+값 / Override=대체값 등).
    UPROPERTY()
    float mMagnitude;

    // @brief 이 모디파이어의 출처 활성 이펙트 핸들(제거/역추적용).
    UPROPERTY()
    FActiveTacticalEffectHandle	mHandle;

    // @brief 이 평가 데이터의 유효 여부(기본 생성 시 false).
    UPROPERTY()
    bool mIsValid;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnActiveTacticalEffectRemoved_Info, const FTacticalEffectRemovalInfo&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnActiveTacticalEffectStackChange, FActiveTacticalEffectHandle, int32 /*NewStackCount*/, int32 /*PreviousStackCount*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnActiveTacticalEffectTimeChange, FActiveTacticalEffectHandle, int32 /*NewStartTime*/, int32 /*NewDuration*/);

/**
 * @brief 활성 이펙트 하나가 발생시키는 이벤트(대리자) 묶음.
 * @details 이펙트 본체(FActiveTacticalEffect)에 값으로 포함되어, 제거/스택변경 시점을 외부에 통지한다.
 */
struct FActiveTacticalEffectEvents
{
    // @brief 이펙트 제거 시 브로드캐스트.
    FOnActiveTacticalEffectRemoved_Info OnEffectRemoved;
    // @brief 이펙트 스택 수 변경 시 브로드캐스트.
    FOnActiveTacticalEffectStackChange OnStackChanged;
    // @brief Duration 종료 Time 변경 시 브로드캐스트.
    FOnActiveTacticalEffectTimeChange OnTimeChanged;
};

/**
 * @brief 현재 활성화 이펙트(런타임에 어떤 대상에 부착되어 동작 중인 이펙트 1건).
 *
 * @details
 *  GAS 의 FActiveGameplayEffect 에 대응한다. 핸들(mHandle)로 식별되고, 런타임 구성/스택/시간 정보는
 *  mSpec(FTacticalEffectSpec)에 담긴다. 활성 이펙트들은 mPendingNext 로 단방향 연결되어
 *  추가 예약 큐를 이루며, 제거는 mIsPendingRemove 플래그로 지연 처리(순회 중 안전 삭제)된다.
 */
USTRUCT()
struct FActiveTacticalEffect
{
    GENERATED_BODY()

public:
    FActiveTacticalEffect() = default;

    /**
     * @brief 핸들과 런타임 스펙으로 활성 이펙트를 구성한다.
     * @param Handle 이 이펙트를 식별하는 발급된 핸들.
     * @param Spec 이펙트 런타임 구성(모디파이어/스택/지속시간 등).
     */
    FActiveTacticalEffect(FActiveTacticalEffectHandle Handle, const FTacticalEffectSpec& Spec);

public:
    /**
     * @brief 동등 비교(핸들 기준). 같은 핸들이면 같은 활성 이펙트로 간주.
     * @param Other 비교 대상.
     * @return 핸들이 같으면 true.
     */
    bool operator==(const FActiveTacticalEffect& Other)
    {
        return mHandle == Other.mHandle;
    }

public:
    int32 GetTimeRemaining(int32 WorldTime) const;
    int32 GetDuration() const;
    ETacticalEffectDurationUnitType GetDurationUnit() const;
    int32 GetEndTime() const;

public:
    // @brief 이펙트 ID
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "Handle"))
    FActiveTacticalEffectHandle mHandle;

    // @brief 이펙트 런타임 구성 데이터
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "Spec"))
    FTacticalEffectSpec mSpec;

    // @brief 이펙트 시작 타이밍
    UPROPERTY(Category = "Effect", VisibleAnywhere, meta = (DisplayName = "StartTime"))
    int32 mStartTime = 0;

public:
    // @brief 추가 예약된 다음 이펙트 포인터
    FActiveTacticalEffect* mPendingNext = nullptr;
    // @brief 삭제 예약된 이펙트 여부
    bool mIsPendingRemove = false;

public:
    // @brief 각 상황에 맞는 대리자
    FActiveTacticalEffectEvents mEventSet;
};

