#include "UI/RDHUDScalingRule.h"

namespace
{
	/** @brief HUD를 짠 캔버스. 좌표가 전부 이 크기를 전제한다.
	 *
	 * 이름에 HUD를 붙인 것은 유니티 빌드 때문이다 -- 같은 뭉치에 들어오는
	 * 다른 파일의 지역 DesignWidth와 이름이 겹치면 C4459 가림 경고가 난다.
	 */
	constexpr float HUDDesignWidth = 1920.0f;
	constexpr float HUDDesignHeight = 1080.0f;
}

/**
 * @brief 가로·세로 중 모자란 쪽에 맞춰 배율을 낸다.
 *
 * @details
 * 화면이 설계 캔버스보다 넉넉하면 1을 돌려준다 -- 판을 억지로 키우지 않는다.
 * 시안에서 오려 낸 껍데기라 확대하면 흐려지고, 남는 공간은 앵커가 구역을
 * 가장자리로 벌려 채운다.
 *
 * 좁아지면 모자란 축의 비율만큼 내려간다. 가로가 모자라면 가로 기준으로,
 * 세로가 모자라면 세로 기준으로 -- 둘 다 모자라면 더 모자란 쪽이 이긴다.
 * 그래서 어느 해상도에서도 구역이 서로 파고들지 않는다.
 */
float URDHUDScalingRule::GetDPIScaleBasedOnSize(FIntPoint Size) const
{
	if (Size.X <= 0 || Size.Y <= 0)
	{
		return 1.0f;
	}

	const float ByWidth = static_cast<float>(Size.X) / HUDDesignWidth;
	const float ByHeight = static_cast<float>(Size.Y) / HUDDesignHeight;
	return FMath::Min3(ByWidth, ByHeight, 1.0f);
}
