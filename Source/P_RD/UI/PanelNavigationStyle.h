#pragma once

/**
 * @file PanelNavigationStyle.h
 * @brief 메뉴/네비게이션 패널이 공통으로 쓰는 색상 팔레트입니다.
 *
 * @details
 * 버튼·텍스트·선택 강조색을 코드 상수로 한곳에 모았습니다. 런타임에 색을 입히는 패널이
 * 여러 개라, 각 위젯이 같은 색 값을 따로 들고 있으면 톤이 어긋나기 쉽습니다.
 * 색을 바꾸려면 이 함수들만 고치면 전체 패널에 동일하게 반영되도록 하기 위한 분리입니다.
 *
 * 이 namespace의 의도는 디자인 시스템 전체를 만들려는 것이 아니라, 이번 UI 패널 묶음에서
 * 반복 사용되는 색상 토큰을 의미 단위로 고정하는 것입니다. 호출부는 "무슨 색인가"보다
 * "어떤 역할의 색인가"를 선택하게 되어, 리뷰 시 색상 변경의 영향 범위를 좁게 볼 수 있습니다.
 */

#include "RDMinimal.h"

/**
 * @brief 네비게이션 패널 테마 색을 돌려주는 함수 모음입니다.
 *
 * @details
 * 각 함수는 용도별(일반 버튼 텍스트, 선택된 버튼, 패널 텍스트, 뒤로 가기 등) 색을 반환합니다.
 * 의미 단위로 이름을 나눠, 호출부가 raw 색상값 대신 "어디에 쓰는 색"인지로 참조하게 합니다.
 *
 * @note 색상 값 자체는 .cpp에만 둡니다. 헤더에는 의미만 노출해, 호출부가 특정 RGB 값에 의존하지 않게 합니다.
 */
namespace RDPanelNavigationStyle
{
	/**
	 * @brief 일반 버튼의 글자색입니다.
	 * @return 기본 상태의 버튼 라벨에 사용할 어두운 텍스트 색.
	 */
	P_RD_API FLinearColor GetButtonTextColor();
	/**
	 * @brief 현재 선택된 버튼의 글자색입니다.
	 * @return 선택 배경 위에서도 읽히도록 일반 버튼보다 더 눌러 잡은 텍스트 색.
	 */
	P_RD_API FLinearColor GetSelectedButtonTextColor();
	/**
	 * @brief 패널 본문 텍스트색입니다.
	 * @return 어두운 패널 배경 위에서 사용하는 밝은 본문 텍스트 색.
	 */
	P_RD_API FLinearColor GetPanelTextColor();
	/**
	 * @brief 설정 옵션 버튼의 배경색입니다.
	 * @return 반복 옵션 항목에 사용할 반투명 기본 버튼 색.
	 */
	P_RD_API FLinearColor GetOptionButtonColor();
	/**
	 * @brief 화면 이동(네비게이션) 버튼의 배경색입니다.
	 * @return 주요 이동 버튼에 사용할 반투명 기본 버튼 색.
	 */
	P_RD_API FLinearColor GetNavigationButtonColor();
	/**
	 * @brief 뒤로 가기 버튼의 배경색입니다.
	 * @return 다른 이동 버튼과 구분되는 보조 강조 색.
	 */
	P_RD_API FLinearColor GetBackButtonColor();
	/**
	 * @brief 선택 강조된 버튼의 배경색입니다.
	 * @return 현재 선택/확정 대상임을 보여주는 강조 색.
	 */
	P_RD_API FLinearColor GetSelectedButtonColor();
}
