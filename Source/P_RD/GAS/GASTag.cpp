#include "GAS/GASTag.h"

namespace InputTags
{
}

namespace EventTags
{
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
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartUseSkill_Add,			"GameplayAbility.Passive.OnStartUseSkill.Add");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartUseSkill_Multiply,	"GameplayAbility.Passive.OnStartUseSkill.Multiply");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartUseSkill_Final,		"GameplayAbility.Passive.OnStartUseSkill.Final");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndUseSkill,				"GameplayAbility.Passive.OnEndUseSkill");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartAttacking_Add,		"GameplayAbility.Passive.OnStartAttacking.Add");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartAttacking_Multiply,	"GameplayAbility.Passive.OnStartAttacking.Multiply");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartAttacking_Final,		"GameplayAbility.Passive.OnStartAttacking.Final");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndAttacking,				"GameplayAbility.Passive.OnEndAttacking");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartHitting_Add,			"GameplayAbility.Passive.OnStartHitting.Add");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartHitting_Multiply,		"GameplayAbility.Passive.OnStartHitting,Multiply");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartHitting_Final,		"GameplayAbility.Passive.OnStartHitting.Final");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndHitting,				"GameplayAbility.Passive.OnEndHitting");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnStartOneMove,				"GameplayAbility.Passive.OnStartOneMove");
	UE_DEFINE_GAMEPLAY_TAG(GameplayAbility_Passive_OnEndOneMove,				"GameplayAbility.Passive.OnEndOneMove");
}

namespace EffectTags
{
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_Cost,									"GameplayEffect.Cost");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_Cost_PassiveStack,					"GameplayEffect.Cost.PassiveStack");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_Cooldown,								"GameplayEffect.Cooldown");
	UE_DEFINE_GAMEPLAY_TAG(GameplayEffect_ActorState_Dead,						"GameplayEffect.ActorState.Dead");
}

namespace CueTags
{
}
