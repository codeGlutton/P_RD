#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Aggregator/TacticalAggregator.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

/**
 * @file AttributeSetComponentModel.cpp
 * @brief 유닛(ActorModel)에 부착되어 속성(Attribute)과 전술 이펙트(TacticalEffect)를
 *        총괄하는 컴포넌트 모델 구현부. GAS의 UAbilitySystemComponent에 대응하는 자체 구현이다.
 *
 *        [PR #191 마이그레이션 맥락]
 *        본 PR은 효과 모디파이어의 "연산 종류" enum을 GAS의 EGameplayModOp에서
 *        자체 enum인 ETacticalModOp(TacticalEffectType.h)로 치환한다(GAS 폐기 작업의 일부).
 *        본 파일에서 직접 영향을 받는 지점은 ApplyModToAttribute()의 시그니처로,
 *        매개변수 타입이 TEnumAsByte<EGameplayModOp::Type> → TEnumAsByte<ETacticalModOp::Type>로
 *        교체되었다.
 *
 *        [ETacticalModOp 정수값을 구 EGameplayModOp와 동일하게 유지하는 이유 3가지]
 *        (1) 직렬화 호환: 기존 에셋/세이브에 정수로 박힌 op 값을 그대로 보존한다.
 *        (2) 배열 인덱싱 호환: Aggregator가 mMods[ETacticalModOp::Max]처럼 op 정수값으로
 *            모디파이어 배열을 인덱싱하므로 값이 바뀌면 인덱스가 깨진다.
 *        (3) CoreRedirect 호환: DefaultEngine.ini의 CoreRedirect가 구 enum 이름을
 *            새 enum 이름으로 매핑하므로, 값까지 같아야 무손실 치환이 된다.
 *
 *        [ETacticalModOp 연산 종류와 수식]
 *        - AddBase(0)          : 합산. 기본값에 더한다.        (별칭 Additive=0)
 *        - MultiplyAdditive(1) : 배율 가산. 곱셈 계수를 누적 합산한다. (별칭 Multiplicitive=1)
 *        - DivideAdditive(2)   : 나눗셈 가산. 나눗셈 계수를 누적 합산한다. (별칭 Division=2)
 *        - Override(3)         : 덮어쓰기. 이전 값을 무시하고 교체한다.   (별칭 Override=3)
 *        - MultiplyCompound(4) : 거듭제곱 곱. 스택 수만큼 거듭제곱으로 곱한다.
 *        - AddFinal(5)         : 최종 합산. 모든 계산 후 마지막에 더한다.
 *        - Max(6)              : 무효/연산 개수(배열 크기 인덱스로 사용).
 *        ※ 항등값/스택 누적 수식은 TacticalEffectUtilities(TacticalEffectType.cpp)의
 *          GetModifierBiasByModifierOp(곱셈계열=1, 합산계열=0)와
 *          ComputeStackedModifierMagnitude(MultiplyCompound는 거듭제곱)에서 처리한다.
 */

/**
 * @brief 컴포넌트 초기화. 소유 ActorModel 하위에 이미 존재하는 AttributeSet들을 수집하고
 *        이펙트 컨테이너/태그 카운트 컨테이너의 소유자를 본 컴포넌트로 등록한다.
 */
void UAttributeSetComponentModel::Initialize()
{
	Super::Initialize();

    // 뒤에서부터 순회하며 null(소멸/직렬화 누락) 엔트리를 제거 — 역순이라 RemoveAt 인덱스가 안정적
    for (int32 Idx = mSpawnedAttributes.Num() - 1; Idx >= 0; --Idx)
    {
        if (mSpawnedAttributes[Idx] == nullptr)
        {
            mSpawnedAttributes.RemoveAt(Idx);
        }
    }

    // 소유 ActorModel을 Outer로 갖는 자식 UObject 중 AttributeSet을 모두 수집한다(Garbage 제외)
    TArray<UObject*> ChildObjects;
    GetObjectsWithOuter(GetOwnerModel(), ChildObjects, false, RF_NoFlags, EInternalObjectFlags::Garbage);

    for (UObject* Obj : ChildObjects)
    {
        UTacticalAttributeSet* Set = Cast<UTacticalAttributeSet>(Obj);
        if (Set)
        {
            mSpawnedAttributes.AddUnique(Set); // 중복 방지하며 등록
        }
    }

    mActiveAttributeEffects.RegisterWithOwnerModel(this); // 활성 이펙트 컨테이너에 소유자 연결
    mTacticalTagCountContainer.SetOwner(this);            // 태그 카운트 컨테이너에 소유자 연결
}

