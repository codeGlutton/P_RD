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
	 * @brief 타이틀과 인게임에서 함께 여는 공용 설정 패널
	 *
	 * @details
	 * 같은 WBP_SettingsPanel을 타이틀 SETTING 버튼과 인게임 SET 버튼이 함께 사용한다.
	 * 타이틀에서는 Title 모드로 런 저장/포기 영역을 숨기고, 인게임에서는 InGame 모드로 현재 런 액션을 표시한다.
	 *
	 * 왜 WorldWidget인가:
	 * 설정은 화면마다 따로 만든 슬롯이 아니라 같은 팝업 생명주기로 열려야 한다.
	 * WorldWidgetSubsystem에 두면 타이틀과 TopMenuBar가 모두 같은 OpenUI/CloseUI 경로를 공유할 수 있다.
	 */
	InGameSettings,

	/**
	 * @brief 인게임 DICE 버튼으로 여는 보유 주사위 패널
	 *
	 * @details
	 * 탑바가 공용 진입점으로 OpenUI/CloseUI를 관리하고, 패널은 표시와 상세 확인만 담당한다.
	 *
	 * @note
	 * Config/DefaultGame.ini의 mWorldWidgetClasses index와 이 enum 순서는 직접 대응한다.
	 * DicePanel 위치가 바뀌면 ini의 [7] 매핑도 같이 조정해야 한다.
	 */
	DicePanel,

	/**
	 * @brief 인게임 SKILL 버튼으로 여는 보유 스킬 패널
	 *
	 * @details
	 * 탑바가 공용 진입점으로 OpenUI/CloseUI를 관리하고, 패널은 스킬 목록/상세 표시를 담당한다.
	 *
	 * @note
	 * Config/DefaultGame.ini의 mWorldWidgetClasses index와 이 enum 순서는 직접 대응한다.
	 * SkillPanel 위치가 바뀌면 ini의 [8] 매핑도 같이 조정해야 한다.
	 */
	SkillPanel,

	/**
	 * @brief 타이틀 START로 여는 독립 캐릭터 선택 오버레이
	 *
	 * @details
	 * 캐릭터 선택은 더 이상 타이틀 HUD 안의 화면이 아니라 InGameSettings처럼 OpenUI()로 여는 독립 월드 위젯이다.
	 * GameMode가 타이틀 HUD를 닫고 이 위젯을 열며, BACK 요청을 받아 다시 타이틀로 되돌린다.
	 *
	 * @note
	 * Config/DefaultGame.ini의 mWorldWidgetClasses index와 이 enum 순서는 직접 대응한다.
	 * 기존 [0..8] 인덱스가 밀리지 않게 항상 마지막 실제 값으로 두고, CharacterSelect는 [9] 매핑을 쓴다.
	 */
	CharacterSelect,

	Count UMETA(Hidden),
};

