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
 *
 * 왜 DicePanel과 같은 베이스를 쓰는가:
 * 스킬과 주사위는 최종 게임 로직은 다르지만, "탑바 버튼으로 열림", "여러 카드 중 하나를 고름", "닫기 버튼으로 닫힘"이라는 UI 흐름은 같다.
 * 이 공통 흐름을 UCarouselPanelWidget에 모아두면 스킬 데이터가 붙을 때도 패널 배치/터치 규칙을 다시 만들 필요가 없다.
 *
 * @note
 * 이 클래스는 아직 스킬 사용 요청을 발생시키지 않는다.
 * 추후 실제 기능이 붙으면 선택된 스킬 ID를 전투/스킬 Subsystem으로 넘기는 역할만 추가하고,
 * 효과 적용 자체는 UI 바깥 로직에 맡기는 방향이 맞다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API USkillPanelWidget : public UCarouselPanelWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 스킬 패널 기본 객체를 생성한다.
	 *
	 * @details
	 * 카드 슬롯 캐싱, 선택, 닫기 처리는 부모 클래스가 담당한다.
	 */
	USkillPanelWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