/**
 * @brief 컴포넌트 해제.
 */
void UAttributeSetComponentModel::Uninitialize()
{
    Super::Uninitialize();
}

/**
 * @brief 복제(Duplicate) 직후 후처리. 활성 이펙트 컨테이너에 복제 사실을 전파해
 *        내부 핸들/포인터를 새 인스턴스 기준으로 재정비하도록 한다.
 * @param DuplicateForPIE PIE(에디터 플레이)용 복제 여부.
 */
void UAttributeSetComponentModel::PostDuplicate(bool DuplicateForPIE)
{
	Super::PostDuplicate(DuplicateForPIE);

	mActiveAttributeEffects.PostDuplicate(DuplicateForPIE);
}

/**
 * @brief 플레이 시작 콜백.
 */
void UAttributeSetComponentModel::BeginPlay()
{
	Super::BeginPlay();
}

/**
 * @brief 플레이 종료 콜백.
 */
void UAttributeSetComponentModel::EndPlay()
{
    Super::EndPlay();
}

/**
 * @brief 현재 스폰되어 있는 모든 AttributeSet 목록을 반환한다.
 * @return AttributeSet 포인터 배열(읽기 전용 참조).
 */
const TArray<UTacticalAttributeSet*>& UAttributeSetComponentModel::GetSpawnedAttributes() const
{
    return mSpawnedAttributes;
}

/**
 * @brief 지정 클래스에 해당하는 AttributeSet을 스폰 목록에서 찾아 반환한다.
 * @param Class 찾을 AttributeSet 클래스.
 * @return 일치하는 AttributeSet, 없으면 nullptr.
 */
const UTacticalAttributeSet* UAttributeSetComponentModel::GetAttributeSet_Internal(TSubclassOf<UTacticalAttributeSet> Class) const
{
    for (const UTacticalAttributeSet* Set : mSpawnedAttributes)
    {
        if (Set != nullptr && Set->IsA(Class) == true) // 정확 일치 + 파생 클래스까지 매칭
        {
            return Set;
        }
    }
    return nullptr;
}

/**
 * @brief 주어진 속성을 보유한 AttributeSet이 존재하는지 검사한다.
 * @param Attribute 검사할 전술 속성.
 * @return 해당 속성의 AttributeSet이 있으면 true.
 */
bool UAttributeSetComponentModel::HasAttributeSetForAttribute(FTacticalAttribute Attribute) const
{
    return (Attribute.IsValid() == true && (GetAttributeSet_Internal(Attribute.GetAttributeSetClass()) != nullptr));
}

/**
 * @brief AttributeSet을 스폰 목록에 등록한다(중복 방지).
 * @param AttributeSet 등록할 AttributeSet.
 */
void UAttributeSetComponentModel::AddSpawnedAttributeSet(UTacticalAttributeSet* AttributeSet)
{
    if (IsValid(AttributeSet) == false)
    {
        return;
    }
    mSpawnedAttributes.AddUnique(AttributeSet);
}

/**
 * @brief AttributeSet을 스폰 목록에서 제거하고, 그 클래스가 보유하던 모든 속성의
 *        Aggregator(계산 객체)도 함께 정리한다.
 * @param AttributeSet 제거할 AttributeSet.
 */
void UAttributeSetComponentModel::RemoveSpawnedAttributeSet(UTacticalAttributeSet* AttributeSet)
{
    if (mSpawnedAttributes.RemoveSingle(AttributeSet) == 1) // 실제로 1개 제거된 경우에만 후처리
    {
        /* 모든 클래스 속성 가져오기 */

        // 제거되는 Set 클래스가 정의한 모든 속성을 열거해 각각의 Aggregator를 정리한다
        TArray<FTacticalAttribute> Attributes;
        UTacticalAttributeSet::GetAttributesFromSetClass(AttributeSet->GetClass(), Attributes);
        for (const FTacticalAttribute& Attribute : Attributes)
        {
            UE_LOG(LogAttributeSetComp, Log, TEXT("계산 객체 Aggregator에서 해당 속성 값 [%s] 제거"), *Attribute.GetName());
            mActiveAttributeEffects.CleanupAttributeAggregator(Attribute); // 잔존 모디파이어 계산 객체 해제
        }
    }
}

