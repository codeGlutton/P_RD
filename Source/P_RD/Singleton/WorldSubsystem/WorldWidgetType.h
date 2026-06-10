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
	 *
	 * 왜 WorldWidget인가:
	 * 월드맵은 특정 방 HUD의 일부가 아니라 모든 방에서 같은 방식으로 열리는 공용 팝업이다.
	 * WorldWidgetSubsystem에 두면 방 HUD가 바뀌어도 MAP 버튼/승리 흐름이 같은 인스턴스 경로를 쓴다.
	 */
	WorldMap,

	/**
	 * @brief 인게임 설정 패널 공용 위젯
	 *
	 * @details
	 * 타이틀 설정과 같은 WBP 구조를 공유하되, 런 저장/포기 같은 인게임 전용 액션 영역을 표시한다.
	 *
	 * 왜 WorldWidget인가:
	 * 설정은 어느 방에서든 동일한 팝업으로 열려야 하고, 방별 HUD가 직접 소유하면 닫기/복귀 규칙이 갈라질 수 있다.
	 * 공용 월드 위젯으로 두면 TopMenuBar가 항상 같은 OpenUI 경로로 열 수 있다.
	 */
	InGameSettings,

	Count UMETA(Hidden),
};

