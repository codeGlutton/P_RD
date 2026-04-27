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
	/* 패시브 능력 태그들 */

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_StartBattle);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_EndBattle);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_StartTurn);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_EndTurn);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_StartRollingDice);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_EndRollingDice);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_StartAttacking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_EndAttacking);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_StartHitting);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_EndHitting);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_GetMoveCount);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_Passive_ChangeHP);
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
