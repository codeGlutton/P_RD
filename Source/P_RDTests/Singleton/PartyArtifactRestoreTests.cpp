/*****************************************************************//**
 * @file   PartyArtifactRestoreTests.cpp
 * @brief  파티 아티팩트 복원 자동화 테스트
 * @details
 * 영구데이터로 파티를 복원할 때 보유 아티팩트가 구성원 전원에 장착되고
 * 패시브까지 설치되는지 검증 (실제 DA 사용).
 * @author 이문환
 * @date   2026-09-10
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#include "Singleton/PartyArtifactRestoreTestsHelper.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModelTestsHelper.h"
#include "Actor/Party/PartyModel.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"
#include "Component/ArtifactComponent/ArtifactComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"

namespace
{
	// 활성 PIE/Game 월드, 없으면 에디터 월드
	UWorld* GetRestoreTestWorld()
	{
		if (GEngine != nullptr)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World() != nullptr)
				{
					return Context.World();
				}
			}
		}
		return GWorld;
	}
}

// 복원 시 구성원 전원 장착과 패시브 설치 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPartyArtifactRestoreTests,
	"P_RD.Singleton.PartyArtifactRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPartyArtifactRestoreTests::RunTest(const FString& Parameters)
{
	/* 월드와 프레임워크 모델 확보 (파티 연결 시 속성 초기화에 필요) */
	UWorld* World = GetRestoreTestWorld();
	if (TestNotNull(TEXT("유효한 월드"), World) == false)
	{
		return false;
	}
	if (TestNotNull(TEXT("전략 프레임워크 모델"), GetWorldSubsystemModel<UTacticalFrameworkModel>(World)) == false)
	{
		return false;
	}

	/* 실제 DA: 기사 스폰 데이터, 패시브만 있는 아티팩트 */
	UStaticPlayerUnitSpawnData* KnightData = LoadObject<UStaticPlayerUnitSpawnData>(nullptr, TEXT("/Game/BP/DataAsset/Unit/PlayerUnit/DA_TestKnightPlayerUnit.DA_TestKnightPlayerUnit"));
	if (TestNotNull(TEXT("기사 스폰 DA"), KnightData) == false)
	{
		return false;
	}
	const FPrimaryAssetId ArtifactId(TEXT("Artifact"), TEXT("DA_Artifact_A001_ChaliceOfLife"));

	/* 파티와 구성원 2명 생성 (복원 경로처럼 월드 소속으로 초기화) */
	UPartyModel* PartyModel = NewObject<UPartyModel>(World);
	UMockPartyMemberModel* MemberA = NewObject<UMockPartyMemberModel>(World);
	UMockPartyMemberModel* MemberB = NewObject<UMockPartyMemberModel>(World);
	for (UMockPartyMemberModel* Member : { MemberA, MemberB })
	{
		Member->SetStaticSpawnData(KnightData);
		Member->Initialize();
	}
	PartyModel->Initialize();

	/* 영구데이터에 아티팩트 시드 후 동기화 (셋째 슬롯은 빈 자리) */
	UPartyArtifactRestoreTestData* PersistData = NewObject<UPartyArtifactRestoreTestData>();
	PersistData->SeedArtifact(ArtifactId);
	TArray<TObjectPtr<UPlayerUnitModel>> Players = { MemberA, MemberB, nullptr };
	PersistData->Sync(PartyModel, Players);

	/* 파티 목록 복원 */
	TestEqual(TEXT("파티 아티팩트 1개"), PartyModel->GetPartyArtifactComponentModel()->GetPartyArtifacts().Num(), 1);

	/* 구성원 전원 장착과 패시브 설치 */
	const FGameplayTag StartRoomTiming = FGameplayTag::RequestGameplayTag(FName(TEXT("GameplayAbility.Passive.OnStartRoom")));
	for (UMockPartyMemberModel* Member : { MemberA, MemberB })
	{
		TestEqual(TEXT("구성원 장착 1개"), Member->GetArtifactComponentModel()->GetArtifacts().Num(), 1);
		TestEqual(TEXT("구성원 패시브 1개"), Member->GetPassiveComponentModel()->GetPassives().Num(), 1);
		TestEqual(TEXT("OnStartRoom 패시브 1개"), Member->GetPassiveComponentModel()->GetPassivesByTiming(StartRoomTiming).Num(), 1);
	}

	/* 빈 슬롯은 그대로 */
	TestNull(TEXT("셋째 슬롯 비어 있음"), PartyModel->GetPlayerUnitModel(2));

	/* 정리 */
	PartyModel->Uninitialize();
	MemberA->Uninitialize();
	MemberB->Uninitialize();

	return true;
}
