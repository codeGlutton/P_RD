/*****************************************************************//**
 * @file   CinematicIntroWidget.h
 * @brief  타이틀 진입 전 인트로 시네마틱 UI 베이스
 * @author Codex
 * @date   2026-06-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Singleton/WorldSubsystem/WorldWidgetTransitionInterface.h"

#include "CinematicIntroWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCinematicIntroFinishedDelegate);

/**
 * @brief 게임 시작 직후 타이틀 메뉴 앞에서 재생되는 인트로 UI
 *
 * @details
 * 이 위젯은 인트로 표시와 완료 신호만 담당한다.
 * 실제 유저 데이터 로드, 런 생성, 방 전환 같은 부트스트랩/게임 진행 책임은 GameMode/Subsystem에 둔다.
 *
 * 현재는 실제 애니메이션이 없으므로 PlayIntroAnimation() 기본 구현이 즉시 FinishIntro()를 호출한다.
 * 나중에 WBP_CinematicIntro가 이 클래스를 상속하고 애니메이션을 추가하면,
 * PlayIntroAnimation()을 BP에서 override한 뒤 애니메이션 종료 시 FinishIntro()를 호출하면 된다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCinematicIntroWidget : public UUserWidget, public IWorldWidgetTransitionInterface
{
	GENERATED_BODY()

public:
	UCinematicIntroWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "UI|Intro Cinematic")
	void OpenIntro();

	UFUNCTION(BlueprintCallable, Category = "UI|Intro Cinematic")
	void PlayIntro();

	UFUNCTION(BlueprintCallable, Category = "UI|Intro Cinematic")
	void SkipIntro();

	UFUNCTION(BlueprintCallable, Category = "UI|Intro Cinematic")
	void CloseIntro();

	UFUNCTION(BlueprintCallable, Category = "UI|Intro Cinematic")
	void FinishIntro();

	UPROPERTY(Category = "UI|Intro Cinematic", BlueprintAssignable)
	FCinematicIntroFinishedDelegate OnIntroFinished;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "UI|Intro Cinematic")
	void PlayIntroAnimation();
	virtual void PlayIntroAnimation_Implementation();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Intro Cinematic")
	void OnIntroOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Intro Cinematic")
	void OnIntroClosed();

protected:
	void HandleWorldWidgetOpened_Implementation(EWorldWidgetType WorldWidgetType) override;
	bool HandleWorldWidgetCloseRequested_Implementation(EWorldWidgetType WorldWidgetType) override;
	void HandleWorldWidgetClosed_Implementation(EWorldWidgetType WorldWidgetType) override;

private:
	bool bIntroFinished = false;
};
