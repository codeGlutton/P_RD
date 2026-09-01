/*****************************************************************//**
 * @file   ArtifactComponentModelTests.cpp
 * @brief  UArtifactComponentModel 자동화 테스트
 * @details
 * 아티펙트 장착 시 연관된 패시브가 PassiveComponentModel에 설치되고,
 * 해제 시 제거되는지 검증 (자산 없이 코드로 DA 구성).
 * @author 이문환
 * @date   2026-07-23
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"

#include "Component/ArtifactComponent/ArtifactComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"
#include "TAS/Passive/TacticalPassive.h"
#include "TAS/Passive/TacticalPassive_Generic.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackFactor.h"

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
	FArtifactComponentModelTests,
	"P_RD.Component.ArtifactComponentModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FArtifactComponentModelTests::RunTest(const FString& Parameters)
{
	const FGameplayTag StartTiming = PassiveTiming(TEXT("GameplayAbility.Passive.OnStartTurn"));

	// 필요한 컴포넌트만 유닛 없이 생성
	UArtifactComponentModel* ArtifactComp = NewObject<UArtifactComponentModel>();
	UPassiveComponentModel* PassiveComp = NewObject<UPassiveComponentModel>();
	if (!TestNotNull(TEXT("아티펙트 컴포넌트 생성"), ArtifactComp) || !TestNotNull(TEXT("패시브 컴포넌트 생성"), PassiveComp))
	{
		return false;
	}

	// 패시브 DA (코드 구성): 제네릭 패시브 + AttackFactor 이펙트 + 수치 5 + OnStartTurn 시점
	UStaticPassiveData* PassiveData = NewObject<UStaticPassiveData>();
	PassiveData->mPassiveClass = UTacticalPassive_Generic::StaticClass();
	FPassiveEffectEntry& Effect = PassiveData->mEffects.AddDefaulted_GetRef();
	Effect.mEffectClass = UTacticalEffect_AttackFactor_AddBase::StaticClass();
	Effect.mMagnitude.mConst = 5.f;
	PassiveData->mActivateTimingTag = StartTiming;

	// 아티펙트 DA (코드 구성): 위 패시브를 연관
	UStaticArtifactData* Artifact = NewObject<UStaticArtifactData>();
	Artifact->mStaticPassiveData.Add(TSoftObjectPtr<UStaticPassiveData>(PassiveData));

	// 장착: 패시브가 설치돼야 함
	const bool bEquipped = ArtifactComp->EquipInternal(Artifact, PassiveComp, nullptr);
	TestTrue(TEXT("장착 성공"), bEquipped);
	TestEqual(TEXT("설치된 패시브 1개"), PassiveComp->GetPassives().Num(), 1);

	// 발동 시점 태그로 조회됨 + 정적 데이터가 주입됨
	TArray<UTacticalPassive*> StartList = PassiveComp->GetPassivesByTiming(StartTiming);
	TestEqual(TEXT("OnStartTurn 패시브 1개"), StartList.Num(), 1);
	if (StartList.Num() == 1)
	{
		TestEqual(TEXT("주입된 정적 데이터 일치"), StartList[0]->GetStaticData(), PassiveData);
	}

	// 장착 조회 확인
	TestNotNull(TEXT("장착 엔트리 조회"), ArtifactComp->GetEquipped(Artifact));
	TestEqual(TEXT("장착 목록 1개"), ArtifactComp->GetArtifacts().Num(), 1);

	// 중복 장착: 같은 아티펙트를 한 번 더 장착하면 엔트리와 패시브가 2개
	TestTrue(TEXT("중복 장착 성공"), ArtifactComp->EquipInternal(Artifact, PassiveComp, nullptr));
	TestEqual(TEXT("장착 목록 2개"), ArtifactComp->GetArtifacts().Num(), 2);
	TestEqual(TEXT("설치된 패시브 2개"), PassiveComp->GetPassives().Num(), 2);

	// 해제: 첫 엔트리만 해제돼야 함 (중복 장착분 하나 남음)
	TestTrue(TEXT("해제 성공"), ArtifactComp->UnequipInternal(Artifact, PassiveComp, nullptr));
	TestEqual(TEXT("해제 후 장착 목록 1개"), ArtifactComp->GetArtifacts().Num(), 1);
	TestEqual(TEXT("해제 후 패시브 1개"), PassiveComp->GetPassives().Num(), 1);

	// 남은 엔트리 해제 후 빈 상태 재해제는 실패
	TestTrue(TEXT("남은 엔트리 해제 성공"), ArtifactComp->UnequipInternal(Artifact, PassiveComp, nullptr));
	TestEqual(TEXT("전부 해제 후 패시브 0개"), PassiveComp->GetPassives().Num(), 0);
	TestNull(TEXT("전부 해제 후 조회 없음"), ArtifactComp->GetEquipped(Artifact));
	TestFalse(TEXT("빈 상태 재해제 실패"), ArtifactComp->UnequipInternal(Artifact, PassiveComp, nullptr));

	return true;
}
