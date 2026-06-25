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
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Simulation/RoomInstance.h"
#include "GameplayTagsManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

// USimulationSubsystem의 protected 멤버 접근용 우회 클래스
class FSimulationSubsystemTestAccessor : public USimulationSubsystem
{
public:
    static URoomInstance* GetRoomInstance(USimulationSubsystem* Subsystem)
    {
        if (Subsystem == nullptr) return nullptr;
        const FSimulationSubsystemTestAccessor* Accessor = static_cast<const FSimulationSubsystemTestAccessor*>(Subsystem);
        return Accessor->mGameRoomContext.mRoomInstance;
    }
};

// 활성화된 월드(PIE, Game)를 탐색하는 헬퍼 함수
static UWorld* GetAnyGameWorld()
{
    if (GEngine == nullptr) return nullptr;
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
    if (!World)
    {
        World = GWorld;
    }
    if (!World)
    {
        AddError(TEXT("Failed to find a valid UWorld"));
        return false;
    }

    USimulationSubsystem* SimSubsystem = World->GetSubsystem<USimulationSubsystem>();
    if (!TestNotNull(TEXT("USimulationSubsystem should be initialized"), SimSubsystem))
    {
        return false;
    }

    URoomInstance* RoomInstance = FSimulationSubsystemTestAccessor::GetRoomInstance(SimSubsystem);
    if (!TestNotNull(TEXT("RoomInstance should be initialized"), RoomInstance))
    {
        return false;
    }

    // UTacticalFrameworkModel 모크 등록 (기존 등록 상태가 없을 때만 등록)
    UTacticalFrameworkModel* FrameworkModel = Cast<UTacticalFrameworkModel>(RoomInstance->mAliveSubsystemModels.FindRef(UTacticalFrameworkModel::StaticClass()));
    bool bCreatedFrameworkMock = false;
    if (!FrameworkModel)
    {
        FrameworkModel = NewObject<UTacticalFrameworkModel>(RoomInstance);
        RoomInstance->mAliveSubsystemModels.Add(UTacticalFrameworkModel::StaticClass(), FrameworkModel);
        bCreatedFrameworkMock = true;
    }

    // ----------------------------------------------------
    // 2. 모델 및 속성 컴포넌트 모델 생성 & 초기화
    // ----------------------------------------------------
    UTASActorModelMock* MockActorModel = NewObject<UTASActorModelMock>(World);
    MockActorModel->Initialize();
    MockActorModel->BeginPlay();

    UAttributeSetComponentModel* CompModel = MockActorModel->GetAttributeComponent();
    if (!TestNotNull(TEXT("AttributeSetComponentModel should be valid"), CompModel))
    {
        return false;
    }

    // AttributeSet이 정상 부착되었는지 확인
    TestTrue(TEXT("UUnitAttributeSet should be registered"), CompModel->HasAttributeSetForAttribute(UUnitAttributeSet::GetHPAttribute()));

    // ----------------------------------------------------
    // 3. 테스트 케이스 1: 기본 속성 변경 검증 (HP 및 MaxHP 초기화)
    // ----------------------------------------------------
    AddInfo(TEXT("=== Test Case 1: Base Value & Current Value Initialization ==="));
    CompModel->SetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute(), 100.f);
    CompModel->SetAttributeBaseValue(UUnitAttributeSet::GetHPAttribute(), 100.f);

    TestEqual(TEXT("Base MaxHP should be 100.f"), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute()), 100.f);
    TestEqual(TEXT("Current MaxHP should be 100.f"), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMaxHPAttribute()), 100.f);
    TestEqual(TEXT("Base HP should be 100.f"), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetHPAttribute()), 100.f);
    TestEqual(TEXT("Current HP should be 100.f"), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute()), 100.f);

    // ----------------------------------------------------
    // 4. 테스트 케이스 2: Instant Effect 적용 검증 (MaxHP 데미지 차감 -20)
    // ----------------------------------------------------
    AddInfo(TEXT("=== Test Case 2: Apply Instant Effect ==="));

    UTacticalEffectContext* EffectContext1 = CompModel->MakeEffectContext();
    EffectContext1->SetInstigator(MockActorModel);
    EffectContext1->SetAttributeSetComponentModel(CompModel);

    TSharedPtr<FTacticalEffectSpec> InstantSpec = CompModel->MakeOutgoingSpec(UTestInstantTacticalEffect::StaticClass(), EffectContext1);
    InstantSpec->mDynamicMagnitude = -20.f;
    CompModel->ApplyTacticalEffectSpecToSelf(*InstantSpec);

    // Instant Effect는 베이스 값과 현재 값을 모두 영구적으로 변화시킴
    TestEqual(TEXT("After Instant HP Mod, Base MaxHP should be 80.f"), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute()), 80.f);
    TestEqual(TEXT("After Instant HP Mod, Current MaxHP should be 80.f"), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMaxHPAttribute()), 80.f);

    // ----------------------------------------------------
    // 5. 테스트 케이스 3: Infinite Effect 적용 및 해제 검증 (DefensePoint +30)
    // ----------------------------------------------------
    AddInfo(TEXT("=== Test Case 3: Apply & Remove Infinite Effect ==="));
    CompModel->SetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute(), 10.f);
    TestEqual(TEXT("Base Defense should be 10.f"), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute()), 10.f);

    UTacticalEffectContext* EffectContext2 = CompModel->MakeEffectContext();
    EffectContext2->SetInstigator(MockActorModel);
    EffectContext2->SetAttributeSetComponentModel(CompModel);

    TSharedPtr<FTacticalEffectSpec> InfiniteEffect = CompModel->MakeOutgoingSpec(UTestInfiniteTacticalEffect::StaticClass(), EffectContext2);
    InfiniteEffect->mDynamicMagnitude = 30.f;
    FActiveTacticalEffectHandle ActiveHandle = CompModel->ApplyTacticalEffectSpecToSelf(*InfiniteEffect);

    TestTrue(TEXT("ActiveHandle should be valid"), ActiveHandle.IsValid());
    // Infinite Effect는 베이스 값은 그대로 두고 현재 값만 변화시킴 (Aggregator에 가산)
    TestEqual(TEXT("Base Defense should remain 10.f"), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute()), 10.f);
    TestEqual(TEXT("Current Defense should be 40.f (Base 10 + Mod 30)"), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute()), 40.f);

    // 이펙트 제거 후 원래대로 복원되는지 확인
    CompModel->RemoveActiveTacticalEffect(ActiveHandle);
    TestEqual(TEXT("After removing Infinite Effect, Current Defense should return to 10.f"), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute()), 10.f);

    // ----------------------------------------------------
    // 6. 테스트 케이스 4: Loose Tag 부여 및 해제 검증
    // ----------------------------------------------------
    AddInfo(TEXT("=== Test Case 4: Loose Tag Add & Remove ==="));
    // 테스트에 사용할 태그 등록
    FGameplayTag TestTagA = UGameplayTagsManager::Get().AddNativeGameplayTag(TEXT("Test.GameplayAbility.Skill"), TEXT("Loose Tag for unit tests"));
    
    TestFalse(TEXT("Should not have TestTagA initially"), CompModel->HasMatchingGameplayTag(TestTagA));

    CompModel->AddLooseGameplayTag(TestTagA);
    TestTrue(TEXT("Should have TestTagA after addition"), CompModel->HasMatchingGameplayTag(TestTagA));
    TestEqual(TEXT("TestTagA count should be 1"), CompModel->GetTagCount(TestTagA), 1);

    CompModel->RemoveLooseGameplayTag(TestTagA);
    TestFalse(TEXT("Should not have TestTagA after removal"), CompModel->HasMatchingGameplayTag(TestTagA));

    // ----------------------------------------------------
    // 7. 테스트 케이스 5: Effect를 통한 Granted Tag 부여 및 해제 검증
    // ----------------------------------------------------
    AddInfo(TEXT("=== Test Case 5: Granted Tag by Infinite Effect ==="));
    FGameplayTag GrantedTag = UGameplayTagsManager::Get().AddNativeGameplayTag(TEXT("Test.GameplayAbility.SkillGranted"), TEXT("Granted Tag for unit tests"));

    UTacticalEffect* TagEffect = NewObject<UTacticalEffect>();
    TagEffect->mDurationPolicy = ETacticalEffectDurationType::Infinite;
    TagEffect->mCachedGrantedTags.AddTag(GrantedTag);

    UTacticalEffectContext* EffectContext3 = NewObject<UTacticalEffectContext>(World);
    EffectContext3->SetInstigator(MockActorModel);
    EffectContext3->SetAttributeSetComponentModel(CompModel);

    FTacticalEffectSpec TagSpec(TagEffect, EffectContext3);
    FActiveTacticalEffectHandle TagActiveHandle = CompModel->ApplyTacticalEffectSpecToSelf(TagSpec);

    TestTrue(TEXT("GrantedTag should be active on CompModel"), CompModel->HasMatchingGameplayTag(GrantedTag));
    TestEqual(TEXT("GrantedTag count should be 1"), CompModel->GetTagCount(GrantedTag), 1);

    CompModel->RemoveActiveTacticalEffect(TagActiveHandle);
    TestFalse(TEXT("GrantedTag should be removed from CompModel"), CompModel->HasMatchingGameplayTag(GrantedTag));

    // ----------------------------------------------------
    // 8. 환경 정리 (Clean up)
    // ----------------------------------------------------
    MockActorModel->EndPlay();
    MockActorModel->Uninitialize();

    CompModel->MarkAsGarbage();
    MockActorModel->MarkAsGarbage();

    if (bCreatedFrameworkMock && RoomInstance)
    {
        RoomInstance->mAliveSubsystemModels.Remove(UTacticalFrameworkModel::StaticClass());
        FrameworkModel->MarkAsGarbage();
    }

    return true;
}
