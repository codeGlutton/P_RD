#pragma once

/**
 * @file PanelNavigationStyle.h
 * @brief 메뉴/네비게이션 패널이 공통으로 쓰는 색상 팔레트입니다.
 *
 * 버튼·텍스트·선택 강조색을 코드 상수로 한곳에 모았습니다. 런타임에 색을 입히는 패널이
 * 여러 개라, 각 위젯이 같은 색 값을 따로 들고 있으면 톤이 어긋나기 쉽습니다.
 * 색을 바꾸려면 이 함수들만 고치면 전체 패널에 동일하게 반영되도록 하기 위한 분리입니다.
 */

#include "RDMinimal.h"

/**
 * @brief 네비게이션 패널 테마 색을 돌려주는 함수 모음입니다.
 *
 * 각 함수는 용도별(일반 버튼 텍스트, 선택된 버튼, 패널 텍스트, 뒤로 가기 등) 색을 반환합니다.
 * 의미 단위로 이름을 나눠, 호출부가 raw 색상값 대신 "어디에 쓰는 색"인지로 참조하게 합니다.
 */
namespace RDPanelNavigationStyle
{
	/** @brief 일반 버튼의 글자색. */
	P_RD_API FLinearColor GetButtonTextColor();
	/** @brief 현재 선택된 버튼의 글자색(일반 버튼과 구분되도록 어둡게). */
	P_RD_API FLinearColor GetSelectedButtonTextColor();
	/** @brief 패널 본문 텍스트색. */
	P_RD_API FLinearColor GetPanelTextColor();
	/** @brief 설정 옵션 버튼의 배경색. */
	P_RD_API FLinearColor GetOptionButtonColor();
	/** @brief 화면 이동(네비게이션) 버튼의 배경색. */
	P_RD_API FLinearColor GetNavigationButtonColor();
	/** @brief 뒤로 가기 버튼의 배경색(다른 버튼과 대비되는 강조 톤). */
	P_RD_API FLinearColor GetBackButtonColor();
	/** @brief 선택 강조된 버튼의 배경색. */
	P_RD_API FLinearColor GetSelectedButtonColor();
}
