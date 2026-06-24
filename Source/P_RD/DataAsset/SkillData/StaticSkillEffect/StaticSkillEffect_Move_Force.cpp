#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Move_Force.h"
#include "TAS/Effect/Move/TacticalEffectContext_Move_Force.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/BoardActor/BoardActorModel.h"

#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffectContext* UStaticSkillEffect_Move_Force::CreateContext(TWeakObjectPtr<class UBoardActorModel> CasterActor)
{
	UTacticalEffectContext_Move_Force* EffectContext = NewObject<UTacticalEffectContext_Move_Force>();

	TWeakObjectPtr<UAttributeSetComponentModel> AttributeSet = CasterActor.Get()->FindComponentModelByClass<UAttributeSetComponentModel>();
	checkf(AttributeSet.IsValid(), TEXT("컴포넌트가 없습니다."));

	EffectContext->mTileLayerFlag = ETileLayerFlag::Unit;

	EffectContext->mBase = mEffectDefaultDistance + mEffectRatioDistance * AttributeSet->GetAttributeCurrentValue(UUnitAttributeSet::GetSkillPointAttribute());
	EffectContext->mGameplayTag = mEffectTag;

	EffectContext->mTacticalEffect = mTacticalEffect;

	return EffectContext;
}
