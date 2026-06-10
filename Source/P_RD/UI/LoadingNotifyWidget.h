/**
 * @file LoadingNotifyWidget.h
 * @brief 룸 전환 중 로딩 상태를 최소 시간 동안 보여주는 알림 위젯.
 *
 * @details
 * Preload가 매우 빨리 끝나면 사용자는 화면 전환이 눌렸는지 알기 어렵다. 이 위젯은 로딩중 상태를
 * 최소 표시 시간 동안 유지한 뒤, CloseUI에서 로딩완료 상태와 닫힘 애니메이션을 끝내고 콜백을 실행한다.
 *
 * 왜 최소 표시 시간이 필요한가:
 * 로딩이 너무 빨리 끝나면 버튼을 눌렀는데 아무 반응 없이 화면만 바뀐 것처럼 보일 수 있다.
 * 짧게라도 로딩중/완료 상태를 보여주면 입력이 처리됐고 다음 흐름으로 넘어간다는 신호가 생긴다.
 */

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"

#include "LoadingNotifyWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * @brief 최소 표시 시간, 로딩중/로딩완료 문구, 점멸 인디케이터를 관리한다.
 *
 * 전환 흐름의 실제 완료 여부는 GameMode/RoomTransitionSubsystem이 결정하고, 이 위젯은 사용자가 볼
 * 로딩 표현과 닫힘 완료 콜백만 담당한다.
 *
 * 왜 전환 완료 판단을 하지 않는가:
 * 위젯이 로드 상태까지 판단하면 UI가 RoomTransitionSubsystem의 내부 규칙을 알아야 한다.
 * 여기서는 "보여주고 닫힌다"까지만 책임져야 같은 알림을 다른 전환에서도 재사용할 수 있다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API ULoadingNotifyWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 로딩 알림 위젯의 기본 뷰포트 ZOrder를 로딩 레이어로 초기화한다.
	 *
	 * @param ObjectInitializer Unreal 객체 생성에 사용하는 기본 초기화 값
	 */
	ULoadingNotifyWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
	 * @brief 로딩 중 상태의 인디케이터 점멸을 갱신한다.
	 *
	 * @param MyGeometry 현재 위젯 지오메트리
	 * @param InDeltaTime 이전 프레임 이후 경과 시간
	 */
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 * @brief OpenUI() 요청을 로딩 중 상태 진입으로 해석한다.
	 */
	void PlayOpenUIAnimation_Implementation() override;

	/**
	 * @brief CloseUI() 요청을 최소 표시 시간과 완료 표시를 거친 닫힘으로 해석한다.
	 */
	void PlayCloseUIAnimation_Implementation() override;

private:
	/**
	 * @brief 로딩 알림이 화면에 표시하는 내부 단계
	 */
	enum class ELoadingNotifyState : uint8
	{
		/** @brief 표시할 로딩 상태가 없는 닫힘 단계 */
		None,

		/** @brief 프리로드 또는 전환 준비가 진행 중인 단계 */
		Loading,

		/** @brief 준비가 끝났고 닫힘 콜백 전 완료 문구를 보여주는 단계 */
		Completed,
	};

	/**
	 * @brief WBP 루트가 없을 때 기본 로딩 인디케이터와 텍스트를 생성한다.
	 */
	void EnsureDefaultVisual();

	/**
	 * @brief 로딩 단계에 맞춰 인디케이터 표시와 상태 문구를 갱신한다.
	 *
	 * @param NewState 새로 적용할 로딩 표시 단계
	 */
	void SetLoadingState(ELoadingNotifyState NewState);

	/**
	 * @brief 기본 인디케이터가 있을 때 점멸 투명도를 반영한다.
	 *
	 * @param Alpha 적용할 투명도
	 */
	void ApplyIndicatorAlpha(float Alpha) const;

	/**
	 * @brief 최소 표시 시간이 끝난 뒤 완료 상태를 보여준다.
	 */
	void ShowCompletedState();

	/**
	 * @brief 완료 표시와 닫힘 지연이 끝난 뒤 CloseUI 생명주기를 완료한다.
	 */
	void FinishCompletedState();

private:
	/**
	 * @brief 기본 WBP 또는 native fallback에서 점멸시킬 로딩 인디케이터
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> mLoadingIndicator;

	/**
	 * @brief 로딩 중/로딩 완료 문구를 표시하는 텍스트 위젯
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mLoadingStatusText;

	/**
	 * @brief 로딩 UI가 너무 빨리 사라지지 않도록 보장할 최소 표시 시간
	 */
	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "MinimumVisibleSeconds", AllowPrivateAccess = true))
	float mMinimumVisibleSeconds = 1.0f;

	/**
	 * @brief 로딩 완료 문구를 보여줄 시간
	 */
	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "CompletedVisibleSeconds", AllowPrivateAccess = true))
	float mCompletedVisibleSeconds = 0.35f;

	/**
	 * @brief 완료 문구 이후 CloseUI 완료까지 기다릴 닫힘 연출 시간
	 */
	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "CloseAnimationSeconds", AllowPrivateAccess = true))
	float mCloseAnimationSeconds = 0.25f;

	/**
	 * @brief 로딩 인디케이터 점멸 속도
	 */
	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "IndicatorBlinkSpeed", AllowPrivateAccess = true))
	float mIndicatorBlinkSpeed = 2.0f;

	/** @brief 최소 표시 시간이 끝나는 시점을 예약하는 타이머 */
	FTimerHandle mMinimumVisibleTimerHandle;

	/** @brief 완료 문구 표시가 끝나는 시점을 예약하는 타이머 */
	FTimerHandle mCompletedVisibleTimerHandle;

	/** @brief 닫힘 연출 시간까지 포함한 CloseUI 완료 예약 타이머 */
	FTimerHandle mCloseAnimationTimerHandle;

	/** @brief OpenUI가 시작된 월드 시간 */
	double mOpenedTimeSeconds = 0.0;

	/** @brief 인디케이터 점멸 계산에 사용하는 누적 시간 */
	float mIndicatorElapsedSeconds = 0.0f;

	/** @brief 현재 로딩 알림 표시 단계 */
	ELoadingNotifyState mLoadingState = ELoadingNotifyState::None;
};
