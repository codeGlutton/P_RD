/*****************************************************************//**
 * @file   SkillTests.cpp
 * @brief  스킬 작동 테스트
 * @details
 * 스킬의 작동이 정상적으로 작동하는 지 테스트하기 위한 cpp 파일
 * @author 김준형
 * @date   2026-06-26
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "SkillTestsHelper.h"
#include "Pawn/Test/SkillTestUnitModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "DataAsset/SkillData/StaticAttackSkillData.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Simulation/RoomInstance.h"
#include "GameplayTagsManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

 // TAS 스킬 작동 확인
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSkillTest,
    "P_RD.Skill.ActivateSkill",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

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

/**
 * @brief 샘플 테스트 실행 함수
 * @details
 * TestTrue, TestEqual 등의 검증 매크로로 조건을 확인한다.
 * false 반환 시 테스트 실패로 처리된다.
 */
bool FSkillTest::RunTest(const FString& Parameters)
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

    // =======================================================================
    // 목업 데이터 생성
    // =======================================================================
    
    // =============================================================================
    // 액터 모델 생성
    // ============================================================================ =
    TObjectPtr<USkillTestUnitModel> SkillActorModel = NewObject<USkillTestUnitModel>(World);
    SkillActorModel->Initialize();
    SkillActorModel->BeginPlay();
    
    UAttributeSetComponentModel* ASCompModel = SkillActorModel->GetAttributeComponentModel();
    if (!TestNotNull(TEXT("AttributeSetComponentModel should be valid"), ASCompModel))
    {
        return false;
    }
    
    // AttributeSet이 정상 부착되었는지 확인
    TestTrue(TEXT("UUnitAttributeSet should be registered"), ASCompModel->HasAttributeSetForAttribute(UUnitAttributeSet::GetHPAttribute()));
    
    // 스킬 모델이 정상 부착되었는지 확인
    USkillComponentModel* SkillComp = SkillActorModel->GetSkillComponentModel();
    if (!TestNotNull(TEXT("SkillComponentModel should be valid"), SkillComp))
    {
        return false;
    }

    AddInfo(TEXT("=== 스킬 컴포넌트 등록 완료 ==="));

    
    // =============================================================================
    // 스킬 데이터 로드
    // 스킬 데이터를 먼저 삽입해야 합니다.
    // 스킬 데이터 변경 필요 시 Game/BP/DataAsset/Skill/Attack/DA_TestAttack_Common
    // =============================================================================
    {
        FString AssetPath = TEXT("/Game/BP/DataAsset/Skill/Attack/DA_TestAttack_Common.DA_TestAttack_Common");
        UStaticSkillData* LoadedData = Cast<UStaticSkillData>(StaticLoadObject(UStaticSkillData::StaticClass(), nullptr, *AssetPath));
    
        if (!TestNotNull(TEXT("SkillComponentModel should be valid"), LoadedData))
        {
            return false;
        }
        SkillComp->AddSkillData(LoadedData);
    
        TSoftObjectPtr<UStaticSkillData> SoftLoadedData;
        SoftLoadedData.LoadSynchronous();
        SkillComp->GetSkillData(0, SoftLoadedData);
        if (!TestTrue(TEXT("스킬이 유효합니다."), SoftLoadedData.IsValid()))
        {
            return false;
        }
        if (!TestTrue(TEXT("스킬이 로드되었습니다."), !SoftLoadedData.IsPending()))
        {
            return false;
        }
    }
    AddInfo(TEXT("=== 스킬 로드 완료 ==="));


    // ==========================
    // 어트리뷰트셋에 HP 등록
    // 스킬 시전자의 스탯을 설정합니다.
    // ===========================
    ASCompModel->SetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute(), 100.f);
    ASCompModel->SetAttributeBaseValue(UUnitAttributeSet::GetHPAttribute(), 100.f);
    ASCompModel->SetAttributeBaseValue(UUnitAttributeSet::GetDamagePointAttribute(), 10.f);

    // =============================================================================
    // 스킬 시전
    // Tiles에 스킬을 효과를 줍니다.
    // 현재 SubSystem에서 TileMapModel에 어려움을 겪고 있어 스킬 컴포넌트에서 자체적으로 유닛을 생성합니다.
    // 해당 유닛의 스탯
    // (MaxHP : 100);
    // (HP : 100);
    // (DamagePoint : 10);
    // (DefensePoint :5);
    // 유닛의 상태가 변경 필요 시(USkillComponentModel::ExtractTarget)에서 변경하면 됩니다.
    // =============================================================================

    TArray<FTileIndex> Tiles;
    Tiles.Add(FTileIndex(0, 0));
    SkillComp->ActivateSkill(0, Tiles, 10);

    return true;
}
