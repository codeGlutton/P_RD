/*****************************************************************//**
 * @file   UnitMovementComponentModel.cpp
 * @brief  유닛 전용 이동 컴포넌트 모델 구현 파일
 * @author 이문환
 * @date   2026-08-10
 *********************************************************************/

#include "Component/BoardMovementComponent/UnitMovementComponentModel.h"

#include "Pawn/UnitModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "GameplayTagType.h"

bool UUnitMovementComponentModel::IsMoveable() const
{
	// 속박 태그 보유 중엔 이동 불가 (저장값 없이 매번 태그에서 계산)
	UAttributeSetComponentModel* AttrComp = GetOwnerModel<UUnitModel>()->GetAttributeComponentModel();
	if (AttrComp == nullptr)
	{
		return true;
	}
	return AttrComp->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Root) == false;
}

void UUnitMovementComponentModel::OnStartStep(int32 StepIndex)
{
	Super::OnStartStep(StepIndex);

	// 한 칸마다 이동력 차감
	if (UAttributeSetComponentModel* AttrComp = GetOwnerModel<UUnitModel>()->GetAttributeComponentModel())
	{
		AttrComp->ApplyModToAttribute(UUnitAttributeSet::GetActionPointAttribute(), ETacticalModOp::AddBase, -1.f);
	}
}
