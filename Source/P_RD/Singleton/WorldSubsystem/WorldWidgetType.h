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

	/**
	 * @brief 인게임 탑바의 주사위 버튼으로 여는 공용 주사위 패널
	 *
	 * @details
	 * 전투 HUD가 직접 소유하지 않고 WorldWidget으로 준비해 두면, 탑바는 다른 팝업과 같은 OpenUI/CloseUI 규칙으로 열 수 있다.
	 * 현재는 실제 주사위 사용 로직이 아니라 WBP_DicePanel 표시, 카드 선택, 임시 회전 입력을 확인하는 단계다.
	 *
	 * @note
	 * Config/DefaultGame.ini의 mWorldWidgetClasses index와 이 enum 순서는 직접 대응한다.
	 * DicePanel 위치가 바뀌면 ini의 [7] 매핑도 같이 조정해야 한다.
	 */
	DicePanel,

	/**
	 * @brief 인게임 탑바의 스킬 버튼으로 여는 공용 스킬 패널
	 *
	 * @details
	 * 주사위 패널과 같은 플로팅 팝업 계층에 두어 MAP/SET/DICE/SKILL 중 하나만 열리는 규칙을 공유한다.
	 * 현재는 실제 스킬 실행 로직이 아니라 WBP_SkillPanel을 탑바에서 열고 닫는 연결을 확인하는 단계다.
	 *
	 * @note
	 * Config/DefaultGame.ini의 mWorldWidgetClasses index와 이 enum 순서는 직접 대응한다.
	 * SkillPanel 위치가 바뀌면 ini의 [8] 매핑도 같이 조정해야 한다.
	 */
	SkillPanel,

	Count UMETA(Hidden),
};

