#include "UI/PanelNavigationStyle.h"

// 색 값은 의도적으로 이 한 파일에만 둔다. 디자인 톤을 조정할 때 여기 상수만 바꾸면
// 모든 네비게이션 패널에 똑같이 적용되도록 하기 위함이다. (헤더의 의미별 함수 이름과 1:1 대응)

FLinearColor RDPanelNavigationStyle::GetButtonTextColor()
{
	return FLinearColor(0.05f, 0.08f, 0.08f, 1.0f);
}

FLinearColor RDPanelNavigationStyle::GetSelectedButtonTextColor()
{
	return FLinearColor(0.05f, 0.05f, 0.04f, 1.0f);
}

FLinearColor RDPanelNavigationStyle::GetPanelTextColor()
{
	return FLinearColor(0.86f, 1.0f, 0.94f, 1.0f);
}

FLinearColor RDPanelNavigationStyle::GetOptionButtonColor()
{
	return FLinearColor(0.80f, 0.96f, 0.90f, 0.86f);
}

FLinearColor RDPanelNavigationStyle::GetNavigationButtonColor()
{
	return FLinearColor(0.80f, 0.96f, 0.90f, 0.94f);
}

FLinearColor RDPanelNavigationStyle::GetBackButtonColor()
{
	return FLinearColor(0.94f, 0.84f, 0.56f, 0.96f);
}

FLinearColor RDPanelNavigationStyle::GetSelectedButtonColor()
{
	return FLinearColor(0.94f, 0.84f, 0.56f, 0.98f);
}
