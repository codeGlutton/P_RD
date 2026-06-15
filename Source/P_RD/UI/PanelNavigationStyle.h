#pragma once

/**
 * @file PanelNavigationStyle.h
 * @brief 메뉴 패널들이 같이 쓰는 색 모음입니다.
 *
 * @details
 * 여러 패널에서 같은 버튼색, 글자색을 쓰도록 색 값을 한곳에 모았습니다.
 * 색을 바꾸고 싶으면 여기 함수만 고치면 됩니다.
 */

#include "RDMinimal.h"

/**
 * @brief 메뉴 UI에서 쓰는 색을 돌려주는 함수들입니다.
 *
 * @details
 * 호출하는 쪽에서 직접 RGB 값을 쓰지 않고, "선택 버튼 색", "뒤로가기 버튼 색"처럼 용도별로 가져가게 합니다.
 */
namespace RDPanelNavigationStyle
{
	/**
	 * @brief 일반 버튼의 글자색입니다.
	 * @return 기본 버튼 글자색.
	 */
	P_RD_API FLinearColor GetButtonTextColor();
	/**
	 * @brief 현재 선택된 버튼의 글자색입니다.
	 * @return 선택된 버튼 위에 올릴 글자색.
	 */
	P_RD_API FLinearColor GetSelectedButtonTextColor();
	/**
	 * @brief 패널 본문 텍스트색입니다.
	 * @return 패널 안의 일반 문구 색.
	 */
	P_RD_API FLinearColor GetPanelTextColor();
	/**
	 * @brief 설정 옵션 버튼의 배경색입니다.
	 * @return 설정 항목 버튼 배경색.
	 */
	P_RD_API FLinearColor GetOptionButtonColor();
	/**
	 * @brief 화면 이동(네비게이션) 버튼의 배경색입니다.
	 * @return 화면 이동 버튼 배경색.
	 */
	P_RD_API FLinearColor GetNavigationButtonColor();
	/**
	 * @brief 뒤로 가기 버튼의 배경색입니다.
	 * @return 뒤로 가기 버튼 배경색.
	 */
	P_RD_API FLinearColor GetBackButtonColor();
	/**
	 * @brief 선택 강조된 버튼의 배경색입니다.
	 * @return 선택된 버튼 배경색.
	 */
	P_RD_API FLinearColor GetSelectedButtonColor();
}
