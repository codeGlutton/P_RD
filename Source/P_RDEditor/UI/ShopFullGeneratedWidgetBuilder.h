#pragma once

namespace ShopFullGeneratedWidgetBuilder
{
	/** 선택한 상점 원화를 임포트하고 WBP_Shop_FullGenerated를 재생성한다. */
	P_RDEDITOR_API void Build();
}

void RegisterShopFullGeneratedWidgetBuilderCommands();
void UnregisterShopFullGeneratedWidgetBuilderCommands();
