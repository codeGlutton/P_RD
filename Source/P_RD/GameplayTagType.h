/*****************************************************************//**
 * @file   GameplayTagType.h
 * @brief  사용되는 태그 정의 매크로를 모아둔 파일
 * @details
 * 태그 매크로 규칙은 @subpage gameplay_tag_page 참고
 * @author 모호재
 * @date   2026-04-25
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * @brief 사용자 입력 시 사용되는 태그를 정의하는 namespace 영역
 */
namespace InputTags
{
}

/**
 * @brief 애니메이션 이벤트 전달 시 사용되는 태그를 정의하는 namespace 영역
 */
namespace AnimationTags
{
	/* 몽타쥬 애니메이션 타입 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Melee_Kick);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Melee_Punch);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Melee_Slash);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Melee_Stab);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Melee_Smash);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Ranged_Direct);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Ranged_Indirect);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_ETC_GetBuff);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_ETC_GetDeBuff);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_ETC_Spawn);

#pragma region Knight
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Knight_NormalSlash);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Knight_SwordSmash);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Knight_Protection);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Knight_Blade);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Knight_PushAttack);
#pragma endregion

#pragma region Ranger
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Ranger_NormalShot);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Ranger_PiercingShot);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Ranger_SnipeShot);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Ranger_InDirectShot);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Ranger_Acceleration);
#pragma endregion

#pragma region Mage
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Mage_NormalEnergyBall);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Mage_Blizarrd);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Mage_EarthQuake);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Mage_EldritchBlast);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Mage_FireBall);
#pragma endregion

#pragma region Barbarian
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Barbarian_NormalSlash);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Barbarian_PowerSlash);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Barbarian_SpinAttack);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Barbarian_Meditation);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Barbarian_Rage);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Barbarian_Taunt);
#pragma endregion

#pragma region Rogue
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Rogue_NormalSlash);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Rogue_Disarmament);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Rogue_PoisonAttack);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Rogue_MultiSlash);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Rogue_SharpOil);
#pragma endregion

#pragma region Druid
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Druid_NormalAttack);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Druid_Longstrider);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Druid_Aging);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Druid_Heal);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Skill_Mercenary_Druid_ThornWhip);
#pragma endregion

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Hit_Damage);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Hit_Buff);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Montage_Hit_DeBuff);


	/* 애니메이션 이벤트 타입 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Event_Skill_HitLogic);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Event_Skill_HitAnimation);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Event_Skill_HitVFX);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Event_Skill_CameraZoom);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Event_Skill_CameraShake);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Animation_Event_Skill_TimeScale);
}

/**
 * @brief Ability를 나타내는 태그를 정의하는 namespace 영역
 */
namespace AbilityTags
{
	/* 플레이어 핵심 능력 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_LevelUp);

	/* 패시브 능력 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartRoom);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndRoom);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartTurn);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndTurn);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartRollingDice);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndRollingDice);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartUsingSkill);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndUsingSkill);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartApplyingEffect);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndApplyingEffect);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartReceivingEffect);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndReceivingEffect);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartOneMove);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndOneMove);
}

/**
 * @brief Effect와 연관된 결과값 태그를 정의하는 namespace 영역
 */
namespace EffectTags
{
	/* 코스트 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_Cost);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_Cost_PassiveStack);

	/* 액터 상태 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_ActorState_Dead);

	/* 상태 이상 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect);

	/* 전투 종료 시까지 유지되는 상태 이상 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_Infinite);

	/* 턴마다 1씩 감소하는 상태 이상 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_TurnDuration);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_TurnDuration_Buff);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_TurnDuration_Buff_Agility);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_TurnDuration_Buff_Fortification);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_TurnDuration_Debuff);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_TurnDuration_Debuff_Vulnerability);
}

/**
 * @brief Cue를 실행하는 태그를 정의하는 namespace 영역
 */
namespace CueTags
{
}