/**
 * @brief 지정 클래스의 AttributeSet을 찾아 반환하되, 없으면 새로 생성해 등록한 뒤 반환한다.
 * @param Class 조회/생성할 AttributeSet 클래스.
 * @return 기존 또는 신규 생성된 AttributeSet, 소유 모델이 없거나 Class가 null이면 nullptr.
 */
const UTacticalAttributeSet* UAttributeSetComponentModel::GetOrCreateAttributeSet_Internal(TSubclassOf<UTacticalAttributeSet> Class)
{
    UActorModel* OwningActorModel = GetOwnerModel();
    const UTacticalAttributeSet* OwnedAttributes = nullptr;
    if (OwningActorModel != nullptr && Class != nullptr)
    {
        /* 기존 속성 객체 찾아보기 */

        OwnedAttributes = GetAttributeSet_Internal(Class);
        if (OwnedAttributes == nullptr)
        {
            /* 발견 못해서 생성 */

            UTacticalAttributeSet* Attributes = NewObject<UTacticalAttributeSet>(OwningActorModel, Class);
            AddSpawnedAttributeSet(Attributes);
            OwnedAttributes = Attributes;
        }
    }

    return OwnedAttributes;
}

/**
 * @brief 현재 컴포넌트의 모든 속성값과 태그 카운트를 스냅샷에 직렬화한다(세이브/롤백용).
 * @param Snapshot 캡처 결과를 담을 스냅샷 구조체.
 */
void UAttributeSetComponentModel::CaptureAllStates(UBoardCombatTargetSnapshotData* Snapshot) const
{
    // 각 AttributeSet의 속성값 캡처
    for (const TObjectPtr<UTacticalAttributeSet>& SpawnedAttribute : mSpawnedAttributes)
    {
        SpawnedAttribute->CaptureAllAttributes(Snapshot); 
    }
    // 태그 카운트 캡처
    mTacticalTagCountContainer.CaptureAllTags(Snapshot);

    // 타일 위치 캡처
    UBoardActorModel* OwnerActorModel = GetOwnerModel<UBoardActorModel>();
    if (OwnerActorModel != nullptr)
    {
        Snapshot->mTileTransform = OwnerActorModel->GetTileTransform();
    }
}

/**
 * @brief 속성의 기본값(BaseValue)을 설정한다. 모디파이어 적용 전의 원본 값이다.
 * @param Attribute 대상 속성.
 * @param BaseValue 설정할 기본값.
 */
void UAttributeSetComponentModel::SetAttributeBaseValue(const FTacticalAttribute& Attribute, float BaseValue)
{
    mActiveAttributeEffects.SetAttributeBaseValue(Attribute, BaseValue);
}

/**
 * @brief 속성의 기본값(BaseValue)을 반환한다.
 * @param Attribute 대상 속성.
 * @return 모디파이어 적용 전 기본값.
 */
float UAttributeSetComponentModel::GetAttributeBaseValue(const FTacticalAttribute& Attribute) const
{
    return mActiveAttributeEffects.GetAttributeBaseValue(Attribute);
}

/**
 * @brief 속성의 현재값(모디파이어가 모두 적용된 최종값)을 반환한다(성공 여부 출력 포함).
 * @param Attribute 대상 속성.
 * @param Found [out] 해당 속성의 AttributeSet을 찾았는지 여부.
 * @return 속성의 현재값. 미발견 시 0.
 */
float UAttributeSetComponentModel::GetAttributeCurrentValue(FTacticalAttribute Attribute, bool& Found) const
{
    if (Attribute.IsValid() == true)
    {
        const UTacticalAttributeSet* FoundAttributeSet = GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
        if (FoundAttributeSet != nullptr)
        {
            Found = true;
            return Attribute.GetNumericValue(FoundAttributeSet);
        }
    }

    Found = false;
    return 0.0f;
}

/**
 * @brief 속성의 현재값(최종값)을 반환한다.
 * @param Attribute 대상 속성.
 * @return 속성의 현재값. AttributeSet 미발견 시 0.
 */
float UAttributeSetComponentModel::GetAttributeCurrentValue(const FTacticalAttribute& Attribute) const
{
    const UTacticalAttributeSet* FoundAttributeSet = GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
    if (FoundAttributeSet == nullptr)
    {
        return 0.f;
    }
    return Attribute.GetNumericValue(FoundAttributeSet);
}

/**
 * @brief 속성의 현재값을 직접 덮어쓴다(내부용). 대상 AttributeSet이 반드시 존재해야 한다.
 * @param Attribute 대상 속성.
 * @param NewValue [in,out] 설정할 값. clamp 등으로 보정될 수 있어 참조로 전달된다.
 */
