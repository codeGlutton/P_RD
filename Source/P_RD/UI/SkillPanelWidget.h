/**
 * @file SkillPanelWidget.h
 * @brief 인게임 탑바 스킬 버튼으로 여는 임시 스킬 패널.
 */

#pragma once

#include "RDMinimal.h"
#include "UI/CarouselPanelWidget.h"

#include "SkillPanelWidget.generated.h"

/**
 * @brief 스킬 카드형 목록을 보여주는 플로팅 패널
 *
 * @details
 * 현재 단계에서는 실제 스킬 실행 로직을 붙이지 않고, WBP로 만든 스킬 패널을 탑바에서 열고 닫을 수 있게 한다.
 * 항목 선택/닫기 동작은 UCarouselPanelWidget의 공통 흐름을 그대로 사용한다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API USkillPanelWidget : public UCarouselPanelWidget
{
	GENERATED_BODY()

public:
	USkillPanelWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
