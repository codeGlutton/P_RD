/*****************************************************************//**
 * @file   ViewportZOrderType.h
 * @brief  뷰포트 등록 시, Z 순서 타입 정의 헤더
 * @author 모호재
 * @date   2026-05-22
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ViewportZOrderType.generated.h"

/**
 * @brief  뷰포트 등록 시, Z 순서 타입
 */
UENUM(BlueprintType)
enum class EViewportZOrderType : uint8
{
	None = 0,
	Base = 1,
	HUD = 2,
	Notification = 3,
	PopUp = 10,
	Modal = 20,
};

/**
 * @brief  뷰포트 Z 순서 타입을 실제 ZOrder 값으로 변환
 */
struct P_RD_API FViewportZOrder
{
	static int32 ToZOrder(EViewportZOrderType Type);
};