void UAttributeSetComponentModel::SetAttributeCurrentValue_Internal(const FTacticalAttribute& Attribute, float& NewValue)
{
    const UTacticalAttributeSet* AttributeSet = GetAttributeSet_Internal(Attribute.GetAttributeSetClass());
    checkf(AttributeSet != nullptr, TEXT("변경하려는 AttributeSet이 nullptr"));

    // const 컨테이너에서 얻은 포인터지만 값 변경이 필요하므로 const_cast로 쓰기 허용
    Attribute.SetNumericValueChecked(NewValue, const_cast<UTacticalAttributeSet*>(AttributeSet));
}

/**
 * @brief 특정 속성의 값 변경 통지 델리게이트를 반환한다(구독용).
 * @param Attribute 구독할 속성.
 * @return 값 변경 시 브로드캐스트되는 델리게이트 참조.
 */
FOnChangeAttributeValue& UAttributeSetComponentModel::GetTacticalAttributeValueChangeDelegate(FTacticalAttribute Attribute)
{
    return mActiveAttributeEffects.GetTacticalAttributeValueChangeDelegate(Attribute);
}

/**
 * @brief 이펙트가 "대상"에게 적용되었을 때 호출되어 외부 구독자에게 통지한다.
 *        (시전자 관점에서 자신이 가한 이펙트를 알리는 경로)
 * @param Model 시전자 측 AttributeSetComponentModel.
 * @param SpecApplied 적용된 이펙트 스펙.
 * @param ActiveHandle 적용된 활성 이펙트 핸들.
 */
void UAttributeSetComponentModel::OnTacticalEffectAppliedToTarget(UAttributeSetComponentModel* Model, const FTacticalEffectSpec& SpecApplied, FActiveTacticalEffectHandle ActiveHandle)
{
    OnTacticalEffectAppliedDelegateToTarget.Broadcast(Model, SpecApplied, ActiveHandle);
}

/**
 * @brief 이펙트가 "자신"에게 적용되었을 때 호출되어 외부 구독자에게 통지한다.
 * @param Model 시전자 측 AttributeSetComponentModel.
 * @param SpecApplied 적용된 이펙트 스펙.
 * @param ActiveHandle 적용된 활성 이펙트 핸들.
 */
void UAttributeSetComponentModel::OnTacticalEffectAppliedToSelf(UAttributeSetComponentModel* Model, const FTacticalEffectSpec& SpecApplied, FActiveTacticalEffectHandle ActiveHandle)
{
    OnTacticalEffectAppliedDelegateToSelf.Broadcast(Model, SpecApplied, ActiveHandle);
}

/**
 * @brief 속성에 단일 모디파이어를 즉시 적용한다(스펙 경유 없이 직접 연산).
 *
 *        [PR #191 enum 치환 지점]
 *        ModifierOp 타입이 구 TEnumAsByte<EGameplayModOp::Type>에서
 *        자체 enum TEnumAsByte<ETacticalModOp::Type>로 교체되었다(GAS 폐기).
 *        ETacticalModOp의 정수값은 구 EGameplayModOp와 동일하게 유지되므로
 *        (직렬화/Aggregator 인덱싱/CoreRedirect 호환) 기존 에셋의 op 값이 그대로 동작한다.
 *
 *        실제 연산(AddBase=합산, MultiplyAdditive=배율 가산, DivideAdditive=나눗셈 가산,
 *        Override=덮어쓰기, MultiplyCompound=거듭제곱 곱, AddFinal=최종 합산)은
 *        Aggregator가 op 값으로 모디파이어 배열을 인덱싱하며 수행한다.
 *
 * @param Attribute 모디파이어를 적용할 대상 속성.
 * @param ModifierOp 적용할 연산 종류(ETacticalModOp).
 * @param ModifierMagnitude 모디파이어 크기(연산 종류에 따라 가산값/배율/제수 등으로 해석).
 */
void UAttributeSetComponentModel::ApplyModToAttribute(const FTacticalAttribute& Attribute, TEnumAsByte<ETacticalModOp::Type> ModifierOp, float ModifierMagnitude)
{
    mActiveAttributeEffects.ApplyModToAttribute(Attribute, ModifierOp, ModifierMagnitude);
}

/**
 * @brief 이펙트 적용에 쓸 컨텍스트를 생성하고 시전자(Instigator)를 본 컴포넌트의 소유 모델로 설정한다.
 * @return 시전자가 채워진 새 TacticalEffectContext.
 */
