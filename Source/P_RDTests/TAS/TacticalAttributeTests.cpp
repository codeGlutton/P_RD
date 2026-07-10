/*****************************************************************//**
 * @file   TacticalAttributeTests.cpp
 * @brief  UAttributeSetComponentModel 자동화 테스트
 * @details
 * AttributeSetComponentModel 및 관련 이펙트/태그 시스템의 동작을 검증한다.
 * @author 모호재
 * @date   2026-06-26
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "TASAttributeTestsHelper.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/AttributeSetMinimal.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Effect/Stat/TacticalEffect_HP.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Simulation/RoomInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

/**
 * @brief 활성화된 월드(PIE, Game)를 탐색하는 헬퍼 함수
 * @return 현재 월드
 */
static UWorld* GetAnyGameWorld()
{
    if (GEngine == nullptr)
    {
        return nullptr;
    }
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World() != nullptr)
        {
            return Context.World();
        }
    }
    return nullptr;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTacticalAttributeTests,
    "P_RD.TAS.AttributeSetComponentModel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTacticalAttributeTests::RunTest(const FString& Parameters)
{
    // ----------------------------------------------------
    // 1. 에디터에 이미 존재하는 활성화된 GWorld 환경 사용
    // ----------------------------------------------------
    UWorld* World = GetAnyGameWorld();
    if (World == nullptr)
    {
        World = GWorld;
    }
    if (World == nullptr)
    {
        AddError(TEXT("유효한 UWorld를 찾는 데 실패했습니다."));
        return false;
    }

    USimulationSubsystem* SimSubsystem = World->GetSubsystem<USimulationSubsystem>();
    if (TestNotNull(TEXT("USimulationSubsystem이 초기화되어야 합니다."), SimSubsystem) == false)
    {
        return false;
    }

    URoomInstance* RoomInstance = FSimulationSubsystemTestAccessor::GetRoomInstance(SimSubsystem);
    if (TestNotNull(TEXT("RoomInstance가 초기화되어야 합니다."), RoomInstance) == false)
    {
        return false;
    }

    // UTacticalFrameworkModel 모크 등록 (기존 등록 상태가 없을 때만 등록)
    UTacticalFrameworkModel* FrameworkModel = Cast<UTacticalFrameworkModel>(RoomInstance->mAliveSubsystemModels.FindRef(UTacticalFrameworkModel::StaticClass()));
    if (FrameworkModel == nullptr)
    {
        FrameworkModel = NewObject<UTacticalFrameworkModel>(RoomInstance);
        RoomInstance->mAliveSubsystemModels.Add(UTacticalFrameworkModel::StaticClass(), FrameworkModel);
    }

    // ----------------------------------------------------
    // 2. 모델 및 속성 컴포넌트 모델 생성 & 초기화
    // ----------------------------------------------------
    UTASActorModelMock* MockActorModel = NewObject<UTASActorModelMock>(World);
    MockActorModel->Initialize();
    MockActorModel->BeginPlay();

    UAttributeSetComponentModel* CompModel = MockActorModel->GetAttributeComponent();
    if (TestNotNull(TEXT("AttributeSetComponentModel이 유효해야 합니다."), CompModel) == false)
    {
        return false;
    }

    // AttributeSet이 정상 부착되었는지 확인
    TestTrue(TEXT("UUnitAttributeSet이 등록되어야 합니다."), CompModel->HasAttributeSetForAttribute(UUnitAttributeSet::GetHPAttribute()));

    // ----------------------------------------------------
    // 3. 테스트 케이스 1: 기본 속성 변경 검증 (HP 및 MaxHP 초기화)
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 1: 기본값 및 현재값 초기화 검증 ==="));
    CompModel->SetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute(), 100.f);
    CompModel->SetAttributeBaseValue(UUnitAttributeSet::GetHPAttribute(), 100.f);

    TestEqual(TEXT("기본 MaxHP는 100.f이어야 합니다."), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute()), 100.f);
    TestEqual(TEXT("현재 MaxHP는 100.f이어야 합니다."), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMaxHPAttribute()), 100.f);
    TestEqual(TEXT("기본 HP는 100.f이어야 합니다."), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetHPAttribute()), 100.f);
    TestEqual(TEXT("현재 HP는 100.f이어야 합니다."), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()), 100.f);

    // ----------------------------------------------------
    // 4. 테스트 케이스 2: Instant Effect 적용 검증 (MaxHP 데미지 차감 -20)
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 2: Instant 이펙트 적용 검증 ==="));

    UTacticalEffectContext* EffectContext1 = CompModel->MakeEffectContext();
    EffectContext1->SetInstigator(MockActorModel);
    EffectContext1->SetAttributeSetComponentModel(CompModel);

    TSharedPtr<FTacticalEffectSpec> InstantSpec = CompModel->MakeOutgoingSpec(UTestInstantTacticalEffect::StaticClass(), EffectContext1);
    InstantSpec->mDynamicMagnitude = -20.f;
    CompModel->ApplyTacticalEffectSpecToSelf(*InstantSpec);

    // Instant Effect는 베이스 값과 현재 값을 모두 영구적으로 변화시킴
    TestEqual(TEXT("Instant HP 변경 후, 기본 MaxHP는 80.f이어야 합니다."), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute()), 80.f);
    TestEqual(TEXT("Instant HP 변경 후, 현재 MaxHP는 80.f이어야 합니다."), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMaxHPAttribute()), 80.f);

    // ----------------------------------------------------
    // 5. 테스트 케이스 3: Infinite Effect 적용 및 해제 검증 (DefensePoint +30)
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 3: Infinite 이펙트 적용 및 제거 검증 ==="));
    CompModel->SetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute(), 10.f);
    TestEqual(TEXT("기본 방어력은 10.f이어야 합니다."), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute()), 10.f);

    UTacticalEffectContext* EffectContext2 = CompModel->MakeEffectContext();
    EffectContext2->SetInstigator(MockActorModel);
    EffectContext2->SetAttributeSetComponentModel(CompModel);

    TSharedPtr<FTacticalEffectSpec> InfiniteEffect = CompModel->MakeOutgoingSpec(UTestInfiniteTacticalEffect::StaticClass(), EffectContext2);
    InfiniteEffect->mDynamicMagnitude = 30.f;
    FActiveTacticalEffectHandle ActiveHandle = CompModel->ApplyTacticalEffectSpecToSelf(*InfiniteEffect);

    TestTrue(TEXT("ActiveHandle이 유효해야 합니다."), ActiveHandle.IsValid());
    // Infinite Effect는 베이스 값은 그대로 두고 현재 값만 변화시킴 (Aggregator에 가산)
    TestEqual(TEXT("기본 방어력은 10.f로 유지되어야 합니다."), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute()), 10.f);
    TestEqual(TEXT("현재 방어력은 40.f이어야 합니다. (기본 10 + 모디파이어 30)"), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute()), 40.f);

    // 이펙트 제거 후 원래대로 복원되는지 확인
    CompModel->RemoveActiveTacticalEffect(ActiveHandle);
    TestEqual(TEXT("Infinite 이펙트 제거 후, 현재 방어력은 10.f로 복구되어야 합니다."), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute()), 10.f);

    // ----------------------------------------------------
    // 6. 테스트 케이스 4: Loose Tag 부여 및 해제 검증
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 4: Loose 태그 추가 및 제거 검증 ==="));
    // 테스트에 사용할 태그 등록
    FGameplayTag TestTagA = AbilityTags::GameplayAbility_LevelUp;

    TestFalse(TEXT("초기에는 TestTagA를 가지고 있지 않아야 합니다."), CompModel->HasMatchingGameplayTag(TestTagA));

    CompModel->AddLooseGameplayTag(TestTagA);
    TestTrue(TEXT("태그 추가 후에는 TestTagA를 가지고 있어야 합니다."), CompModel->HasMatchingGameplayTag(TestTagA));
    TestEqual(TEXT("TestTagA의 개수는 1이어야 합니다."), CompModel->GetTagCount(TestTagA), 1);

    CompModel->RemoveLooseGameplayTag(TestTagA);
    TestFalse(TEXT("태그 제거 후에는 TestTagA를 가지고 있지 않아야 합니다."), CompModel->HasMatchingGameplayTag(TestTagA));

    // ----------------------------------------------------
    // 7. 테스트 케이스 5: Effect를 통한 Granted Tag 부여 및 해제 검증
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 5: Infinite 이펙트에 의한 Granted 태그 부여 검증 ==="));
    FGameplayTag GrantedTag = AbilityTags::GameplayAbility_Passive_OnEndApplyingEffect;

    UTacticalEffectContext* EffectContext3 = CompModel->MakeEffectContext();
    EffectContext3->SetInstigator(MockActorModel);
    EffectContext3->SetAttributeSetComponentModel(CompModel);

    TSharedPtr<FTacticalEffectSpec> TagSpec = CompModel->MakeOutgoingSpec(UTestInfiniteTagEffect::StaticClass(), EffectContext3);
    FActiveTacticalEffectHandle TagActiveHandle = CompModel->ApplyTacticalEffectSpecToSelf(*TagSpec);

    TestTrue(TEXT("CompModel에 GrantedTag가 활성화되어야 합니다."), CompModel->HasMatchingGameplayTag(GrantedTag));
    TestEqual(TEXT("GrantedTag의 개수는 1이어야 합니다."), CompModel->GetTagCount(GrantedTag), 1);

    CompModel->RemoveActiveTacticalEffect(TagActiveHandle);
    TestFalse(TEXT("CompModel에서 GrantedTag가 제거되어야 합니다."), CompModel->HasMatchingGameplayTag(GrantedTag));

    // ----------------------------------------------------
    // 8. 테스트 케이스 6: 손상된 Spec/Context/ModifierOp 방어 검증
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 6: 잘못된 Effect 입력 방어 검증 ==="));
    const float MaxHPBeforeInvalidEffects = CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute());

    FTacticalEffectSpec EmptySpec;
    const FActiveTacticalEffectHandle EmptySpecHandle = CompModel->ApplyTacticalEffectSpecToSelf(EmptySpec);
    TestFalse(TEXT("빈 Spec은 무효 핸들을 반환해야 합니다."), EmptySpecHandle.IsValid());

    const UTacticalEffect* InstantEffectCDO = UTestInstantTacticalEffect::StaticClass()->GetDefaultObject<UTacticalEffect>();
    FTacticalEffectSpec MissingContextSpec(InstantEffectCDO, nullptr);
    const FActiveTacticalEffectHandle MissingContextHandle = CompModel->ApplyTacticalEffectSpecToSelf(MissingContextSpec);
    TestFalse(TEXT("Context가 없는 Spec은 무효 핸들을 반환해야 합니다."), MissingContextHandle.IsValid());

    UTacticalEffect* InvalidOpEffect = NewObject<UTacticalEffect>(MockActorModel);
    InvalidOpEffect->mDurationPolicy = ETacticalEffectDurationType::Instant;
    InvalidOpEffect->mStackingType = ETacticalEffectStackingType::None;
    FTacticalModifierInfo InvalidModifier;
    InvalidModifier.mAttribute = UUnitAttributeSet::GetMaxHPAttribute();
    InvalidModifier.mModifierOp = static_cast<ETacticalModOp::Type>(ETacticalModOp::Max);
    InvalidModifier.mModifierMagnitude = 1000.f;
    InvalidOpEffect->mModifiers.Add(InvalidModifier);

    UTacticalEffectContext* InvalidOpContext = CompModel->MakeEffectContext();
    InvalidOpContext->SetAttributeSetComponentModel(CompModel);
    FTacticalEffectSpec InvalidOpSpec(InvalidOpEffect, InvalidOpContext);
    const FActiveTacticalEffectHandle InvalidOpHandle = CompModel->ApplyTacticalEffectSpecToSelf(InvalidOpSpec);
    TestFalse(TEXT("범위를 벗어난 ModifierOp는 무효 핸들을 반환해야 합니다."), InvalidOpHandle.IsValid());
    TestEqual(TEXT("잘못된 Effect 입력은 속성값을 변경하지 않아야 합니다."), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute()), MaxHPBeforeInvalidEffects);

    // ----------------------------------------------------
    // 9. 테스트 케이스 7: HP 이펙트 중첩 실행과 사망 태그 검증
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 7: HP 이펙트 도중 사망 이펙트 중첩 실행 검증 ==="));
    CompModel->SetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute(), 100.f);
    CompModel->SetAttributeBaseValue(UUnitAttributeSet::GetHPAttribute(), 10.f);

    UTacticalEffectContext* DamageContext = CompModel->MakeEffectContext();
    DamageContext->SetAttributeSetComponentModel(CompModel);
    TSharedPtr<FTacticalEffectSpec> DamageSpec = CompModel->MakeOutgoingSpec(UTacticalEffect_HP::StaticClass(), DamageContext);
    DamageSpec->mDynamicMagnitude = -20.f;
    CompModel->ApplyTacticalEffectSpecToSelf(*DamageSpec);

    TestEqual(TEXT("치명 피해 후 HP는 0이어야 합니다."), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()), 0.f);
    TestTrue(TEXT("치명 피해의 중첩 이펙트 실행 후 사망 태그가 있어야 합니다."), CompModel->HasMatchingGameplayTag(EffectTags::GameplayEffect_ActorState_Dead));

    // ----------------------------------------------------
    // 10. 테스트 케이스 8: Dirty 배치 콜백 중 다음 Aggregator 제거 검증
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 8: Dirty 배치 중 Aggregator 제거 안전성 검증 ==="));
    UTacticalEffectContext* FirstDirtyContext = CompModel->MakeEffectContext();
    FirstDirtyContext->SetAttributeSetComponentModel(CompModel);
    TSharedPtr<FTacticalEffectSpec> FirstDirtySpec = CompModel->MakeOutgoingSpec(UTestInfiniteTacticalEffect::StaticClass(), FirstDirtyContext);
    const FActiveTacticalEffectHandle FirstDirtyHandle = CompModel->ApplyTacticalEffectSpecToSelf(*FirstDirtySpec);

    UTacticalEffect* SecondDirtyEffect = NewObject<UTacticalEffect>(MockActorModel);
    SecondDirtyEffect->mDurationPolicy = ETacticalEffectDurationType::Infinite;
    SecondDirtyEffect->mStackingType = ETacticalEffectStackingType::None;
    FTacticalModifierInfo SecondDirtyModifier;
    SecondDirtyModifier.mAttribute = UUnitAttributeSet::GetMaxHPAttribute();
    SecondDirtyModifier.mModifierOp = ETacticalModOp::AddBase;
    SecondDirtyModifier.mModifierMagnitude = 1.f;
    SecondDirtyEffect->mModifiers.Add(SecondDirtyModifier);

    UTacticalEffectContext* SecondDirtyContext = CompModel->MakeEffectContext();
    SecondDirtyContext->SetAttributeSetComponentModel(CompModel);
    FTacticalEffectSpec SecondDirtySpec(SecondDirtyEffect, SecondDirtyContext);
    const FActiveTacticalEffectHandle SecondDirtyHandle = CompModel->ApplyTacticalEffectSpecToSelf(SecondDirtySpec);

    bool bFirstDirtyBroadcasted = false;
    bool bSecondEffectRemovedInCallback = false;
    const FDelegateHandle DirtyCallbackHandle = CompModel->GetTacticalAttributeValueChangeDelegate(UUnitAttributeSet::GetDefensePointAttribute()).AddLambda(
        [CompModel, &bFirstDirtyBroadcasted, &bSecondEffectRemovedInCallback, SecondDirtyHandle](const FTacticalAttributeChangeData&)
    {
        bFirstDirtyBroadcasted = true;
        bSecondEffectRemovedInCallback = CompModel->RemoveActiveTacticalEffect(SecondDirtyHandle);
    });

    FrameworkModel->BeginAggregatorDirtyBatch();
    CompModel->SetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute(), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute()) + 1.f);
    CompModel->SetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute(), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute()) + 1.f);
    FrameworkModel->EndAggregatorDirtyBatch();

    TestTrue(TEXT("첫 Aggregator의 Dirty 콜백이 실행되어야 합니다."), bFirstDirtyBroadcasted);
    TestTrue(TEXT("앞선 콜백에서 다음 Aggregator의 Effect가 안전하게 제거되어야 합니다."), bSecondEffectRemovedInCallback);
    CompModel->GetTacticalAttributeValueChangeDelegate(UUnitAttributeSet::GetDefensePointAttribute()).Remove(DirtyCallbackHandle);
    CompModel->RemoveActiveTacticalEffect(FirstDirtyHandle);

    // ----------------------------------------------------
    // 11. 환경 정리 (Clean up)
    // ----------------------------------------------------
    MockActorModel->EndPlay();
    MockActorModel->Uninitialize();

    CompModel->MarkAsGarbage();
    MockActorModel->MarkAsGarbage();

    if (RoomInstance != nullptr)
    {
        RoomInstance->mAliveSubsystemModels.Remove(UTacticalFrameworkModel::StaticClass());
        FrameworkModel->MarkAsGarbage();
    }

    return true;
}
