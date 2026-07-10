/*****************************************************************//**
 * @file   TacticalSimulationTests.cpp
 * @brief  시뮬레이션 복제 환경에서의 이펙트 적용 및 제거 자동화 테스트
 * @details
 * Infinite Effect가 적용된 상태에서 RoomInstance를 복제하고, 
 * 복제본에서 이펙트를 제거했을 때 원본에는 영향이 없는지 검증한다.
 * @author 모호재
 * @date   2026-06-27
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
#include "Simulation/Factory/ObjectModelFactory.h"
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
    FTacticalSimulationTests,
    "P_RD.TAS.Simulation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FTacticalSimulationTests::RunTest(const FString& Parameters)
{
    // 1. 월드 및 서브시스템 획득
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

    // 2. 모델 팩토리를 통해 액터 모델 생성 (RoomInstance에 소속되어 복제 대상이 됨)
    UTASActorModelMock* MockActorModel = SimSubsystem->GetModelFactory().NewModel<UTASActorModelMock>();
    if (TestNotNull(TEXT("MockActorModel이 팩토리를 통해 생성되어야 합니다."), MockActorModel) == false)
    {
        return false;
    }

    UAttributeSetComponentModel* CompModel = MockActorModel->GetAttributeComponent();
    if (TestNotNull(TEXT("AttributeSetComponentModel이 유효해야 합니다."), CompModel) == false)
    {
        return false;
    }

    // 기본 방어력 세팅
    CompModel->SetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute(), 10.f);
    TestEqual(TEXT("초기 기본 방어력은 10.f이어야 합니다."), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute()), 10.f);
    TestEqual(TEXT("초기 현재 방어력은 10.f이어야 합니다."), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute()), 10.f);

    // 3. 무한 이펙트 적용 (방어력 +30)
    UTacticalEffectContext* EffectContext = CompModel->MakeEffectContext();
    EffectContext->SetInstigator(MockActorModel);
    EffectContext->SetAttributeSetComponentModel(CompModel);

    TSharedPtr<FTacticalEffectSpec> InfiniteEffect = CompModel->MakeOutgoingSpec(UTestInfiniteTacticalEffect::StaticClass(), EffectContext);
    InfiniteEffect->mDynamicMagnitude = 30.f;
    FActiveTacticalEffectHandle ActiveHandle = CompModel->ApplyTacticalEffectSpecToSelf(*InfiniteEffect);

    TestTrue(TEXT("ActiveHandle이 유효해야 합니다."), ActiveHandle.IsValid());
    TestEqual(TEXT("기본 방어력은 10.f로 유지되어야 합니다."), CompModel->GetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute()), 10.f);
    TestEqual(TEXT("현재 방어력은 40.f이어야 합니다. (기본 10 + 모디파이어 30)"), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute()), 40.f);

    // 4. 시뮬레이션 상태로 변경하여 복제 유도
    AddInfo(TEXT("=== 시뮬레이션 상태로 전환 (RoomInstance 복제) ==="));
    FSimulationSubsystemTestAccessor::SetSimState(SimSubsystem, ESRPGSimulationState::RunningSimulation);

    // 복제된 룸 인스턴스 획득
    URoomInstance* SimRoomInstance = FSimulationSubsystemTestAccessor::GetRoomInstance(SimSubsystem);
    if (TestNotNull(TEXT("SimRoomInstance가 유효해야 합니다."), SimRoomInstance) == false)
    {
        return false;
    }
    TestTrue(TEXT("SimRoomInstance는 원본 RoomInstance와 다른 인스턴스여야 합니다."), SimRoomInstance != RoomInstance);

    // 복제된 액터 모델 검색
    int32 ModelId = MockActorModel->GetModelId();
    UTASActorModelMock* ClonedActorModel = Cast<UTASActorModelMock>(SimRoomInstance->mAliveWorldModels.FindRef(ModelId));
    if (TestNotNull(TEXT("SimRoomInstance 내에 ClonedActorModel이 존재해야 합니다."), ClonedActorModel) == false)
    {
        return false;
    }
    TestTrue(TEXT("ClonedActorModel은 원본과 다른 객체여야 합니다."), ClonedActorModel != MockActorModel);

    UAttributeSetComponentModel* ClonedCompModel = ClonedActorModel->GetAttributeComponent();
    if (TestNotNull(TEXT("ClonedCompModel이 유효해야 합니다."), ClonedCompModel) == false)
    {
        return false;
    }

    // 복제 직후 수치 일치 여부 확인
    TestEqual(TEXT("[복제본] 기본 방어력은 10.f이어야 합니다."), ClonedCompModel->GetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute()), 10.f);
    TestEqual(TEXT("[복제본] 현재 방어력은 40.f이어야 합니다."), ClonedCompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute()), 40.f);

    // 5. 복제본에서 이펙트 해제 테스트
    AddInfo(TEXT("=== 복제된 모델에서 활성 이펙트 제거 ==="));
    bool Removed = ClonedCompModel->RemoveActiveTacticalEffect(ActiveHandle);
    TestTrue(TEXT("복제된 컴포넌트에서 이펙트가 성공적으로 제거되어야 합니다."), Removed);

    // 6. 결과 검증 (격리 여부 확인)
    // 복제본은 방어력이 원래 수치(10)로 복구되어야 함
    TestEqual(TEXT("[복제본] 이펙트 제거 후, 방어력이 10.f로 복구되어야 합니다."), ClonedCompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute()), 10.f);
    
    // 원본은 여전히 방어력 버프가 유지되어 40이어야 함
    TestEqual(TEXT("[원본] 방어력이 40.f로 유지되어야 합니다."), CompModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute()), 40.f);

    // 7. 환경 정리
    AddInfo(TEXT("=== 게임 상태 복원 및 환경 정리 ==="));
    FSimulationSubsystemTestAccessor::SetSimState(SimSubsystem, ESRPGSimulationState::RunningGame);

    SimSubsystem->GetModelFactory().DestroyModel(MockActorModel);

    return true;
}
