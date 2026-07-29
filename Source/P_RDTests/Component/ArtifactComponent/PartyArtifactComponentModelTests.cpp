/*****************************************************************//**
 * @file   PartyArtifactComponentModelTests.cpp
 * @brief  UPartyArtifactComponentModel 자동화 테스트
 * @details
 * 아티펙트 획득/제거 시 파티 구성원 전원에 장착/해제가 배포되는지,
 * 신규 합류 일괄 장착과 저장용 ID 변환이 동작하는지 검증.
 * 변경 이벤트(OnChangeArtifact)가 성공한 획득/제거에만 통지되는지 검증.
 * @author 이문환
 * @date   2026-07-23
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "Component/ArtifactComponent/PartyArtifactComponentModelTestsHelper.h"
#include "Actor/Party/PartyModel.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"
#include "Component/ArtifactComponent/ArtifactComponentModel.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"

// 획득/제거 배포, 합류 일괄 장착, 저장용 ID 변환 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPartyArtifactComponentModelTests,
	"P_RD.Component.PartyArtifactComponentModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPartyArtifactComponentModelTests::RunTest(const FString& Parameters)
{
	/* 파티 모델 + 구성원 2명 생성 및 등록 */
	UPartyModel* PartyModel = NewObject<UPartyModel>();
	UPartyArtifactComponentModel* PartyArtifactModel = (PartyModel != nullptr) ? PartyModel->GetPartyArtifactComponentModel() : nullptr;
	UMockPartyMemberModel* MemberA = NewObject<UMockPartyMemberModel>();
	UMockPartyMemberModel* MemberB = NewObject<UMockPartyMemberModel>();

	const bool IsPartyModelValid = TestNotNull(TEXT("파티 모델 생성"), PartyModel);
	const bool IsPartyArtifactModelValid = TestNotNull(TEXT("파티 아티팩트 컴포넌트 모델 생성"), PartyArtifactModel);
	const bool IsMemberAValid = TestNotNull(TEXT("구성원 A 생성"), MemberA);
	const bool IsMemberBValid = TestNotNull(TEXT("구성원 B 생성"), MemberB);

	if ((IsPartyModelValid == false) || (IsPartyArtifactModelValid == false) || (IsMemberAValid == false) || (IsMemberBValid == false))
	{
		return false;
	}

	PartyModel->GetPlayerUnitModels()[0] = MemberA;
	PartyModel->GetPlayerUnitModels()[1] = MemberB;

	/* 변경 이벤트 계측: 성공한 획득/제거에만 최신 목록이 통지돼야 함 */
	int32 ChangeCount = 0;
	int32 LastArtifactNum = INDEX_NONE;
	PartyArtifactModel->OnChangeArtifact.AddLambda([&ChangeCount, &LastArtifactNum](const TArray<TObjectPtr<UStaticArtifactData>>& PartyArtifacts) {
		++ChangeCount;
		LastArtifactNum = PartyArtifacts.Num();
		});

	/* 아티펙트 DA (코드 구성): 배포 검증이 목적이라 패시브/스탯 없이 빈 DA 사용 */
	UStaticArtifactData* ArtifactA = NewObject<UStaticArtifactData>();
	UStaticArtifactData* ArtifactB = NewObject<UStaticArtifactData>();

	/* 획득: 파티 목록에 추가되고 구성원 전원에 장착돼야 함 */
	TestTrue(TEXT("아티펙트 A 획득 성공"), PartyArtifactModel->AddArtifact(ArtifactA));
	TestTrue(TEXT("아티펙트 B 획득 성공"), PartyArtifactModel->AddArtifact(ArtifactB));
	TestEqual(TEXT("파티 목록 2개"), PartyArtifactModel->GetPartyArtifacts().Num(), 2);
	TestEqual(TEXT("구성원 A 장착 2개"), MemberA->GetArtifactComponentModel()->GetArtifacts().Num(), 2);
	TestEqual(TEXT("구성원 B 장착 2개"), MemberB->GetArtifactComponentModel()->GetArtifacts().Num(), 2);
	TestEqual(TEXT("획득 이벤트 2회"), ChangeCount, 2);
	TestEqual(TEXT("획득 이벤트 페이로드 2개"), LastArtifactNum, 2);

	/* nullptr 획득은 실패하고 아무것도 변하지 않아야 함 */
	TestFalse(TEXT("nullptr 획득 실패"), PartyArtifactModel->AddArtifact(static_cast<UStaticArtifactData*>(nullptr)));
	TestEqual(TEXT("실패 후 파티 목록 그대로 2개"), PartyArtifactModel->GetPartyArtifacts().Num(), 2);
	TestEqual(TEXT("획득 실패 시 이벤트 없음"), ChangeCount, 2);

	/* 제거: 파티 목록에서 빠지고 구성원 전원에서 해제돼야 함 */
	TestTrue(TEXT("아티펙트 A 제거 성공"), PartyArtifactModel->RemoveArtifact(ArtifactA));
	TestEqual(TEXT("파티 목록 1개"), PartyArtifactModel->GetPartyArtifacts().Num(), 1);
	TestEqual(TEXT("구성원 A 장착 1개"), MemberA->GetArtifactComponentModel()->GetArtifacts().Num(), 1);
	TestEqual(TEXT("구성원 B 장착 1개"), MemberB->GetArtifactComponentModel()->GetArtifacts().Num(), 1);
	TestEqual(TEXT("제거 이벤트 3회"), ChangeCount, 3);
	TestEqual(TEXT("제거 이벤트 페이로드 1개"), LastArtifactNum, 1);

	/* 남은 것이 B인지 확인 (A를 지웠는데 B가 지워지면 안 됨) */
	TestNull(TEXT("구성원 A에 A 없음"), MemberA->GetArtifactComponentModel()->GetEquipped(ArtifactA));
	TestNotNull(TEXT("구성원 A에 B 유지"), MemberA->GetArtifactComponentModel()->GetEquipped(ArtifactB));

	/* 미보유 제거는 실패 */
	TestFalse(TEXT("미보유 A 재제거 실패"), PartyArtifactModel->RemoveArtifact(ArtifactA));
	TestEqual(TEXT("제거 실패 시 이벤트 없음"), ChangeCount, 3);

	/* 신규 합류: 보유 전체(현재 B 1개)가 일괄 장착돼야 함 */
	UMockPartyMemberModel* MemberC = NewObject<UMockPartyMemberModel>();
	PartyArtifactModel->EquipArtifactsTo(MemberC);
	TestEqual(TEXT("신규 구성원 장착 1개"), MemberC->GetArtifactComponentModel()->GetArtifacts().Num(), 1);
	TestNotNull(TEXT("신규 구성원에 B 장착"), MemberC->GetArtifactComponentModel()->GetEquipped(ArtifactB));
	TestEqual(TEXT("합류 배포는 이벤트 없음"), ChangeCount, 3);

	/* 저장용 ID 변환: 보유 개수와 일치하고 타입이 Artifact여야 함 */
	TArray<FPrimaryAssetId> ArtifactIds = PartyArtifactModel->GetPartyArtifactIds();
	TestEqual(TEXT("ID 목록 1개"), ArtifactIds.Num(), 1);
	if (ArtifactIds.Num() == 1)
	{
		TestEqual(TEXT("ID 타입 Artifact"), ArtifactIds[0].PrimaryAssetType.GetName().ToString(), FString(TEXT("Artifact")));
	}

	return true;
}
