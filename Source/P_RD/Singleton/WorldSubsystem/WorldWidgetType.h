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
	
	FadeInOut,
	LoadingNotify,

	/**
	 * @brief 인게임에서 현재 런의 월드맵을 표시하는 공용 위젯
	 *
	 * @details
	 * MAP 버튼으로 열면 조회용, 전투 승리 후 열면 다음 방 선택용으로 사용한다.
	 */
	WorldMap,

	/**
	 * @brief 인게임 설정 패널 공용 위젯
	 *
	 * @details
	 * 타이틀 설정과 같은 WBP 구조를 공유하되, 런 저장/포기 같은 인게임 전용 액션 영역을 표시한다.
	 */
	InGameSettings,

	Count UMETA(Hidden),
};

