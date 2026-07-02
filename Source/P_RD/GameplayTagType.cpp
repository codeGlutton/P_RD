#include "GameplayTagType.h"

namespace InputTags
{
}

namespace EventTags
{
}

namespace AbilityTags
{
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_LevelUp,								"GameplayAbility.LevelUp");

	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Skill,								"GameplayAbility.Skill");

	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartRoom,					"GameplayAbility.Passive.OnStartRoom");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndRoom,					"GameplayAbility.Passive.OnEndRoom");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartTurn,					"GameplayAbility.Passive.OnStartTurn");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndTurn,					"GameplayAbility.Passive.OnEndTurn");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartRollingDice,			"GameplayAbility.Passive.OnStartRollingDice");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndRollingDice,			"GameplayAbility.Passive.OnEndRollingDice");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartUsingSkill,			"GameplayAbility.Passive.OnStartUsingSkill");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndUsingSkill,				"GameplayAbility.Passive.OnEndUsingSkill");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartApplyingEffect,		"GameplayAbility.Passive.OnStartApplyingEffect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndApplyingEffect,			"GameplayAbility.Passive.OnEndApplyingEffect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartReceivingEffect,		"GameplayAbility.Passive.OnStartReceivingEffect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndReceivingEffect,		"GameplayAbility.Passive.OnEndReceivingEffect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartOneMove,				"GameplayAbility.Passive.OnStartOneMove");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndOneMove,				"GameplayAbility.Passive.OnEndOneMove");
}

namespace EffectTags
{
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_Cost,									"GameplayEffect.Cost");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_Cost_PassiveStack,					"GameplayEffect.Cost.PassiveStack");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_Cooldown,								"GameplayEffect.Cooldown");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_ActorState_Dead,						"GameplayEffect.ActorState.Dead");

	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect,							"GameplayEffect.StatusEffect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_Buff,					"GameplayEffect.StatusEffect.Buff");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_Debuff,					"GameplayEffect.StatusEffect.Debuff");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_Debuff_Weakness,			"GameplayEffect.StatusEffect.Debuff.Weakness");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_Debuff_Vulnerability,	"GameplayEffect.StatusEffect.Debuff.Vulnerability");
}

namespace CueTags
{
}
