#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Attack.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackPoint.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackFactor.h"
#include "TAS/Effect/Stat/TacticalEffect_HP.h"
#include "TAS/Effect/Stat/TacticalEffect_Defense.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "AttributeSet/UnitAttributeSet.h"

void FSkillEffectLayer_Attack::ClearPointEffect(IBoardCombatTarget* ActorModel) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    AttributeSetComponentModel->ApplyModToAttribute(UUnitAttributeSet::GetAttackPointAttribute(), ETacticalModOp::Override, 0.f);
}

void FSkillEffectLayer_Attack::ApplyPointEffect(IBoardCombatTarget* ActorModel, float DiceSum) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(ActorModel));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_AttackPoint::StaticClass(), EffectContext);
    EffectSpec->mDynamicMagnitude = mDefaultDamage + DiceSum * mDiceRatio;
    AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
}

void FSkillEffectLayer_Attack::CommitEffect(IBoardCombatTarget* OwnerActorModel, const TArray<FTileIndex>& TargetTileIndexes, const TArray<IBoardCombatTarget*>& OtherCombatTargets, float DiceSum) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = OwnerActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(OwnerActorModel));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    FActiveTacticalEffectHandle EffectHandle;

    /* 포인트를 Factor에 임시 추가 */
    {
        TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_AttackFactor::StaticClass(), EffectContext);
        EffectSpec->mDynamicMagnitude = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetAttackPointAttribute());
        EffectHandle = AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
    }

    // TODO : 약화 등의 패시브 효과 적용
    const float TotalAttack = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetAttackFactorAttribute());

    /* 데미지 적용 */
    for (const IBoardCombatTarget* OtherCombatTarget : OtherCombatTargets)
    {
        UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
        checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

        // [로컬 테스트] 데미지/방어 로그가 "맞는 쪽(타겟)"에 기록되도록, 타겟 대상 이펙트의 context instigator를 타겟으로 둔다.
        // TacticalEffect_HP/Defense는 instigator를 LogAttributeEffect의 actorID로만 쓰므로 데미지 수치엔 영향 없음.
        // (정식 수정은 로그가 instigator가 아닌 '영향받은 유닛'을 쓰도록 프레임워크에서 — 팀 확정 필요)
        UTacticalEffectContext* TargetEffectContext = AttributeSetComponentModel->MakeEffectContext();
        TargetEffectContext->SetInstigator(Cast<UActorModel>(const_cast<IBoardCombatTarget*>(OtherCombatTarget)));
        TargetEffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

        const float TotalDefense = OtherAttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefenseAttribute());

        /* 방어력 까기 */
        {
            const float DefenseDiff = -FMath::Min(TotalAttack, TotalDefense);
            if (DefenseDiff < 0)
            {
                TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_Defense::StaticClass(), TargetEffectContext);
                EffectSpec->mDynamicMagnitude = DefenseDiff;
                AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
            }
        }
        /* 체력 까기 */
        {
            const float HPDiff = TotalDefense - TotalAttack;
            if (HPDiff < 0)
            {
                TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_HP::StaticClass(), TargetEffectContext);
                EffectSpec->mDynamicMagnitude = HPDiff;
                AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
            }
        }
    }

    /* 포인트를 Factor에서 제거 */
    {
        AttributeSetComponentModel->RemoveActiveTacticalEffect(EffectHandle);
    }
}
