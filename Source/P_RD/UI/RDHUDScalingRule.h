/*****************************************************************//**
 * @file   RDHUDScalingRule.h
 * @brief  HUD가 어느 해상도에서도 겹치지 않게 하는 UI 배율 규칙.
 * @details
 * 모바일은 해상도가 제각각이라 한 가지 기준만 보면 반드시 어딘가에서 깨진다.
 * 기본 규칙(ShortestSide)은 짧은 변 하나만 보므로, 세로로 긴 화면에서는 짧은
 * 변이 가로가 되어 배율을 키운다 -- 그런데 HUD는 가로로 넓은 배치라 그만큼
 * 더 삐져나간다. 실제로 창을 끌어 확인했더니 스킬 줄 오른쪽이 잘렸다.
 *
 * 가로·세로를 둘 다 보고 모자란 쪽에 맞춘다. 해상도가 커져도 동일 화면비의
 * 버튼과 글자가 화면에서 차지하는 비율을 유지하도록 확대 배율도 허용한다.
 * @author 박용수
 * @date   2026-07-27
 *********************************************************************/
#pragma once

#include "CoreMinimal.h"
#include "Engine/DPICustomScalingRule.h"
#include "RDHUDScalingRule.generated.h"

/**
 * @brief 설계 캔버스가 화면에 들어가는 균일 배율.
 */
UCLASS()
class P_RD_API URDHUDScalingRule : public UDPICustomScalingRule
{
	GENERATED_BODY()

public:
	virtual float GetDPIScaleBasedOnSize(FIntPoint Size) const override;
};
