/*****************************************************************//**
 * @file   EquipmentComponentModelTests.cpp
 * @brief  UEquipmentComponentModel 자동화 테스트
 * @details
 * 장비 장착 시 참조 패시브가 PassiveComponentModel에 설치되고,
 * 해제 시 제거되는지 검증 (자산 없이 코드로 DA 구성).
 * @author 이문환
 * @date   2026-06-28
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"

#include "Component/EquipmentComponent/EquipmentComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "DataAsset/EquipmentData/StaticWeaponEquipmentData.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"
#include "TAS/Passive/TacticalPassive.h"
#include "TAS/Passive/TacticalPassive_AddStat.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackPoint.h"

namespace
{
	// 발동 시점 태그를 이름으로 조회 (P_RD의 extern 태그 심볼은 export 안 되므로 모듈 경계용)
	FGameplayTag PassiveTiming(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName));
	}
}

// 장착 → 패시브 설치, 해제 → 제거 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEquipmentComponentModelTests,
	"P_RD.Component.EquipmentComponentModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FEquipmentComponentModelTests::RunTest(const FString& Parameters)
{
	const FGameplayTag StartTiming = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));

	// 컴포넌트 (오너 없이 독립 생성, 내부 시드로 형제 컴포넌트를 직접 주입)
	UEquipmentComponentModel* EquipComp = NewObject<UEquipmentComponentModel>();
	UPassiveComponentModel* PassiveComp = NewObject<UPassiveComponentModel>();
	if (!TestNotNull(TEXT("장비 컴포넌트 생성"), EquipComp) || !TestNotNull(TEXT("패시브 컴포넌트 생성"), PassiveComp))
	{
		return false;
	}

	// 패시브 DA (코드 구성): AddStat 패시브 + AttackPoint 이펙트 + 수치 5 + OnStartTurn 시점
	UStaticPassiveData* PassiveData = NewObject<UStaticPassiveData>();
	PassiveData->mPassiveClass = UTacticalPassive_AddStat::StaticClass();
	PassiveData->mEffectClass = UTacticalEffect_AttackPoint::StaticClass();
	PassiveData->mMagnitude = 5.f;
	PassiveData->mActivateTimingTag = StartTiming;

	// 무기 DA (코드 구성): 위 패시브를 참조
	UStaticWeaponEquipmentData* Weapon = NewObject<UStaticWeaponEquipmentData>();
	Weapon->mStaticPassiveData.Add(TSoftObjectPtr<UStaticPassiveData>(PassiveData));

	// 장착: 패시브가 설치돼야 함
	const bool bEquipped = EquipComp->EquipInternal(Weapon, PassiveComp, nullptr);
	TestTrue(TEXT("장착 성공"), bEquipped);
	TestEqual(TEXT("설치된 패시브 1개"), PassiveComp->GetPassives().Num(), 1);

	// 발동 시점 태그로 조회됨 + 정적 데이터가 주입됨
	TArray<UTacticalPassive*> StartList = PassiveComp->GetPassivesByTiming(StartTiming);
	TestEqual(TEXT("OnStartTurn 패시브 1개"), StartList.Num(), 1);
	if (StartList.Num() == 1)
	{
		TestEqual(TEXT("주입된 정적 데이터 일치"), StartList[0]->GetStaticData(), PassiveData);
	}

	// 무기 슬롯 점유 확인
	TestNotNull(TEXT("무기 슬롯 점유"), EquipComp->GetEquipped(EEquipmentType::Weapon));

	// 해제: 패시브가 제거돼야 함
	const bool bUnequipped = EquipComp->UnequipInternal(EEquipmentType::Weapon, PassiveComp, nullptr);
	TestTrue(TEXT("해제 성공"), bUnequipped);
	TestEqual(TEXT("해제 후 패시브 0개"), PassiveComp->GetPassives().Num(), 0);
	TestNull(TEXT("무기 슬롯 비움"), EquipComp->GetEquipped(EEquipmentType::Weapon));

	// 빈 슬롯 재해제는 실패
	TestFalse(TEXT("빈 슬롯 재해제 실패"), EquipComp->UnequipInternal(EEquipmentType::Weapon, PassiveComp, nullptr));

	return true;
}
