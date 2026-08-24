#include "TAS/Effect/Tag/TacticalEffect_Acumeny.h"
#include "GameplayTagType.h"

UTacticalEffect_Buff_Acumeny::UTacticalEffect_Buff_Acumeny()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Acumeny);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Acumeny);
}

UTacticalEffect_AddBuff_Acumeny::UTacticalEffect_AddBuff_Acumeny()
{
	mStatusEffect = UTacticalEffect_Buff_Acumeny::StaticClass();
}

UTacticalEffect_GetBuff_Acumeny::UTacticalEffect_GetBuff_Acumeny()
{
	mStatusEffect = UTacticalEffect_Buff_Acumeny::StaticClass();
}

UTacticalEffect_Debuff_Acumeny::UTacticalEffect_Debuff_Acumeny()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Debuff_Acumeny);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_Infinite_Debuff_Acumeny);
}

UTacticalEffect_AddDebuff_Acumeny::UTacticalEffect_AddDebuff_Acumeny()
{
	mStatusEffect = UTacticalEffect_Debuff_Acumeny::StaticClass();
}

UTacticalEffect_GetDebuff_Acumeny::UTacticalEffect_GetDebuff_Acumeny()
{
	mStatusEffect = UTacticalEffect_Debuff_Acumeny::StaticClass();
}
