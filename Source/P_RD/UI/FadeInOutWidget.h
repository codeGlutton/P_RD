/**
 * @file FadeInOutWidget.h
 * @brief 룸 전환 시 화면 전환 타이밍을 UI 애니메이션 콜백으로 맞추는 페이드 위젯.
 *
 * @details
 * RoomTransitionSubsystem은 레벨 전환 준비 상태만 알고, 화면을 언제 검게 덮고 언제 다시 보여줄지는
 * UI가 결정해야 한다. 이 위젯은 OpenUI에서 페이드아웃을 완료한 뒤 전환 실행 콜백을 넘기고,
 * CloseUI에서 새 방 입장 후 페이드인을 수행하도록 분리한다.
 */

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"

#include "FadeInOutWidget.generated.h"

/**
 * @brief OpenUI는 검은 화면으로 페이드아웃하고 CloseUI는 다시 페이드인한다.
 *
 * WBP_FadeInOut이 없거나 애니메이션이 아직 비어 있어도 기본 검은 패널로 동작하게 하여
 * 전환 API 검증과 모바일 빌드 확인을 먼저 진행할 수 있게 한다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFadeInOutWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UFadeInOutWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	bool Initialize() override;
	void NativePreConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void PlayOpenUIAnimation_Implementation() override;
	void PlayCloseUIAnimation_Implementation() override;

private:
	void EnsureDefaultVisual();
	void StartFade(float StartAlpha, float EndAlpha, float Duration, bool bFinishOpen);
	void ApplyFadeAlpha(float Alpha);

private:
	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "FadeOutSeconds", AllowPrivateAccess = true))
	float mFadeOutSeconds = 0.22f;

	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "FadeInSeconds", AllowPrivateAccess = true))
	float mFadeInSeconds = 0.45f;

	float mFadeStartAlpha = 0.0f;
	float mFadeEndAlpha = 1.0f;
	float mFadeElapsedSeconds = 0.0f;
	float mFadeDurationSeconds = 0.0f;
	bool mFadeAnimationPlaying = false;
	bool mFinishOpenWhenFadeEnds = false;
};
