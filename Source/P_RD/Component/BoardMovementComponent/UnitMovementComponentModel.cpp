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

bool UUnitMovementComponentModel::CanSelfMove() const
{
	// 이동을 막는 상태이상 태그가 있으면 이동 불가
	UAttributeSetComponentModel* AttrComp = GetOwnerModel<UUnitModel>()->GetAttributeComponentModel();
	if (AttrComp == nullptr)
	{
		return true;
	}
	const bool IsNotRoot = AttrComp->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root) == false;
	const bool IsNotStun = AttrComp->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Stun) == false;

	return IsNotRoot && IsNotStun;
}

void UUnitMovementComponentModel::OnStartStep(int32 StepIndex, EBoardMoveMode MoveMode)
{
	Super::OnStartStep(StepIndex, MoveMode);

	// 밀려나는 이동은 행동력 미차감 (강제 이동은 자원 소모 없음)
	if (MoveMode != EBoardMoveMode::Normal)
	{
		return;
	}

	// 한 칸마다 이동력 차감
	if (UAttributeSetComponentModel* AttrComp = GetOwnerModel<UUnitModel>()->GetAttributeComponentModel())
	{
		AttrComp->ApplyModToAttribute(UUnitAttributeSet::GetActionPointAttribute(), ETacticalModOp::AddBase, -1.f);
	}
}
