#include "GameplayTagType.h"

namespace InputTags
{
}

namespace AnimationTags
{
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_GetBuff,							"Animation.Montage.GetBuff");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_GetDebuff,							"Animation.Montage.GetDebuff");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_GetDefense,						"Animation.Montage.GetDefense");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_GetMovement,						"Animation.Montage.GetMovement");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Heal,								"Animation.Montage.Heal");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Hit_Slash,							"Animation.Montage.Hit.Slash");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Hit_Stab,							"Animation.Montage.Hit.Stab");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Hit_Smash,							"Animation.Montage.Hit.Smash");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Attack_Slash,						"Animation.Montage.Attack.Slash");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Attack_Stab,						"Animation.Montage.Attack.Stab");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Attack_Smash,						"Animation.Montage.Attack.Smash");

	UE_DEFINE_GAMEPLAY_TAG(Animation_Event_Hit,									"Animation.Event.Hit");
}

namespace AbilityTags
{
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_LevelUp,								"GameplayAbility.LevelUp");

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
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_ActorState_Dead,						"GameplayEffect.ActorState.Dead");

	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect,							"GameplayEffect.StatusEffect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_Buff,					"GameplayEffect.StatusEffect.Buff");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_Buff_Agility,			"GameplayEffect.StatusEffect.Buff.Agility");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_Buff_Fortification,		"GameplayEffect.StatusEffect.Buff.Fortification");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_Debuff,					"GameplayEffect.StatusEffect.Debuff");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_Debuff_Weakness,			"GameplayEffect.StatusEffect.Debuff.Weakness");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_Debuff_Vulnerability,	"GameplayEffect.StatusEffect.Debuff.Vulnerability");
}

namespace CueTags
{
}
