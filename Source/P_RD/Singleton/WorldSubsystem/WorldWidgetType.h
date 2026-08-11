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
	// 구 TopMenuBar 자리. enum 값 = DefaultGame.ini 배열 인덱스라 뒷항목이 밀리지 않게 예약으로 비워 둔다.
	ReservedLegacySlot0 = 0 UMETA(Hidden),
	// 삭제된 MsgNotify/SaveNotify 자리. 위젯 클래스가 옛 HUD와 함께 사라져
	// 게임 모드가 만들지도 열지도 않는다. 뒤 enum 값과 ini 인덱스를 밀지 않는다.
	ReservedLegacyMsgNotifySlot UMETA(Hidden),
	ReservedLegacySaveNotifySlot UMETA(Hidden),

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
	 * WorldWidgetSubsystem에 두면 타이틀과 인게임 화면이 모두 같은 OpenUI/CloseUI 경로를 공유할 수 있다.
	 */
	InGameSettings,

	// 삭제된 DicePanel의 직렬화 값을 보존한다. 뒤 enum 값과 ini 인덱스를 밀지 않는다.
	ReservedLegacyDicePanelSlot UMETA(Hidden),

	// 삭제된 SkillPanel(WBP_SkillPanel)의 자리를 보존한다. 여는 진입점(구 탑바)이
	// 옛 HUD와 함께 사라졌고, 스킬 UX는 전투 커맨드 카드·상세창 스킬 행·고용 카드로
	// 대체됐다. 뒤 enum 값과 ini 인덱스를 밀지 않는다.
	ReservedLegacySkillPanelSlot UMETA(Hidden),

	// 삭제된 CharacterSelect(WBP_CharacterSelect_New)의 자리를 보존한다.
	// 용병 선택(MercenaryHire)으로 대체되어 게임 모드가 더는 만들지도 열지도 않는다.
	// 뒤 enum 값과 ini 인덱스를 밀지 않는다.
	ReservedLegacyCharacterSelectSlot UMETA(Hidden),

	/**
	 * @brief 타이틀 START 로 여는 용병 선택 게시판
	 *
	 * @details
	 * 런을 시작할 때 여섯 중 셋을 고르는 화면이다. 한 명만 고르던 캐릭터
	 * 선택을 대신한다 -- 그 위젯은 아직 [9] 에 남아 있지만 게임 모드가 더는
	 * 열지 않는다.
	 *
	 * @note
	 * Config/DefaultGame.ini의 mWorldWidgetClasses index와 이 enum 순서는 직접 대응한다.
	 * 앞 인덱스가 밀리지 않게 항상 마지막 실제 값으로 두고, MercenaryHire는 [10] 매핑을 쓴다.
	 */
	MercenaryHire,

	// 삭제된 공용 인벤토리의 ini 배열 인덱스를 보존한다.
	ReservedLegacyInventorySlot UMETA(Hidden),

	Count UMETA(Hidden),
};

