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
	None = 0			UMETA(ToolTip = "단순 위젯"),
	PopUp = 10			UMETA(ToolTip = "단순 팝업"),
	FadeInOut = 20		UMETA(ToolTip = "페이드 인, 페이드 아웃"),
	LoadingNotify = 30	UMETA(ToolTip = "페이드 인, 페이드 아웃 위에 뜰 로딩 메세지"),
};

