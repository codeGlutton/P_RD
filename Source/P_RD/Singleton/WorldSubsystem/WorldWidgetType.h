/*****************************************************************//**
 * @file   WorldWidgetType.h
 * @brief  특정한 소유자 없이 월드에 소속된 Widget 타입 정의 헤더
 * @author 모호재
 * @date   2026-05-22
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "WorldWidgetType.generated.h"

/**
 * @brief  특정한 소유자 없이 월드에 소속된 Widget 타입
 */
UENUM(BlueprintType)
enum class EWorldWidgetType : uint8
{
	TopMenuBar = 0,
	MsgNotify,
	SaveNotify,

	// 추가 가능

	Count UMETA(Hidden),
};

