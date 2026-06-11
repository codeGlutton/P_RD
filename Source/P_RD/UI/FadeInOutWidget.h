/**
 * @file FadeInOutWidget.h
 * @brief 룸 전환 시 화면 전환 타이밍을 UI 애니메이션 콜백으로 맞추는 페이드 위젯.
 *
 * @details
 * RoomTransitionSubsystem은 레벨 전환 준비 상태만 알고, 화면을 언제 검게 덮고 언제 다시 보여줄지는
 * UI가 결정해야 한다. 이 위젯은 OpenUI로 전환 레이어를 뷰포트에 올린 뒤,
 * StartFadeOut/StartFadeIn으로 실제 페이드 방향을 분리해서 실행한다.
 *
 * 왜 OpenUI와 페이드 함수를 나누는가:
 * OpenUI는 모든 UI가 공유하는 표시 생명주기이고, 페이드인/페이드아웃은 이 위젯만 가진 연출이다.
 * 두 책임을 나누면 GameMode는 위젯을 여는 일과 전환 화면을 덮거나 걷는 일을 순서대로 명시할 수 있다.
 */

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"

#include "FadeInOutWidget.generated.h"

class UFadeInOutWidget;

DECLARE_DELEGATE_OneParam(FOnEndFadeInAnimation, UFadeInOutWidget*)
DECLARE_DELEGATE_OneParam(FOnEndFadeOutAnimation, UFadeInOutWidget*)

enum class EFadeInOutAnimationType : uint8
{
	None,
	FadeIn,
	FadeOut,
};

/**
 * @brief 전환 레이어 표시와 실제 페이드 방향을 분리해서 처리한다.
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

	/**
	 * @brief 검은 화면에서 게임 화면으로 다시 드러내는 페이드인을 시작한다.
	 *
	 * @param OnEndFadeInAnimation 페이드인이 끝난 뒤 실행할 콜백
	 */
	void StartFadeIn(FOnEndFadeInAnimation OnEndFadeInAnimation = FOnEndFadeInAnimation());

	/**
	 * @brief 전환이 이어지기 전 화면을 검게 덮는 페이드아웃을 시작한다.
	 *
	 * @param OnEndFadeOutAnimation 페이드아웃이 끝난 뒤 실행할 콜백
	 */
	void StartFadeOut(FOnEndFadeOutAnimation OnEndFadeOutAnimation = FOnEndFadeOutAnimation());

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
	 * @brief 공통 OpenUI 생명주기를 즉시 완료한다.
	 */
	void PlayOpenUIAnimation_Implementation() override;

	/**
	 * @brief 공통 CloseUI 생명주기를 즉시 완료한다.
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
	 * @param FadeAnimationType 완료 시 실행할 페이드 콜백 종류
	 */
	void StartFade(float StartAlpha, float EndAlpha, float Duration, EFadeInOutAnimationType FadeAnimationType);

	/**
	 * @brief 현재 페이드 요청을 완료하고 방향에 맞는 콜백을 실행한다.
	 */
	void FinishFade();

	/**
	 * @brief 계산된 페이드 투명도를 위젯 전체에 반영한다.
	 *
	 * @param Alpha 적용할 투명도
	 */
	void ApplyFadeAlpha(float Alpha);

private:
	/**
	 * @brief StartFadeOut()에서 검은 화면까지 덮는 데 걸리는 시간
	 */
	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "FadeOutSeconds", AllowPrivateAccess = true))
	float mFadeOutSeconds = 0.22f;

	/**
	 * @brief StartFadeIn()에서 다시 게임 화면을 보여주는 데 걸리는 시간
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

	/** @brief 현재 실행 중인 페이드 방향 */
	EFadeInOutAnimationType mFadeAnimationType = EFadeInOutAnimationType::None;

	/** @brief 페이드인 완료 시 실행할 콜백 */
	FOnEndFadeInAnimation OnEndFadeInAnimation;

	/** @brief 페이드아웃 완료 시 실행할 콜백 */
	FOnEndFadeOutAnimation OnEndFadeOutAnimation;
};
