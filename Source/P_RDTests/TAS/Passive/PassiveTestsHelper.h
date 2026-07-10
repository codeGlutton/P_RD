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
#include "TAS/Passive/TacticalPassive_AddStat.h"
#include "TAS/Effect/TacticalEffect.h"
#include "GameplayTagType.h"
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

/**
 * @brief 태그 지속형(발동/해제) 테스트용 목 이펙트
 *
 * @details
 *  Infinite + GrantedTags 방식: 활성 리스트에 등록되어 핸들이 남고,
 *  핸들 제거 시 부여 태그(취약)도 함께 회수됨.
 *  프로덕션에는 아직 이런 이펙트가 없어서 패시브의 해제 계약 검증용으로만 사용.
 */
UCLASS()
class UMockGrantedTagTacticalEffect : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UMockGrantedTagTacticalEffect()
	{
		// 지속형: 활성 리스트 등록 → 핸들로 해제 가능
		mDurationPolicy = ETacticalEffectDurationType::Infinite;
		mStackingType = ETacticalEffectStackingType::None;

		// 활성 중 부여 태그: 취약 (모디파이어 없음 = 순수 태그형)
		mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Vulnerability);
	}
};

/**
 * @brief 수량 조건 테스트용 목 패시브 (AddStat + HP 자격 조건)
 *
 * @details
 *  자격 조건에 체력 조건 판정을 구현한 목 패시브
 */
UCLASS()
class UMockConditionAddStatPassive : public UTacticalPassive_AddStat
{
	GENERATED_BODY()

public:
	// 이 값 미만 HP면 자격 있음!
	float mQualifyHPBelow = 50.f;

protected:
	virtual bool IsTargetQualified(const FBoardCombatTargetSnapshotData* Snapshot) const override
	{
		if (Snapshot == nullptr)
		{
			return false;
		}
		const float* HP = Snapshot->mAttributes.Find(UUnitAttributeSet::GetHPAttribute());
		return (HP != nullptr) && (*HP < mQualifyHPBelow);
	}
};
