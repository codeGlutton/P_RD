#include "TAS/Effect/TacticalEffectContext.h"
#include "Actor/ActorModel.h"

#include "TAS/TacticalAbility.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffectContext::UTacticalEffectContext(UActorModel* Instigator, UObject* EffectCauser)
{
	SetInstigator(Instigator);
	SetEffectCauser(EffectCauser);
}

void UTacticalEffectContext::SetInstigator(UActorModel* Instigator)
{
	mInstigator = MakeWeakObjectPtr<UActorModel>(Instigator);
}

void UTacticalEffectContext::SetEffectCauser(UObject* EffectCauser)
{
	mEffectCauser = MakeWeakObjectPtr<UObject>(EffectCauser);
}

void UTacticalEffectContext::SetSourceObject(const UObject* SourceObject)
{
	mSourceObject = MakeWeakObjectPtr(const_cast<UObject*>(SourceObject));
}

void UTacticalEffectContext::SetAbility(const UTacticalAbility* Ability)
{
	mAbilityInstance = MakeWeakObjectPtr<UTacticalAbility>(const_cast<UTacticalAbility*>(Ability));
}

void UTacticalEffectContext::SetAttributeSetComponentModel(const UAttributeSetComponentModel* Model)
{
	mAttributeSetCompModelInstance = MakeWeakObjectPtr<UAttributeSetComponentModel>(const_cast<UAttributeSetComponentModel*>(Model));
}
