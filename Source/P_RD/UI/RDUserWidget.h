#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"

#include "RDUserWidget.generated.h"

class UButton;
class USoundBase;

/**
 * @brief UI 열기 애니메이션이 끝났을 때 호출되는 콜백
 *
 * @details
 * OpenUI() 호출자는 위젯이 실제로 열린 뒤 이어서 실행할 작업을 이 콜백으로 넘긴다.
 * 애니메이션을 쓰지 않는 위젯도 같은 호출 규칙을 유지하기 위해 FinishOpenUI()에서 실행된다.
 */
DECLARE_DELEGATE_OneParam(FOnEndUIOpenAnimation, UUserWidget*)

/**
 * @brief UI 닫기 애니메이션이 끝났을 때 호출되는 콜백
 *
 * @details
 * CloseUI() 호출자는 위젯이 닫힌 뒤 이어서 실행할 작업을 이 콜백으로 넘긴다.
 * 닫기 연출이 없는 위젯도 같은 호출 규칙을 유지하기 위해 FinishCloseUI()에서 실행된다.
 */
DECLARE_DELEGATE_OneParam(FOnEndUICloseAnimation, UUserWidget*)

/**
 * @brief RD 프로젝트 UMG 위젯의 공통 베이스 클래스
 *
 * @details
 * 모든 화면 위젯을 같은 방식으로 열고 닫기 위한 프로젝트 단위의 UI 생명주기 베이스다.
 * 기존 이름이 특정 동작을 암시했다면, 이 클래스는 "토글 가능"보다 넓은 의미로
 * 화면 표시, 뷰포트 등록, 열기/닫기 애니메이션 완료 콜백, 닫힌 뒤 유지/제거 정책을 한곳에 모은다.
 *
 * 의도는 위젯 사용자가 AddToViewport(), SetVisibility(), RemoveFromParent()를 직접 섞어 쓰지 않고
 * OpenUI()/CloseUI()만 호출하게 만드는 것이다. 그래야 HUD, 월드 공용 위젯, 팝업 위젯이 같은 규칙으로
 * 열리고 닫히며, Blueprint 애니메이션을 붙인 위젯과 C++ 기본 위젯의 완료 타이밍도 동일하게 다룰 수 있다.
 *
 * 파생 WBP는 PlayOpenUIAnimation()/PlayCloseUIAnimation()을 Blueprint에서 구현하고,
 * 애니메이션 마지막에 FinishOpenUI()/FinishCloseUI()를 호출하면 된다. 애니메이션을 구현하지 않으면
 * 기본 구현이 즉시 Finish 함수를 호출한다.
 *
 * 왜 이렇게 하는가:
 * 각 WBP가 AddToViewport(), Visibility, 애니메이션 완료 콜백을 제각각 처리하면 어떤 화면은 보이는데
 * GameMode는 아직 닫혔다고 판단하거나, 반대로 이미 열린 위젯을 또 열어 입력이 꼬일 수 있다.
 * 그래서 프로젝트 UI는 먼저 이 베이스를 통과하게 해서 "열림/닫힘/완료"의 의미를 하나로 맞춘다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API URDUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 기본 표시 상태를 닫힘으로 초기화한다.
	 *
	 * @details
	 * UUserWidget 생성 직후에는 아직 게임 흐름에서 열기 요청을 받은 상태가 아니다.
	 * 생성자에서 Collapsed로 시작해 두면 Designer 배치나 서브시스템 초기화 과정에서
	 * 의도치 않게 화면이 먼저 보이는 일을 막을 수 있다.
	 *
	 * @param ObjectInitializer Unreal 객체 생성에 사용하는 기본 초기화 값
	 */
	URDUserWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * @brief 위젯을 여는 단일 진입점
	 *
	 * @details
	 * 뷰포트 등록, 표시 상태 변경, 열기 애니메이션 시작, 완료 콜백 저장을 한 번에 처리한다.
	 * 이미 열린 위젯에 다시 요청이 들어오면 화면 상태를 건드리지 않고 콜백만 즉시 실행한다.
	 * 호출부가 위젯의 현재 상태를 별도로 분기하지 않아도 되게 하기 위한 정책이다.
	 * 호출부를 단순하게 두는 이유는 UI를 여는 쪽이 "이 위젯이 이미 AddToViewport 됐는지"까지 알기 시작하면
	 * 같은 화면이라도 GameMode, TopBar, Subsystem마다 서로 다른 열기 규칙이 생기기 때문이다.
	 *
	 * @param Callback 열기 애니메이션 완료 후 실행할 선택 콜백
	 */
	virtual void OpenUI(FOnEndUIOpenAnimation Callback = FOnEndUIOpenAnimation());

	/**
	 * @brief 위젯을 닫는 단일 진입점
	 *
	 * @details
	 * 닫기 애니메이션을 시작하고, 완료 시 ApplyCloseUI()를 통해 Collapsed 처리 또는 부모 제거를 수행한다.
	 * 이미 닫힌 위젯은 콜백을 즉시 실행하고, 닫히는 중인 위젯은 중복 닫기 요청을 무시한다.
	 * 닫기 연출과 후속 로직이 여러 번 실행되는 것을 막기 위한 생명주기 방어다.
	 *
	 * @param Callback 닫기 애니메이션 완료 후 실행할 선택 콜백
	 */
	virtual void CloseUI(FOnEndUICloseAnimation Callback = FOnEndUICloseAnimation());

	/**
	 * @brief 위젯이 열렸거나 열리는 중이며 실제 화면에 보이는지 확인한다.
	 *
	 * @return 열린 상태로 취급할 수 있으면 true
	 */
	UFUNCTION(BlueprintPure, Category = "UI|Toggleable")
	virtual bool IsOpened() const;

	/**
	 * @brief 열기 애니메이션 완료를 C++ 생명주기에 알린다.
	 *
	 * @details
	 * Blueprint에서 PlayOpenUIAnimation()을 구현했다면 애니메이션 마지막에 이 함수를 호출해야 한다.
	 * 이 함수가 호출되어야 상태가 Open으로 고정되고 OpenUI()의 완료 콜백이 실행된다.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Toggleable")
	void FinishOpenUI();

	/**
	 * @brief 닫기 애니메이션 완료를 C++ 생명주기에 알린다.
	 *
	 * @details
	 * Blueprint에서 PlayCloseUIAnimation()을 구현했다면 애니메이션 마지막에 이 함수를 호출해야 한다.
	 * 이 함수가 호출되어야 실제 닫힘 처리가 적용되고 CloseUI()의 완료 콜백이 실행된다.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Toggleable")
	void FinishCloseUI();

protected:
	/**
	 * @brief 열기 애니메이션을 재생한다.
	 *
	 * @details
	 * WBP마다 다른 연출을 붙일 수 있도록 BlueprintNativeEvent로 둔다.
	 * 별도 구현이 없으면 기본 구현이 즉시 FinishOpenUI()를 호출해 콜백 규칙을 유지한다.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "UI|Toggleable")
	void PlayOpenUIAnimation();
	virtual void PlayOpenUIAnimation_Implementation();

	/**
	 * @brief 닫기 애니메이션을 재생한다.
	 *
	 * @details
	 * WBP마다 다른 연출을 붙일 수 있도록 BlueprintNativeEvent로 둔다.
	 * 별도 구현이 없으면 기본 구현이 즉시 FinishCloseUI()를 호출해 콜백 규칙을 유지한다.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "UI|Toggleable")
	void PlayCloseUIAnimation();
	virtual void PlayCloseUIAnimation_Implementation();

	/**
	 * @brief OpenUI()가 확정한 열기 상태를 실제 UMG 표시 상태에 반영한다.
	 */
	virtual void ApplyOpenUI();

	/**
	 * @brief FinishCloseUI() 이후 닫힘 정책을 실제 UMG 표시 상태에 반영한다.
	 */
	virtual void ApplyCloseUI();

	/**
	 * @brief AddToViewport()에 사용할 ZOrder를 반환한다.
	 *
	 * @return 뷰포트 표시 순서
	 */
	virtual int32 GetViewportZOrder() const;

	/**
	 * @brief 닫을 때 부모에서 제거할지, Collapsed 상태로 유지할지 결정한다.
	 *
	 * @return RemoveFromParent()로 제거해야 하면 true
	 */
	virtual bool ShouldRemoveFromParentOnClose() const;

	/**
	 * @brief 이 위젯의 버튼에 공용 누름 효과(어둡게+축소)를 적용할지 여부. 기본 true.
	 * @details 모든 화면에서 동일한 눌림 피드백을 보장한다. 특수 화면만 필요하면
	 *          override로 끌 수 있다.
	 */
	virtual bool ShouldApplyButtonFeedback() const;

	/**
	 * @brief 위젯 초기화 시(피드백을 켠 화면에 한해) 트리 안의 버튼에 공용 누름 효과를 1회 배선한다.
	 */
	virtual void NativeOnInitialized() override;

