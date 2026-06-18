#include "UI/TopMenuBarLayoutPolicy.h"

/** @brief 투명 hit area가 입력만 받도록 거의 보이지 않는 흰색을 쓴다. */
FLinearColor RDTopMenuBarLayout::GetRuntimeHitAreaColor()
{
	return FLinearColor(1.0f, 1.0f, 1.0f, 0.01f);
}

/** @brief DICE 버튼 아래쪽까지 터치 허용 영역을 확장한 anchor. */
FAnchors RDTopMenuBarLayout::GetDiceHitAreaAnchors()
{
	return FAnchors(0.815f, 0.105f, 0.870f, 0.170f);
}

/** @brief SKILL 버튼 아래쪽까지 터치 허용 영역을 확장한 anchor. */
FAnchors RDTopMenuBarLayout::GetSkillHitAreaAnchors()
{
	return FAnchors(0.870f, 0.105f, 0.930f, 0.170f);
}

/** @brief 디자이너 버튼보다 위에 올라와야 모바일 터치가 누락되지 않는다. */
int32 RDTopMenuBarLayout::GetRuntimeHitAreaZOrder()
{
	return 1000;
}
