/*****************************************************************//**
 * @file   GASTag.h
 * @brief  사용되는 태그 정의 매크로를 모아둔 파일
 * @details
 * 태그 매크로 규칙은 @subpage gas_tag_page 참고
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

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_LevelUp);

	/* 패시브 능력 태그들 */

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartStage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndStage);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartTurn);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndTurn);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartRollingDice);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndRollingDice);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnUseAttackSkill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnUseDefenseSkill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnUseMoveSkill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnUseBuffSkill);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartAttacking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndAttacking);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnStartHitting);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnEndHitting);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnGetDefensePoint);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnGetMovePoint);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnGetPower);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_OnChangeHP);
}

/**
 * @brief Effect에 의해 추가되는 태그를 정의하는 namespace 영역
 */
namespace EffectTags
{
	/* 코스트 태그들 */

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_Cost);

	/* 쿨다운 태그들 */

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEffect_Cooldown);
}

/**
 * @brief Cue를 실행하는 태그를 정의하는 namespace 영역
 */
namespace CueTags
{
}