private:
	/** @brief 이 위젯 트리의 버튼들을 찾아 누름 효과를 설정하고 눌림/뗌 이벤트를 연결한다. */
	void SetupCommonButtonFeedback();

	/**
	 * @brief 버튼과 짝지어진 "보이는" 동반 위젯(프레임 이미지/텍스트)을 이름 규칙으로 찾는다.
	 * @details 투명 히트영역과 실제 프레임이 형제인 경우를 위해서다.
	 *          규칙: "<Base>Button<Suffix>"에 대해 "<Base>...<Suffix>" Image/Text를 짝으로 본다.
	 */
	void CollectButtonCompanions(const UButton* Button, TArray<UWidget*>& OutCompanions) const;

	/** @brief 현재 눌림 상태로 어둡게/축소된 동반 위젯들을 원래대로 복원하고 캐시를 비운다. */
	void RestoreActivePressCompanions();

	/** @brief 아무 버튼이나 눌리는 순간: 눌린 버튼을 살짝 축소해 "눌리는 느낌"을 준다. */
	UFUNCTION()
	void HandleAnyButtonPressed();

	/** @brief 버튼에서 손을 떼는 순간: 모든 관리 버튼의 스케일을 원래대로 되돌린다. */
	UFUNCTION()
	void HandleAnyButtonReleased();

	/** @brief 누름 피드백 대상으로 캐시해 둔 버튼들(이 위젯 트리 소속). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> mFeedbackButtons;

	/** @brief 각 버튼의 원래 배경색(멀티플라이어). 누르면 이 값을 어둡게, 떼면 이 값으로 복원한다. mFeedbackButtons와 인덱스 대응. */
	TArray<FLinearColor> mFeedbackButtonBaseColors;

	/** @brief 지금 눌려서 어둡게/축소된 동반 위젯들(한 번에 한 버튼). 떼면 복원 후 비운다. 트리가 소유하므로 약참조. */
	TArray<TWeakObjectPtr<UWidget>> mActivePressCompanions;

	/** @brief mActivePressCompanions의 원래 색(복원용). 인덱스 대응. */
	TArray<FLinearColor> mActivePressCompanionBaseColors;

	/** @brief 공용 버튼 클릭 사운드(생성자에서 하드레퍼런스 로드). 버튼 스타일 PressedSlateSound에 주입한다. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> mCommonButtonPressSound;

protected:
	/**
	 * @brief 뷰포트에 올라갈 때 사용할 표시 순서
	 *
	 * @details
	 * 공통 OpenUI() 경로를 쓰면서도 HUD, 팝업, 로딩 UI의 쌓임 순서를 WBP 기본값으로 조정할 수 있게 둔다.
	 */
	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "ViewportZOrder"))
	int32 mViewportZOrder = 0;

	/**
	 * @brief CloseUI() 완료 시 부모에서 제거할지 여부
	 *
	 * @details
	 * 자주 다시 열리는 HUD는 Collapsed로 남기는 편이 낫고, 일회성 팝업은 제거하는 편이 낫다.
	 * 위젯별 비용과 상태 보존 필요성이 다르므로 공통 베이스에서 정책만 제공한다.
	 */
	UPROPERTY(Category = UI, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "RemoveFromParentOnClose"))
	bool mRemoveFromParentOnClose = false;

private:
	/**
	 * @brief OpenUI()/CloseUI() 중복 호출을 정리하기 위한 내부 생명주기 상태
	 */
	enum class ERDUserWidgetLifecycleState : uint8
	{
		Closed,
		Opening,
		Open,
		Closing,
	};

	/** @brief 열기 완료 시 실행할 대기 콜백 */
	FOnEndUIOpenAnimation OnEndUIOpenAnimation;

	/** @brief 닫기 완료 시 실행할 대기 콜백 */
	FOnEndUICloseAnimation OnEndUICloseAnimation;

	/** @brief 현재 열기/닫기 진행 상태 */
	ERDUserWidgetLifecycleState mLifecycleState = ERDUserWidgetLifecycleState::Closed;
};
