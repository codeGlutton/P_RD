/*****************************************************************//**
 * @file   PassiveTestsHelper.h
 * @brief  패시브 발동/해제 경로 테스트용 공통 목업
 * @author 이문환
 * @date   2026-06-30
 *
 * @details
 *  패시브의 실제 적용 경로(NotifyPassive→ApplyTacticalEffect...)는 owner/target이
 *  IBoardCombatTarget이어야 하고 UTacticalFrameworkModel(월드 서브시스템)을 요구.
 *  그래서 모호재님의 UTASActorModelMock과 동일 구성(실제 속성 컴포넌트 + 유닛 속성셋 보유)으로
 *  보드액터 목을 만들되, 베이스만 UBoardActorModel + IBoardCombatTarget으로 설정.
 *  테스트는 모호재님의 패턴대로 월드/룸/프레임워크를 세팅한 뒤 실제 적용 결과(속성 변화)를 검증.
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "PassiveTestsHelper.generated.h"

/**
 * @brief 패시브 테스트용 UBoardActorModel 목 (IBoardCombatTarget 구현 + 실제 속성 컴포넌트 보유)
 *
 * @details
 *  NotifyPassive가 캐스팅할 수 있도록 베이스가 보드 액터 + 전투 타겟.
 */
UCLASS()
class UMockBoardActorModel : public UBoardActorModel, public IBoardCombatTarget
{
	GENERATED_BODY()

public:
	UMockBoardActorModel()
	{
		// 실제 속성 컴포넌트 + 유닛 속성셋을 기본 서브오브젝트로 생성
		mAttributeComponentModel = CreateDefaultSubobject<UAttributeSetComponentModel>(TEXT("AttributeSetComponentModel"));
		mUnitAttributeSet = CreateDefaultSubobject<UUnitAttributeSet>(TEXT("UnitAttributeSet"));
	}

	// 테스트 목이므로 컴포넌트 모델 초기화 단계는 비움
	virtual void PreInitializeComponentModels() override {}
	virtual void PostInitializeComponentModels() override {}

	// IBoardCombatTarget 구현
	virtual UAttributeSetComponentModel* GetAttributeComponentModel() const override { return mAttributeComponentModel; }
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override {}
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId::NoTeam; }

private:
	UPROPERTY()
	TObjectPtr<UAttributeSetComponentModel> mAttributeComponentModel;

	UPROPERTY()
	TObjectPtr<UUnitAttributeSet> mUnitAttributeSet;
};
