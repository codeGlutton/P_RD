/*****************************************************************//**
 * @file   TacticalPassive.cpp
 * @brief  패시브 베이스 클래스 구현
 * @author 이문환
 * @date   2026-06-25
 *********************************************************************/

#include "TAS/Passive/TacticalPassive.h"

#include "TAS/Passive/PassiveActivateContext.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

void UTacticalPassive::ActivatePassive(
	const FPassiveActivateContext& Ctx,
	TInstancedStruct<FTacticalPassiveState>& PassiveState)
{
	// 계산 결과를 담을 내부 중간 버퍼 (외부로 반환하지 않음)
	FBoardCombatTargetSnapshotData Contribution;

	// 계산: 기여값 산출 + 러닝 상태 갱신 (실제 내부 상태는 여기서 안 바꿈)
	EvaluatePassive(Ctx, Contribution, PassiveState);
	// 적용: 계산된 기여값을 대상 이펙트로 적용
	NotifyPassive(Ctx, Contribution);
}

void UTacticalPassive::NotifyPassive(
	const FPassiveActivateContext& Ctx,
	FBoardCombatTargetSnapshotData& TargetDelta)
{
	// 적용 대상은 이번 계산 대상(Target). 자기버프면 mTarget == mOwner라 동일하게 처리됨.
	UBoardActorModel* TargetModel = Ctx.mTarget.Get();
	IBoardCombatTarget* CombatTarget = Cast<IBoardCombatTarget>(TargetModel);
	if (CombatTarget == nullptr)
	{
		// 대상이 타격 가능 모델이 아니면(또는 대상 미지정) 적용 없이 TargetDelta 유지
		return;
	}

	// 대상에서 속성 컴포넌트 획득
	UAttributeSetComponentModel* AttributeComp = CombatTarget->GetAttributeComponentModel();
	if (AttributeComp == nullptr)
	{
		return;
	}

	// 계산된 속성 변화량을 대상 aggregator에 mod로 꽂는다.
	// TargetDelta.mAttributes는 속성당 "가산 총합"(op 정보 없음)이라 EGameplayModOp::Additive로 적용.
	for (const TPair<FGameplayAttribute, float>& Pair : TargetDelta.mAttributes)
	{
		// TODO:
		// ApplyModToAttribute가 void라 롤백용 핸들 회수 불가.
		// DeactivatePassive를 위해선 핸들 반환 API(ApplyGameplayEffectSpec 공개 등) 필요.
		// AttributeComp->ApplyModToAttribute(Pair.Key, EGameplayModOp::Additive, Pair.Value);
	}

	// TODO: TargetDelta.mTags(발동 여부형 변화) 적용 경로는 미정. 정해지면 추가.
}