UTacticalEffectContext* UAttributeSetComponentModel::MakeEffectContext() const
{
    UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
    check(TacticalFrameworkModel != nullptr);
    UTacticalEffectContext* Context = TacticalFrameworkModel->AllocTacticalEffectContext();

    Context->SetInstigator(GetOwnerModel());

    return Context;
}

/**
 * @brief 지정 이펙트 클래스로부터 적용용 이펙트 스펙을 생성한다.
 * @param EffectClass 스펙으로 만들 이펙트 클래스.
 * @param Context 사용할 컨텍스트. nullptr이면 내부에서 새로 생성한다.
 * @return 생성된 이펙트 스펙. EffectClass가 null이면 nullptr.
 */
TSharedPtr<FTacticalEffectSpec> UAttributeSetComponentModel::MakeOutgoingSpec(TSubclassOf<UTacticalEffect> EffectClass, UTacticalEffectContext* Context) const
{
    if (Context == nullptr)
    {
        Context = MakeEffectContext();
    }

    if (EffectClass != nullptr)
    {
        UTacticalEffect* Effect = EffectClass->GetDefaultObject<UTacticalEffect>();

        TSharedPtr<FTacticalEffectSpec> NewSpec = MakeShared<FTacticalEffectSpec>(Effect, Context);
        return NewSpec;
    }

    return nullptr;
}

FActiveTacticalEffectHandle UAttributeSetComponentModel::ApplyTacticalEffectSpecToTarget(const FTacticalEffectSpec& Spec, UAttributeSetComponentModel* Target)
{
    FActiveTacticalEffectHandle ReturnHandle;
    if (Target != nullptr)
    {
        ReturnHandle = Target->ApplyTacticalEffectSpecToSelf(Spec);
    }
    return ReturnHandle;
}

FActiveTacticalEffectHandle UAttributeSetComponentModel::ApplyTacticalEffectSpecToSelf(const FTacticalEffectSpec& Spec)
{
	// 이펙트 적용 중 활성 이펙트 컨테이너 변경을 방지하는 스코프 락
	FScopedActiveTacticalEffectLock ScopeLock(mActiveAttributeEffects);
	// "현재 적용 중인 이펙트" 컨텍스트를 스코프 동안 설정(중첩 적용 추적용)
	FScopeCurrentTacticalEffectBeingApplied ScopedGEApplication(GetWorld(), &Spec, this);

	/* 진입 검사 */

	// 이펙트 자체의 적용 조건(태그 요구사항 등)을 통과하지 못하면 무효 핸들 반환
	if (Spec.mEffectClass->CanApply(mActiveAttributeEffects, Spec) == false)
	{
		return FActiveTacticalEffectHandle();
	}

	// 모디파이어가 가리키는 속성이 하나라도 무효면 적용 중단
	for (const FTacticalModifierInfo& Mod : Spec.mEffectClass->mModifiers)
	{
		if (Mod.mAttribute.IsValid() == false)
		{
			return FActiveTacticalEffectHandle();
		}
	}

	FActiveTacticalEffectHandle	MyHandle(INDEX_NONE, GetWorld());
	bool FoundExistingStackableGE = false;

	FActiveTacticalEffect* AppliedEffect = nullptr;
	FTacticalEffectSpec* OurCopyOfSpec = nullptr; // 실제 연산 대상이 되는 스펙(지속형은 컨테이너 보관본, 순간형은 로컬 복사본)
	TUniquePtr<FTacticalEffectSpec> StackSpec;
	{
		// 지속형(Has Duration/Infinite) 이펙트: 활성 이펙트 컨테이너에 등록하고 보관본 스펙을 참조
		if (Spec.mEffectClass->mDurationPolicy != ETacticalEffectDurationType::Instant)
		{
			AppliedEffect = mActiveAttributeEffects.ApplyTacticalEffectSpec(Spec, FoundExistingStackableGE);
			if (AppliedEffect == nullptr)
			{
				return FActiveTacticalEffectHandle();
			}

			MyHandle = AppliedEffect->mHandle;
			OurCopyOfSpec = &(AppliedEffect->mSpec);
		}

		if (OurCopyOfSpec == nullptr)
		{
			/* 인스턴스 Effect */

			// 순간형(Instant) 이펙트: 컨테이너에 보관하지 않으므로 로컬 복사본을 만들어 연산에 사용
			StackSpec = MakeUnique<FTacticalEffectSpec>(Spec);
			OurCopyOfSpec = StackSpec.Get();

			UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
			check(TacticalFrameworkModel != nullptr);
			// 전역 사전 처리 훅(시전자 기반 동적 모디파이어 캡처 등)
			TacticalFrameworkModel->GlobalPreTacticalEffectSpecApply(*OurCopyOfSpec, this);
		}
	}

	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
	check(TacticalFrameworkModel != nullptr);
	TacticalFrameworkModel->SetCurrentAppliedTE(OurCopyOfSpec); // 현재 적용 스펙을 프레임워크에 등록

	// 순간형이면 즉시 모디파이어를 실행해 BaseValue에 영구 반영
	if (Spec.mEffectClass->mDurationPolicy == ETacticalEffectDurationType::Instant)
	{
        ExecuteTacticalEffect(*OurCopyOfSpec);
	}

	Spec.mEffectClass->OnApplied(mActiveAttributeEffects, *OurCopyOfSpec); // 이펙트별 적용 후처리 콜백

    // 시전자 측 컴포넌트를 컨텍스트에서 얻어 "자신에게 적용됨" 통지
    UAttributeSetComponentModel* InstigatorASCModel = Spec.GetContext()->GetAttributeSetComponentModel();
    OnTacticalEffectAppliedToSelf(InstigatorASCModel, *OurCopyOfSpec, MyHandle);

	// 시전자가 존재하면 시전자 측에 "대상에게 적용됨" 통지(시전자 관점 콜백)
	if (InstigatorASCModel != nullptr)
	{
        InstigatorASCModel->OnTacticalEffectAppliedToTarget(this, *OurCopyOfSpec, MyHandle);
	}

	return MyHandle;
}

