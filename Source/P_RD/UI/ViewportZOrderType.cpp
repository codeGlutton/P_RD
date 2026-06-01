#include "UI/ViewportZOrderType.h"

int32 FViewportZOrder::ToZOrder(EViewportZOrderType Type)
{
	switch (Type)
	{
	case EViewportZOrderType::Base:
	case EViewportZOrderType::None:
		return 0;
	case EViewportZOrderType::HUD:
		return 100;
	case EViewportZOrderType::Notification:
		return 800;
	case EViewportZOrderType::PopUp:
		return 1000;
	case EViewportZOrderType::Modal:
		return 1200;
	default:
		return 0;
	}
}
