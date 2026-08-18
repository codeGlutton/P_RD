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
    TestTrue(TEXT("UUnitAttributeSet이 등록되어야 합니다."), CompModel->HasAttributeSetForAttribute(UCombatTargetAttributeSet::GetHPAttribute()));

    // ----------------------------------------------------
    // 3. 테스트 케이스 1: 기본 속성 변경 검증 (HP 및 MaxHP 초기화)
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 1: 기본값 및 현재값 초기화 검증 ==="));
    CompModel->SetAttributeBaseValue(UCombatTargetAttributeSet::GetMaxHPAttribute(), 100.f);
    CompModel->SetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute(), 100.f);

    TestEqual(TEXT("기본 MaxHP는 100.f이어야 합니다."), CompModel->GetAttributeBaseValue(UCombatTargetAttributeSet::GetMaxHPAttribute()), 100.f);
    TestEqual(TEXT("현재 MaxHP는 100.f이어야 합니다."), CompModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetMaxHPAttribute()), 100.f);
    TestEqual(TEXT("기본 HP는 100.f이어야 합니다."), CompModel->GetAttributeBaseValue(UCombatTargetAttributeSet::GetHPAttribute()), 100.f);
    TestEqual(TEXT("현재 HP는 100.f이어야 합니다."), CompModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHPAttribute()), 100.f);

    // ----------------------------------------------------
    // 4. 테스트 케이스 2: Instant Effect 적용 검증 (MaxHP 데미지 차감 -20)
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 2: Instant 이펙트 적용 검증 ==="));

    UTacticalEffectContext* EffectContext1 = CompModel->MakeEffectContext();

    TSharedPtr<FTacticalEffectSpec> InstantSpec = CompModel->MakeOutgoingSpec(UTestInstantTacticalEffect::StaticClass(), EffectContext1);
    InstantSpec->mDynamicMagnitude = -20.f;
    CompModel->ApplyTacticalEffectSpecToSelf(*InstantSpec);

    // Instant Effect는 베이스 값과 현재 값을 모두 영구적으로 변화시킴
    TestEqual(TEXT("Instant HP 변경 후, 기본 MaxHP는 80.f이어야 합니다."), CompModel->GetAttributeBaseValue(UCombatTargetAttributeSet::GetMaxHPAttribute()), 80.f);
    TestEqual(TEXT("Instant HP 변경 후, 현재 MaxHP는 80.f이어야 합니다."), CompModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetMaxHPAttribute()), 80.f);

    // ----------------------------------------------------
    // 5. 테스트 케이스 3: Infinite Effect 적용 및 해제 검증 (Defense +30)
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 3: Infinite 이펙트 적용 및 제거 검증 ==="));
    CompModel->SetAttributeBaseValue(UCombatTargetAttributeSet::GetDefenseAttribute(), 10.f);
    TestEqual(TEXT("기본 방어력은 10.f이어야 합니다."), CompModel->GetAttributeBaseValue(UCombatTargetAttributeSet::GetDefenseAttribute()), 10.f);

    UTacticalEffectContext* EffectContext2 = CompModel->MakeEffectContext();

    TSharedPtr<FTacticalEffectSpec> InfiniteEffect = CompModel->MakeOutgoingSpec(UTestInfiniteTacticalEffect::StaticClass(), EffectContext2);
    InfiniteEffect->mDynamicMagnitude = 30.f;
    FActiveTacticalEffectHandle ActiveHandle = CompModel->ApplyTacticalEffectSpecToSelf(*InfiniteEffect);

    TestTrue(TEXT("ActiveHandle이 유효해야 합니다."), ActiveHandle.IsValid());
    // Infinite Effect는 베이스 값은 그대로 두고 현재 값만 변화시킴 (Aggregator에 가산)
    TestEqual(TEXT("기본 방어력은 10.f로 유지되어야 합니다."), CompModel->GetAttributeBaseValue(UCombatTargetAttributeSet::GetDefenseAttribute()), 10.f);
    TestEqual(TEXT("현재 방어력은 40.f이어야 합니다. (기본 10 + 모디파이어 30)"), CompModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetDefenseAttribute()), 40.f);

    // 이펙트 제거 후 원래대로 복원되는지 확인
    CompModel->RemoveActiveTacticalEffect(ActiveHandle);
    TestEqual(TEXT("Infinite 이펙트 제거 후, 현재 방어력은 10.f로 복구되어야 합니다."), CompModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetDefenseAttribute()), 10.f);

    // ----------------------------------------------------
    // 6. 테스트 케이스 4: Loose Tag 부여 및 해제 검증
    // ----------------------------------------------------
    AddInfo(TEXT("=== 테스트 케이스 4: Loose 태그 추가 및 제거 검증 ==="));
    // 테스트에 사용할 태그 등록
    FGameplayTag TestTagA = AbilityTags::GameplayAbility_Passive_OnStartRoom;

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

    TSharedPtr<FTacticalEffectSpec> TagSpec = CompModel->MakeOutgoingSpec(UTestInfiniteTagEffect::StaticClass(), EffectContext3);
    FActiveTacticalEffectHandle TagActiveHandle = CompModel->ApplyTacticalEffectSpecToSelf(*TagSpec);

    TestTrue(TEXT("CompModel에 GrantedTag가 활성화되어야 합니다."), CompModel->HasMatchingGameplayTag(GrantedTag));
    TestEqual(TEXT("GrantedTag의 개수는 1이어야 합니다."), CompModel->GetTagCount(GrantedTag), 1);

    CompModel->RemoveActiveTacticalEffect(TagActiveHandle);
    TestFalse(TEXT("CompModel에서 GrantedTag가 제거되어야 합니다."), CompModel->HasMatchingGameplayTag(GrantedTag));

    // ----------------------------------------------------
    // 8. 환경 정리 (Clean up)
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