/**
 * @brief 순간형 이펙트의 모디파이어를 즉시 실행해 속성 BaseValue에 영구 반영한다.
 *        무한(Infinite) 이펙트는 이 경로로 실행될 수 없다(check로 보장).
 * @param Spec 실행할 이펙트 스펙.
 */
void UAttributeSetComponentModel::ExecuteTacticalEffect(FTacticalEffectSpec& Spec)
{
    checkf(Spec.GetDuration() == FTacticalEffectConstants::NO_DURATION, TEXT("즉발 이펙트만 가능"));
    mActiveAttributeEffects.ExecuteActiveEffectsFrom(Spec);
}

/**
 * @brief 활성 이펙트를 실제로 활성화한다. 부여 태그와 모디파이어를 컨테이너에 반영한다.
 *        도중 제거 대기(PendingRemove) 상태가 되면 무효 핸들을 반환한다.
 * @param ActiveHandle 활성화할 이펙트 핸들(소유권 이동).
 * @return 활성화 성공 시 동일 핸들, 제거 대기 시 무효 핸들.
 */
FActiveTacticalEffectHandle UAttributeSetComponentModel::SetActiveTacticalEffect(FActiveTacticalEffectHandle&& ActiveHandle)
{
    FActiveTacticalEffect* ActiveEffect = mActiveAttributeEffects.GetActiveTacticalEffect(ActiveHandle);
    checkf(ActiveEffect != nullptr, TEXT("활성화할 Effect 미존재"));

    FScopedActiveTacticalEffectLock ScopeLockActiveGameplayEffects(mActiveAttributeEffects); // 활성 이펙트 변경 락
    FScopedTacticalAggregatorOnDirtyBatch AggregatorOnDirtyBatcher(GetWorld());              // dirty 콜백을 배치로 묶어 중복 재계산 방지
    mActiveAttributeEffects.AddActiveTacticalEffectGrantedTagsAndModifiers(*ActiveEffect);   // 부여 태그+모디파이어 반영

    if (ActiveEffect->mIsPendingRemove == true)
    {
        return FActiveTacticalEffectHandle();
    }
    return MoveTemp(ActiveHandle);
}

/**
 * @brief 활성 이펙트를 제거한다(스택 단위 제거 지원).
 * @param Handle 제거할 활성 이펙트 핸들.
 * @param StacksToRemove 제거할 스택 수.
 * @return 제거 성공 여부.
 */
bool UAttributeSetComponentModel::RemoveActiveTacticalEffect(FActiveTacticalEffectHandle Handle, int32 StacksToRemove)
{
    return mActiveAttributeEffects.RemoveActiveTacticalEffect(Handle, StacksToRemove);
}

