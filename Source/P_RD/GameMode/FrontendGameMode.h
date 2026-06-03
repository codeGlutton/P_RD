/*****************************************************************//**
 * @file   FrontendGameMode.h
 * @brief  프론트엔드 GameMode 정의 헤더
 * @author Codex
 * @date   2026-06-02
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"

#include "FrontendGameMode.generated.h"

/**
 * @brief 게임 시작 전 타이틀/캐릭터 선택 UI를 띄우는 프론트엔드 GameMode
 *
 * @details
 * 전투 방을 시작하기 전 단계에서 사용하는 GameMode다.
 * 일반 방처럼 플레이어 유닛을 스폰하거나 전투용 WorldWidget을 만들지 않고,
 * 타이틀 HUD만 화면에 올린 뒤 UI 입력을 받을 수 있게 한다.
 *
 * @note
 * 이번 UI-only 브랜치에서는 캐릭터 선택 후 실제 런 시작과 방 전환을 연결하지 않는다.
 * FrontendGameMode는 타이틀/캐릭터 선택 HUD를 띄우고 UI 입력 모드로 바꾸는 역할만 맡는다.
 */
UCLASS()
class P_RD_API AFrontendGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

public:
	/**
	 * @brief 프론트엔드 HUD 클래스를 설정하고 방 위젯 목록을 비움
	 *
	 * @details
	 * GamePlaySettings의 mFrontendHUDClass가 있으면 그 클래스를 사용한다.
	 * 설정이 비어 있으면 UTitleMenuWidget 네이티브 클래스를 fallback으로 사용한다.
	 *
	 * 타이틀 화면은 전투 HUD나 월드 위젯이 필요 없으므로 mWorldWidgets를 비운다.
	 */
	AFrontendGameMode();

protected:
	/**
	 * @brief 타이틀 화면에서는 일반 방 초기화를 하지 않음
	 *
	 * @details
	 * 타이틀은 전투 방이 아니므로 플레이어 유닛, 적, 방 오브젝트를 만들지 않는다.
	 * 여기서 부모의 방 초기화를 호출하면 캐릭터 선택 전인데도 게임 플레이 상태가 만들어질 수 있다.
	 */
	void InitializeCommonRoom() override;

	/**
	 * @brief 타이틀 HUD를 화면에 올리고 UI 입력 모드로 바꿈
	 *
	 * @details
	 * WorldWidgetSubsystem이 만든 HUD를 Viewport에 추가하고 보이게 한다.
	 * 타이틀 맵에 TitleCamera 태그가 붙은 카메라가 있으면 그 카메라를 화면 기준으로 사용한다.
	 * 그 뒤 PlayerController를 UIOnly 입력 모드로 바꿔 마우스/터치로 버튼을 누를 수 있게 한다.
	 */
	void BeginRoom() override;
};
