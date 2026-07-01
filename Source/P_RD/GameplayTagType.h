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
 * @brief 이벤트 전달 시 사용되는 태그를 정의하는 namespace 영역
 */
namespace EventTags
{
}

/**
 * @brief Ability를 나타내는 태그를 정의하는 namespace 영역
 */
namespace AbilityTags
{
	/* 플레이어 핵심 능력 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_LevelUp);

	/* 스킬 */
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Skill);


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

	/* 쿨다운 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_Cooldown);

	/* 액터 상태 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_ActorState_Dead);

	/* 상태 이상 태그들 */

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_Buff);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_Debuff);

	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_Debuff_Weakness);
	P_RD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_StatusEffect_Debuff_Vulnerability);
}

/**
 * @brief Cue를 실행하는 태그를 정의하는 namespace 영역
 */
namespace CueTags
{
}