/**
 * @brief 활성 이펙트 핸들로부터 그 이펙트의 정의(CDO 클래스)를 조회한다.
 * @param Handle 조회할 활성 이펙트 핸들.
 * @return 이펙트 정의, 핸들이 무효면 nullptr.
 */
const UTacticalEffect* UAttributeSetComponentModel::GetTacticalEffectDefForHandle(FActiveTacticalEffectHandle Handle)
{
    FActiveTacticalEffect* ActiveEffect = mActiveAttributeEffects.GetActiveTacticalEffect(Handle);
    if (ActiveEffect != nullptr)
    {
        return ActiveEffect->mSpec.mEffectClass;
    }
    return nullptr;
}

const FActiveTacticalEffect* UAttributeSetComponentModel::GetActiveTacticalEffect(const FActiveTacticalEffectHandle Handle) const
{
    return mActiveAttributeEffects.GetActiveTacticalEffect(Handle);
}

void UAttributeSetComponentModel::CheckDurationExpired(const int32 Time, ETacticalEffectDurationUnitType UnitType)
{
    mActiveAttributeEffects.CheckDurationExpired(Time, UnitType);
}

void UAttributeSetComponentModel::OnTacticalEffectDurationChange(FActiveTacticalEffect& ActiveEffect)
{
}

/**
 * @brief 속성 Aggregator가 dirty(재계산 필요) 상태가 되었을 때의 콜백. 현재값 재계산을 트리거한다.
 * @param Aggregator dirty 상태가 된 Aggregator.
 * @param Attribute 해당 Aggregator가 담당하는 속성.
 */
void UAttributeSetComponentModel::OnAttributeAggregatorDirty(FTacticalAggregator* Aggregator, FTacticalAttribute Attribute)
{
    mActiveAttributeEffects.OnAttributeAggregatorDirty(Aggregator, Attribute);
}

/**
 * @brief 모디파이어 크기가 의존하는 Aggregator가 변경되었을 때의 콜백.
 *        (다른 속성에 기반해 크기가 계산되는 동적 모디파이어의 재계산 경로)
 * @param Handle 영향을 받는 활성 이펙트 핸들.
 * @param ChangedAggregator 변경된 의존 대상 Aggregator.
 */
void UAttributeSetComponentModel::OnMagnitudeDependencyChange(FActiveTacticalEffectHandle Handle, const FTacticalAggregator* ChangedAggregator)
{
    mActiveAttributeEffects.OnMagnitudeDependencyChange(Handle, ChangedAggregator);
}

void UAttributeSetComponentModel::RemoveLooseGameplayTagsMatchingTag(const FGameplayTag& GameplayTag, int32 Count)
{
    // 모든 LooseGameplayTag 가져오기
    const FGameplayTagContainer& OwnedTags = GetOwnedGameplayTags();

    // 부모 태그 하위의 자식 태그들만 필터링
    FGameplayTagContainer SubTags = OwnedTags.Filter(FGameplayTagContainer(GameplayTag));

    // 필터링된 하위 태그들을 각각 원하는 스택 크기로 감소
    if (SubTags.IsEmpty() == false)
    {
        RemoveLooseGameplayTags(SubTags, Count);
    }
}

/**
 * @brief 특정 게임플레이 태그의 카운트 변화 이벤트를 등록한다(구독).
 * @param Tag 구독할 태그.
 * @param EventType 이벤트 종류(추가/제거/신규/소멸 등).
 * @return 태그 카운트 변화 시 호출되는 델리게이트 참조.
 */
FOnTacticalEffectTagCountChanged& UAttributeSetComponentModel::RegisterTacticalTagEvent(FGameplayTag Tag, EGameplayTagEventType::Type EventType)
{
    return mTacticalTagCountContainer.RegisterGameplayTagEvent(Tag, EventType);
}

/**
 * @brief 등록했던 태그 이벤트 구독을 해제한다.
 * @param DelegateHandle 해제할 델리게이트 핸들.
 * @param Tag 대상 태그.
 * @param EventType 이벤트 종류.
 * @return 제거 성공 여부.
 */
bool UAttributeSetComponentModel::UnregisterTacticalTagEvent(FDelegateHandle DelegateHandle, FGameplayTag Tag, EGameplayTagEventType::Type EventType)
{
    return mTacticalTagCountContainer.RegisterGameplayTagEvent(Tag, EventType).Remove(DelegateHandle);
}

/**
 * @brief 컨테이너 내 모든 태그에 대해 스택 카운트 변경을 통지한다.
 * @param Container 통지할 태그들의 컨테이너.
 */
