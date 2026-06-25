#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Stat.h"
#include "TAS/Effect/Stat/TacticalEffectContext_Stat.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/BoardActor/BoardActorModel.h"

#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffectContext* UStaticSkillEffect_Stat::CreateContext(TWeakObjectPtr<class UBoardActorModel> CasterActor)
{
	UTacticalEffectContext_Stat* EffectContext = NewObject<UTacticalEffectContext_Stat>();

	TWeakObjectPtr<UAttributeSetComponentModel> AttributeSet = CasterActor.Get()->FindComponentModelByClass<UAttributeSetComponentModel>();
	checkf(AttributeSet.IsValid(), TEXT("컴포넌트가 없습니다."));

	//EffectContext->mTileLayerFlag = ETileLayerFlag::Unit;

	////EffectContext->mAttributeData = 
	//EffectContext->mBase = mEffectDefaultValue + mEffectRatioValue * AttributeSet->GetAttributeCurrentValue(UUnitAttributeSet::GetSkillPointAttribute());
	//EffectContext->mGameplayTag = mEffectTag;

	//EffectContext->mTacticalEffect = mTacticalEffect;


	return EffectContext;
}
