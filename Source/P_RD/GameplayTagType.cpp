#include "GameplayTagType.h"

namespace InputTags
{
}

namespace AnimationTags
{
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Kick,									"Animation.Montage.Skill.Melee.Kick");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Punch,									"Animation.Montage.Skill.Melee.Punch");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Slash,									"Animation.Montage.Skill.Melee.Slash");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Stab,									"Animation.Montage.Skill.Melee.Stab");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Smash,									"Animation.Montage.Skill.Melee.Smash");

	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Ranged_Direct,							"Animation.Montage.Skill.Ranged.Direct");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Ranged_Indirect,							"Animation.Montage.Skill.Ranged.InDirect");

	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_ETC_GetBuff,								"Animation.Montage.Skill.ETC.GetBuff");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_ETC_GetDeBuff,							"Animation.Montage.Skill.ETC.GetDeBuff");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_ETC_Spawn,								"Animation.Montage.Skill.ETC.Spawn");

	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Special_0,								"Animation.Montage.Skill.Special.0");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Special_1,								"Animation.Montage.Skill.Special.1");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Special_2,								"Animation.Montage.Skill.Special.2");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Special_3,								"Animation.Montage.Skill.Special.3");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Skill_Special_4,								"Animation.Montage.Skill.Special.4");


	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Hit_Damage,									"Animation.Montage.Hit.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Hit_Buff,										"Animation.Montage.Hit.Buff");
	UE_DEFINE_GAMEPLAY_TAG(Animation_Montage_Hit_DeBuff,									"Animation.Montage.Hit.DeBuff");


	UE_DEFINE_GAMEPLAY_TAG(Animation_Event_Hit,												"Animation.Event.Hit");
}

namespace AbilityTags
{
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_LevelUp,											"GameplayAbility.LevelUp");

	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartRoom,								"GameplayAbility.Passive.OnStartRoom");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndRoom,								"GameplayAbility.Passive.OnEndRoom");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartTurn,								"GameplayAbility.Passive.OnStartTurn");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndTurn,								"GameplayAbility.Passive.OnEndTurn");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartRollingDice,						"GameplayAbility.Passive.OnStartRollingDice");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndRollingDice,						"GameplayAbility.Passive.OnEndRollingDice");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartUsingSkill,						"GameplayAbility.Passive.OnStartUsingSkill");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndUsingSkill,							"GameplayAbility.Passive.OnEndUsingSkill");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartApplyingEffect,					"GameplayAbility.Passive.OnStartApplyingEffect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndApplyingEffect,						"GameplayAbility.Passive.OnEndApplyingEffect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartReceivingEffect,					"GameplayAbility.Passive.OnStartReceivingEffect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndReceivingEffect,					"GameplayAbility.Passive.OnEndReceivingEffect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartOneMove,							"GameplayAbility.Passive.OnStartOneMove");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndOneMove,							"GameplayAbility.Passive.OnEndOneMove");
}

namespace EffectTags
{
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_Cost,												"GameplayEffect.Cost");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_Cost_PassiveStack,								"GameplayEffect.Cost.PassiveStack");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_ActorState_Dead,									"GameplayEffect.ActorState.Dead");

	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect,										"GameplayEffect.StatusEffect");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_Infinite,							"GameplayEffect.StatusEffect.Infinite");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_TurnDuration,						"GameplayEffect.StatusEffect.TurnDuration");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_TurnDuration_Buff,					"GameplayEffect.StatusEffect.TurnDuration.Buff");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_TurnDuration_Buff_Agility,			"GameplayEffect.StatusEffect.TurnDuration.Buff.Agility");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_TurnDuration_Buff_Fortification,		"GameplayEffect.StatusEffect.TurnDuration.Buff.Fortification");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_TurnDuration_Debuff,					"GameplayEffect.StatusEffect.TurnDuration.Debuff");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness,		"GameplayEffect.StatusEffect.TurnDuration.Debuff.Weakness");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_StatusEffect_TurnDuration_Debuff_Vulnerability,	"GameplayEffect.StatusEffect.TurnDuration.Debuff.Vulnerability");
}

namespace CueTags
{
}
