#include "UI/PanelNavigationStyle.h"

/*
 * 패널(설정/스킬 등) 네비게이션의 공통 색 팔레트를 한곳에 모은다.
 *
 * 왜 함수로 한곳에 두는가:
 * - 같은 색을 여러 위젯이 코드로 만들면 톤을 바꿀 때 전부 찾아다녀야 한다.
 *   여기 함수만 고치면 모든 패널 색이 일관되게 바뀐다(디자인 토큰 역할).
 * - 머티리얼/에디터 에셋이 아니라 코드 값인 이유: 패널 위젯 상당수가 런타임에
 *   ConstructWidget로 생성돼 디자이너 에셋에 색을 미리 박아둘 수 없기 때문.
 *
 * 팔레트 컨셉: 바탕은 옅은 민트(차분한 배경), 되돌리기/선택 계열은 따뜻한 앰버(주목·구분).
 */

// 일반 버튼 글자색: 밝은 버튼 위에서 읽히도록 거의 검정에 가까운 짙은 톤.
FLinearColor RDPanelNavigationStyle::GetButtonTextColor()
{
	return FLinearColor(0.05f, 0.08f, 0.08f, 1.0f);
}

// 선택된 버튼 글자색: 앰버 강조 배경 위에서도 대비가 유지되도록 더 짙게.
FLinearColor RDPanelNavigationStyle::GetSelectedButtonTextColor()
{
	return FLinearColor(0.05f, 0.05f, 0.04f, 1.0f);
}

// 패널 본문 글자색: 어두운 패널 배경 위라 밝은 민트 톤으로 가독성 확보.
FLinearColor RDPanelNavigationStyle::GetPanelTextColor()
{
	return FLinearColor(0.86f, 1.0f, 0.94f, 1.0f);
}

// 옵션(토글류) 버튼: 바탕 민트, 알파를 살짝 낮춰 네비 버튼보다 덜 강조.
FLinearColor RDPanelNavigationStyle::GetOptionButtonColor()
{
	return FLinearColor(0.80f, 0.96f, 0.90f, 0.86f);
}

// 화면 이동(네비게이션) 버튼: 같은 민트지만 알파를 높여 더 또렷하게.
FLinearColor RDPanelNavigationStyle::GetNavigationButtonColor()
{
	return FLinearColor(0.80f, 0.96f, 0.90f, 0.94f);
}

// 뒤로가기/취소: 진행 계열과 한눈에 구분되도록 따뜻한 앰버.
FLinearColor RDPanelNavigationStyle::GetBackButtonColor()
{
	return FLinearColor(0.94f, 0.84f, 0.56f, 0.96f);
}

// 현재 선택 항목 강조: 뒤로가기와 같은 앰버 계열로 "활성" 상태를 표시.
FLinearColor RDPanelNavigationStyle::GetSelectedButtonColor()
{
	return FLinearColor(0.94f, 0.84f, 0.56f, 0.98f);
}
