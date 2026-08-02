/*****************************************************************//**
 * @file   EquipmentStatModifierTests.cpp
 * @brief  UEquipmentComponentModel 장비 고유 스탯 적용 자동화 테스트
 * @details
 * 장비의 mStatModifiers가 장착 시 무한 이펙트로 속성에 적용되고,
 * 해제 시 원복되는지 검증 (라이브 월드 + 속성셋 모크 사용).
 * @author 이문환
 * @date   2026-06-28
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "TAS/TASAttributeTestsHelper.h"
#include "Component/EquipmentComponent/EquipmentComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/TacticalEffect.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "Simulation/RoomInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// 활성 PIE/Game 월드 탐색 (없으면 호출측이 GWorld로 폴백)
	UWorld* FindAnyGameWorld()
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
}

// 장비 스탯(mStatModifiers) 장착 시 적용, 해제 시 원복 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEquipmentStatModifierTests,
	"P_RD.Component.EquipmentStatModifier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FEquipmentStatModifierTests::RunTest(const FString& Parameters)
{
	// 라이브 월드 확보 (TAS 속성 테스트와 동일 환경)
	UWorld* World = FindAnyGameWorld();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 월드"), World) == false)
	{
		return false;
	}

	// 시뮬레이션 서브시스템 + 룸 인스턴스 확보
	USimulationSubsystem* SimSubsystem = World->GetSubsystem<USimulationSubsystem>();
	if (TestNotNull(TEXT("SimulationSubsystem"), SimSubsystem) == false)
	{
		return false;
	}
	URoomInstance* RoomInstance = FSimulationSubsystemTestAccessor::GetRoomInstance(SimSubsystem);
	if (TestNotNull(TEXT("RoomInstance"), RoomInstance) == false)
	{
		return false;
	}

	// 전략 프레임워크 모델 등록 (없을 때만 모크 등록 — 이펙트 컨텍스트/적용에 필요)
	UTacticalFrameworkModel* FrameworkModel = Cast<UTacticalFrameworkModel>(RoomInstance->mAliveSubsystemModels.FindRef(UTacticalFrameworkModel::StaticClass()));
	if (FrameworkModel == nullptr)
	{
		FrameworkModel = NewObject<UTacticalFrameworkModel>(RoomInstance);
		RoomInstance->mAliveSubsystemModels.Add(UTacticalFrameworkModel::StaticClass(), FrameworkModel);
	}

	// 속성 컴포넌트 모크 (UUnitAttributeSet 부착)
	UTASActorModelMock* MockActorModel = NewObject<UTASActorModelMock>(World);
	MockActorModel->Initialize();
	MockActorModel->BeginPlay();
	UAttributeSetComponentModel* AttrComp = MockActorModel->GetAttributeComponent();
	if (TestNotNull(TEXT("AttributeSetComponentModel"), AttrComp) == false)
	{
		return false;
	}

	// 공격력 기본값 10 세팅 (현재값도 10으로 전파됨)
	const FTacticalAttribute AttackPoint = UCombatTargetAttributeSet::GetAttackFactorAttribute();
	AttrComp->SetAttributeBaseValue(AttackPoint, 10.f);
	TestEqual(TEXT("기본 공격력 10"), AttrComp->GetAttributeCurrentValue(AttackPoint), 10.f);

	// 무기 DA (코드 구성): 공격력 +5 고유 스탯
	UStaticEquipmentData* Weapon = NewObject<UStaticEquipmentData>();
	FTacticalModifierInfo Mod;
	Mod.mAttribute = AttackPoint;
	Mod.mModifierOp = ETacticalModOp::AddBase;
	Mod.mModifierMagnitude = 5.f;
	Weapon->mStatModifiers.Add(Mod);

	// 장착: 스탯 이펙트가 자신에게 적용 (패시브 컴포넌트는 불필요 → nullptr)
	UEquipmentComponentModel* EquipComp = NewObject<UEquipmentComponentModel>();
	const bool bEquipped = EquipComp->EquipInternal(Weapon, nullptr, AttrComp);
	TestTrue(TEXT("장착 성공"), bEquipped);
	TestEqual(TEXT("장착 후 공격력 15 (기본 10 + 스탯 5)"), AttrComp->GetAttributeCurrentValue(AttackPoint), 15.f);

	// 스탯 이펙트 핸들이 유효해야 함
	const FEquippedEntry* Entry = EquipComp->GetEquipped(EEquipmentType::Weapon);
	if (TestNotNull(TEXT("무기 슬롯 점유"), Entry))
	{
		TestTrue(TEXT("스탯 이펙트 핸들 유효"), Entry->mStatEffectHandle.IsValid());
	}

	// 해제: 스탯이 원복돼야 함
	const bool bUnequipped = EquipComp->UnequipInternal(EEquipmentType::Weapon, nullptr, AttrComp);
	TestTrue(TEXT("해제 성공"), bUnequipped);
	TestEqual(TEXT("해제 후 공격력 10 원복"), AttrComp->GetAttributeCurrentValue(AttackPoint), 10.f);

	return true;
}
