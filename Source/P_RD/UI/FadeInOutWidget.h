/**
 * @file FadeInOutWidget.h
 * @brief 룸 전환 시 화면 전환 타이밍을 UI 애니메이션 콜백으로 맞추는 페이드 위젯.
 *
 * @details
 * RoomTransitionSubsystem은 레벨 전환 준비 상태만 알고, 화면을 언제 검게 덮고 언제 다시 보여줄지는
 * UI가 결정해야 한다. 이 위젯은 OpenUI에서 페이드아웃을 완료한 뒤 전환 실행 콜백을 넘기고,
 * CloseUI에서 새 방 입장 후 페이드인을 수행하도록 분리한다.
 *
 * 왜 OpenUI가 fade out이고 CloseUI가 fade in인가:
 * 전환 직전에는 화면을 덮는 일이 "페이드 UI를 여는 것"이고, 새 방에 들어온 뒤에는 그 덮개를 닫으며
 * 게임 화면을 다시 보여준다. 이렇게 맞추면 GameMode는 모든 위젯을 OpenUI/CloseUI 규칙으로만 다룰 수 있다.
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
 *
 * 왜 native fallback을 두는가:
 * WBP 작업이 늦어지거나 패키징에서 위젯 바인딩이 비어 있어도 전환 순서 자체는 검증할 수 있어야 한다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFadeInOutWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 페이드 위젯의 기본 뷰포트 ZOrder를 전환 레이어로 초기화한다.
	 *
	 * @param ObjectInitializer Unreal 객체 생성에 사용하는 기본 초기화 값
	 */
	UFadeInOutWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/**
	 * @brief 런타임에서 WBP 루트가 없을 때 최소 기본 화면을 준비한다.
	 *
	 * @return UUserWidget 초기화 성공 여부
	 */
	bool Initialize() override;

	/**
	 * @brief 에디터 미리보기에서도 native fallback 화면을 확인할 수 있게 한다.
	 */
	void NativePreConstruct() override;

	/**
	 * @brief tick 기반 페이드 진행률을 갱신하고 완료 시 OpenUI/CloseUI 생명주기를 끝낸다.
	 *
	 * @param MyGeometry 현재 위젯 지오메트리
	 * @param InDeltaTime 이전 프레임 이후 경과 시간
	 */
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 * @brief OpenUI() 요청을 검은 화면으로 덮는 페이드아웃으로 해석한다.
	 */
	void PlayOpenUIAnimation_Implementation() override;

	/**
	 * @brief CloseUI() 요청을 화면을 다시 드러내는 페이드인으로 해석한다.
	 */
	void PlayCloseUIAnimation_Implementation() override;

private:
	/**
	 * @brief WBP 루트가 없을 때 전체 화면 검은 패널을 생성한다.
	 */
	void EnsureDefaultVisual();

	/**
	 * @brief 하나의 완료 경로로 페이드 애니메이션을 시작한다.
	 *
	 * @param StartAlpha 시작 투명도
	 * @param EndAlpha 목표 투명도
	 * @param Duration 페이드에 사용할 시간
	 * @param bFinishOpen 완료 시 FinishOpenUI()를 호출해야 하면 true, FinishCloseUI()를 호출해야 하면 false
	 */
	void StartFade(float StartAlpha, float EndAlpha, float Duration, bool bFinishOpen);

	/**
	 * @brief 계산된 페이드 투명도를 위젯 전체에 반영한다.
	 *
	 * @param Alpha 적용할 투명도
	 */
	void ApplyFadeAlpha(float Alpha);

private:
	/**
	 * @brief OpenUI()에서 검은 화면까지 덮는 데 걸리는 시간
	 */
	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "FadeOutSeconds", AllowPrivateAccess = true))
	float mFadeOutSeconds = 0.22f;

	/**
	 * @brief CloseUI()에서 다시 게임 화면을 보여주는 데 걸리는 시간
	 */
	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "FadeInSeconds", AllowPrivateAccess = true))
	float mFadeInSeconds = 0.45f;

	/** @brief 현재 페이드의 시작 투명도 */
	float mFadeStartAlpha = 0.0f;

	/** @brief 현재 페이드의 목표 투명도 */
	float mFadeEndAlpha = 1.0f;

	/** @brief 현재 페이드가 진행된 시간 */
	float mFadeElapsedSeconds = 0.0f;

	/** @brief 현재 페이드의 전체 시간 */
	float mFadeDurationSeconds = 0.0f;

	/** @brief tick에서 페이드를 갱신해야 하는지 여부 */
	bool mFadeAnimationPlaying = false;

	/** @brief 페이드 완료 시 OpenUI 완료로 끝낼지 CloseUI 완료로 끝낼지 결정하는 플래그 */
	bool mFinishOpenWhenFadeEnds = false;
};
