#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Move_Force.h"
#include "TAS/Effect/Move/TacticalEffectContext_Move_Force.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#include "AttributeSet/UnitAttributeSet.h"



bool UStaticSkillEffect_Move_Force::CreateBaseEffectContainer(TWeakObjectPtr<class UBoardActorModel> CasterActor, OUT FBoardCombatTargetSnapshotData& Container)
{
	TWeakObjectPtr<UAttributeSetComponentModel> AttributeSet = CasterActor.Get()->FindComponentModelByClass<UAttributeSetComponentModel>();
	checkf(AttributeSet.IsValid(), TEXT("컴포넌트가 없습니다."));

	if (AttributeSet.IsValid())
	{
		//Container.mTags.Add(mEffectTag, mEffectDefaultDistance + mEffectRatioDistance * AttributeSet->GetAttributeCurrentValue(UUnitAttributeSet::GetSkillPointAttribute()));
	}
	else
	{
		Container.mTags.Add(mEffectTag, mEffectDefaultDistance);
	}

	return true;
}

float UStaticSkillEffect_Move_Force::GetPoint(TWeakObjectPtr<class UBoardActorModel> CasterActor, float SkillPoint)
{
	TWeakObjectPtr<UAttributeSetComponentModel> AttributeSet = CasterActor.Get()->FindComponentModelByClass<UAttributeSetComponentModel>();
	checkf(AttributeSet.IsValid(), TEXT("컴포넌트가 없습니다."));

	if (AttributeSet.IsValid())
	{
		return mEffectDefaultDistance + mEffectRatioDistance * SkillPoint;
	}

	return mEffectDefaultDistance;
}