void UAttributeSetComponentModel::NotifyTagMap_StackCountChange(const FGameplayTagContainer& Container)
{
    for (TArray<FGameplayTag>::TConstIterator TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
    {
        const FGameplayTag& Tag = *TagIt;
        mTacticalTagCountContainer.Notify_StackCountChange(Tag);
    }
}

/**
 * @brief 단일 태그의 카운트를 증감하고, 0→양수/양수→0 경계 변화 시 OnTagUpdated를 통지한다.
 * @param Tag 증감할 태그.
 * @param CountDelta 카운트 증감량(양수=추가, 음수=제거).
 */
void UAttributeSetComponentModel::UpdateTagMapSingle_Internal(const FGameplayTag& Tag, int32 CountDelta)
{
    if (CountDelta > 0)
    {
        // 카운트 증가: 0→양수로 새로 존재하게 되면 true 반환 → 추가 통지
        if (mTacticalTagCountContainer.UpdateTagCount(Tag, CountDelta))
        {
            OnTagUpdated(Tag, true);
        }
    }
    else if (CountDelta < 0)
    {
        // 카운트 감소: 부모 태그 제거를 지연 처리해 순회 중 변경을 피한다
        TArray<FDeferredTagChangeDelegate> DeferredTagChangeDelegates;
        if (mTacticalTagCountContainer.UpdateTagCount_DeferredParentRemoval(Tag, CountDelta, DeferredTagChangeDelegates))
        {
            mTacticalTagCountContainer.FillParentTags(); // 부모 태그 계층 재구성
            OnTagUpdated(Tag, false);                    // 제거 통지

            // 지연해 둔 부모 태그 변화 델리게이트들을 안전한 시점에 실행
            for (FDeferredTagChangeDelegate& Delegate : DeferredTagChangeDelegates)
            {
                Delegate.Execute();
            }
        }
    }
}

/**
 * @brief 여러 태그(컨테이너)의 카운트를 일괄 증감하고 경계 변화에 대해 통지한다.
 * @param Container 증감 대상 태그 컨테이너.
 * @param CountDelta 카운트 증감량(양수=추가, 음수=제거).
 */
void UAttributeSetComponentModel::UpdateTagMap_Internal(const FGameplayTagContainer& Container, int32 CountDelta)
{
	if (CountDelta > 0)
	{
		// 추가 경로: 각 태그를 증가시키고 새로 존재하게 된 태그마다 즉시 통지
		for (TArray<FGameplayTag>::TConstIterator TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
		{
			const FGameplayTag& Tag = *TagIt;
			if (mTacticalTagCountContainer.UpdateTagCount(Tag, CountDelta))
			{
				OnTagUpdated(Tag, true);
			}
		}
	}
	else if (CountDelta < 0)
	{
		// 제거 경로: 부모 태그 제거를 지연하고, 실제 사라진 태그를 모아 일괄 후처리
		TArray<FGameplayTag> RemovedTags;
		RemovedTags.Reserve(Container.Num());
		TArray<FDeferredTagChangeDelegate> DeferredTagChangeDelegates;

		for (TArray<FGameplayTag>::TConstIterator TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
		{
			const FGameplayTag& Tag = *TagIt;
			if (mTacticalTagCountContainer.UpdateTagCount_DeferredParentRemoval(Tag, CountDelta, DeferredTagChangeDelegates))
			{
				RemovedTags.Add(Tag); // 0으로 떨어진 태그만 수집
			}
		}

		if (RemovedTags.Num() > 0)
		{
            mTacticalTagCountContainer.FillParentTags(); // 부모 태그 계층을 한 번에 재구성
		}

		// 지연된 부모 태그 변화 델리게이트 실행
		for (FDeferredTagChangeDelegate& Delegate : DeferredTagChangeDelegates)
		{
			Delegate.Execute();
		}

		// 사라진 태그들에 대한 제거 통지(계층 재구성 이후라 일관된 상태에서 통지됨)
		for (FGameplayTag& Tag : RemovedTags)
		{
			OnTagUpdated(Tag, false);
		}
	}
}

/**
 * @brief 태그 존재 상태가 변할 때 호출되는 가상 후크. 파생/외부에서 재정의해 반응할 수 있다(기본 구현 없음).
 * @param Tag 변경된 태그.
 * @param TagExists 변경 후 해당 태그가 존재하는지 여부(true=추가, false=제거).
 */
void UAttributeSetComponentModel::OnTagUpdated(const FGameplayTag& Tag, bool TagExists)
{
}
