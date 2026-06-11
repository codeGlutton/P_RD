#include "UI/SkillPanelWidget.h"

/**
 * @brief 스킬 패널 기본 객체를 생성한다.
 *
 * @details
 * 현재 스킬 패널은 별도 게임 로직 없이 UCarouselPanelWidget의 카드 선택/닫기 동작을 그대로 사용한다.
 * 실제 스킬 데이터와 사용 규칙이 붙기 전까지는 WBP 표시와 탑바 연결 검증용 패널로 둔다.
 */
USkillPanelWidget::USkillPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
