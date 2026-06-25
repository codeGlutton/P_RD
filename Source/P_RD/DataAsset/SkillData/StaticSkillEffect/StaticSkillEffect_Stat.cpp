#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Stat.h"
#include "TAS/Effect/Stat/TacticalEffectContext_Stat.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#include "AttributeSet/UnitAttributeSet.h"


bool UStaticSkillEffect_Stat::CreateBaseEffectContainer(TWeakObjectPtr<class UBoardActorModel> CasterActor, OUT FBoardCombatTargetSnapshotData& Container)
{
	TWeakObjectPtr<UAttributeSetComponentModel> AttributeSet = CasterActor.Get()->FindComponentModelByClass<UAttributeSetComponentModel>();
	checkf(AttributeSet.IsValid(), TEXT("컴포넌트가 없습니다."));

	if (AttributeSet.IsValid())
	{
		//Container.mTags.Add(mEffectTag, mEffectDefaultValue + mEffectRatioValue * AttributeSet->GetAttributeCurrentValue(UUnitAttributeSet::GetSkillPointAttribute()));
	}
	else
	{
		Container.mTags.Add(mEffectTag, mEffectDefaultValue);
	}


	return true;
}

float UStaticSkillEffect_Stat::GetPoint(TWeakObjectPtr<class UBoardActorModel> CasterActor, float SkillPoint)
{
	TWeakObjectPtr<UAttributeSetComponentModel> AttributeSet = CasterActor.Get()->FindComponentModelByClass<UAttributeSetComponentModel>();
	checkf(AttributeSet.IsValid(), TEXT("컴포넌트가 없습니다."));

	if (AttributeSet.IsValid())
	{
		return mEffectDefaultValue + mEffectRatioValue * SkillPoint;
	}

	return mEffectDefaultValue;

}
