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

bool UUnitMovementComponentModel::IsMoveable() const
{
	// 이동불가 상태이상 태그 검사 자리 (태그 작업에서 구현)
	return true;
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
