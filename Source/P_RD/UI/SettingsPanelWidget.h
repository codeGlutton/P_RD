/**
 * @file SettingsPanelWidget.h
 * @brief 타이틀과 인게임에서 함께 쓰는 설정 패널 위젯.
 *
 * @details
 * 같은 설정 화면을 타이틀 메뉴와 인게임 팝업에서 재사용하기 위해, 이 파일은 화면 입력과 표시 상태만 담당한다.
 * 실제 저장, 런 포기, 타이틀 이동은 이 위젯을 여는 쪽이 처리한다.
 */

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"

#include "SettingsPanelWidget.generated.h"

class UButton;
class UCheckBox;
class USlider;
class UTextBlock;
class UWidget;

/** @brief 설정 패널이 타이틀용인지 인게임용인지 구분한다. */
UENUM(BlueprintType)
enum class ESettingsPanelMode : uint8
{
	Title,
	InGame,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSettingsPanelEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSettingsPanelFloatEvent, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSettingsPanelBoolEvent, bool, bValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSettingsPanelIntEvent, int32, Value);

/**
 * @brief 공통 설정 UI의 표시 상태를 관리하고 버튼/슬라이더 입력을 이벤트로 전달한다.
 *
 * 타이틀과 인게임은 같은 WBP를 사용하며, 저장 후 종료/포기하기처럼 런 상태가 필요한 액션만
 * InGame 모드에서 표시한다.
 *
 * 이 위젯은 설정값 저장, 런 종료, 타이틀 이동을 직접 수행하지 않는다. 버튼 입력을 On... 이벤트로
 * 올려 보내면 TitleMenuWidget, TopMenuBarWidget, GameMode가 각 화면의 권한에 맞게 처리한다.
 *
 * 왜 이벤트만 내보내는가:
 * 설정 패널 안에 저장/전환 로직을 넣으면 타이틀에서 열린 패널도 인게임 런 상태를 알아야 한다.
 * 반대로 입력만 이벤트로 올리면 같은 WBP를 두 화면에서 공유하면서도, 실제 처리는 각 화면의 책임으로 남길 수 있다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API USettingsPanelWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 설정 패널의 기본 ZOrder와 모드를 초기화한다.
	 *
	 * @param ObjectInitializer Unreal 객체 생성에 사용하는 기본 초기화 값
	 */
	USettingsPanelWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** @brief 패널 모드를 바꾸고 모드별 영역 표시를 다시 맞춘다. */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void SetPanelMode(ESettingsPanelMode NewPanelMode);

	/**
	 * @brief 현재 설정 패널 모드를 반환한다.
	 *
	 * @return 타이틀/인게임 중 현재 모드
	 */
	UFUNCTION(BlueprintPure, Category = "UI|Settings")
	ESettingsPanelMode GetPanelMode() const;

	/** @brief 런 상태에 따라 저장 후 종료/포기하기 버튼 활성 상태를 갱신한다. */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void RefreshPanelState(bool bCanSaveRun, bool bCanAbandonRun);

	/** @brief 저장/포기 처리 결과처럼 외부 흐름이 결정한 상태 문구만 표시한다. */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void SetStatusText(const FText& Text) const;

	/** @brief 처리 중 중복 입력을 막기 위해 런 액션 버튼만 활성/비활성화한다. */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void SetRunActionsEnabled(bool bEnabled) const;

	/** @brief 런 포기 확인 패널을 보여준다. 실제 런 종료는 확정 이벤트 수신자가 수행한다. */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void ShowAbandonConfirm() const;

	/** @brief 런 포기 확인 패널을 숨긴다. */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void HideAbandonConfirm() const;

	/**
	 * @brief Back 버튼이 눌렸을 때 외부 화면 흐름에 복귀를 요청하는 이벤트
	 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelEvent OnBackRequested;

	/**
	 * @brief 저장 후 나가기 버튼이 눌렸을 때 외부 흐름으로 알리는 이벤트
	 *
	 * @details
	 * 패널은 저장/전환을 직접 실행하지 않는다. 인게임 탑바나 GameMode가 이 이벤트를 받아 권한 있는 흐름에서 처리한다.
	 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelEvent OnSaveAndExitRequested;

	/**
	 * @brief 런 포기 확인이 완료됐을 때 외부 흐름으로 알리는 이벤트
	 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelEvent OnAbandonRunConfirmed;

	/**
	 * @brief 설정 초기화 버튼이 눌렸을 때 외부 흐름으로 알리는 이벤트
	 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelEvent OnResetRequested;

	/** @brief BGM 볼륨 슬라이더 값 변경 이벤트 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelFloatEvent OnBgmVolumeChanged;

	/** @brief 효과음 볼륨 슬라이더 값 변경 이벤트 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelFloatEvent OnSfxVolumeChanged;

	/** @brief UI 볼륨 슬라이더 값 변경 이벤트 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelFloatEvent OnUiVolumeChanged;

	/** @brief 화면 흔들림 체크 상태 변경 이벤트 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelBoolEvent OnScreenShakeChanged;

	/** @brief 진동 체크 상태 변경 이벤트 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelBoolEvent OnVibrationChanged;

	/** @brief 그래픽 품질 선택 요청 이벤트 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelIntEvent OnQualityRequested;

protected:
	/**
	 * @brief WBP 바인딩 검증, 입력 이벤트 연결, 초기 표시 상태 동기화를 수행한다.
	 *
	 * @details
	 * 여기서 하는 일은 버튼/슬라이더를 이 클래스의 Handle... 함수에 연결하고, 현재 모드에 맞게 보일 영역을 정리하는 것이다.
	 * Handle... 함수들은 실제 저장이나 런 종료를 하지 않고 On... 이벤트를 Broadcast해 바깥 흐름에 요청만 전달한다.
	 *
	 * 왜 Construct 시점에 연결하는가:
	 * WBP가 실제로 생성된 뒤에야 버튼과 슬라이더 포인터가 유효하다. 이 시점에 연결해 두면 타이틀/인게임 어디서 열리든
	 * 같은 입력 경로를 쓰고, 없는 Optional 위젯은 건너뛰어 오래된 WBP와도 같이 동작할 수 있다.
	 */
	void NativeConstruct() override;

	/**
	 * @brief NativeConstruct()에서 연결한 WBP 입력 이벤트를 해제한다.
	 */
	void NativeDestruct() override;

private:
	/**
	 * @brief 설정 패널의 기본 표시 문구를 WBP TextBlock에 반영한다.
	 */
	void SyncText() const;

	/**
	 * @brief 타이틀/인게임 모드에 따라 런 액션 영역 표시를 전환한다.
	 *
	 * @details
	 * 같은 WBP 안에 인게임 전용 버튼까지 같이 두되, 타이틀 모드에서는 숨겨서 잘못된 런 액션을 누를 수 없게 한다.
	 *
	 * 왜 WBP를 둘로 나누지 않는가:
	 * 볼륨, 품질, 뒤로가기 같은 공통 설정은 두 화면에서 같아야 한다. 모드로 차이만 숨기면 공통 UI 수정이 한 곳에 모인다.
	 */
	void ApplyModeVisibility() const;

	/**
	 * @brief 현재 브랜치에서 사용하지 않는 언어 설정 영역을 숨긴다.
	 */
	void HideDeprecatedLanguageControls() const;

	/**
	 * @brief WBP_SettingsPanel에 필수 바인딩이 연결되어 있는지 로그로 확인한다.
	 */
	void ValidateDesignerBindings() const;

	/**
	 * @brief 버튼 포인터가 유효할 때만 활성 상태를 변경한다.
	 *
	 * @param Button 활성 상태를 변경할 버튼
	 * @param bEnabled 새 활성 상태
	 */
	void SetButtonEnabled(UButton* Button, bool bEnabled) const;

	UFUNCTION()
	void HandleBackButtonClicked();

	UFUNCTION()
	void HandleSaveAndExitButtonClicked();

	UFUNCTION()
	void HandleAbandonRunButtonClicked();

	UFUNCTION()
	void HandleConfirmAbandonButtonClicked();

	UFUNCTION()
	void HandleCancelAbandonButtonClicked();

	UFUNCTION()
	void HandleResetButtonClicked();

	UFUNCTION()
	void HandleLowQualityButtonClicked();

	UFUNCTION()
	void HandleMediumQualityButtonClicked();

	UFUNCTION()
	void HandleHighQualityButtonClicked();

	UFUNCTION()
	void HandleBgmVolumeChanged(float Value);

	UFUNCTION()
	void HandleSfxVolumeChanged(float Value);

	UFUNCTION()
	void HandleUiVolumeChanged(float Value);

	UFUNCTION()
	void HandleScreenShakeChanged(bool bChecked);

	UFUNCTION()
	void HandleVibrationChanged(bool bChecked);

private:
	/** @brief 설정 패널 제목 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsTitleText;

	/** @brief 외부 처리 결과나 안내 문구를 보여주는 상태 텍스트 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** @brief 이전 화면으로 돌아가는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

	/** @brief Back 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BackButtonText;

	/** @brief 저장 후 나가기 요청 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveAndExitButton;

	/** @brief 저장 후 나가기 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SaveAndExitButtonText;

	/** @brief 런 포기 확인 패널을 여는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> AbandonRunButton;

	/** @brief 런 포기 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AbandonRunButtonText;

	/** @brief 설정값 초기화 요청 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResetButton;

	/** @brief 초기화 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResetButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AudioSectionHeader;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MasterVolumeRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BGMVolumeRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SFXVolumeRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UIVolumeRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DisplaySectionHeader;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BrightnessRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScreenShakeRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VibrationRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QualityRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GameplaySectionHeader;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FastModeRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkipAnimationRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AutoEndTurnRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InfoSectionHeader;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CreditsRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LicenseRow_Label;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CreditsOpenButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LicenseOpenButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> LanguageSectionHeader;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> LanguageRow;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LowQualityButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LowQualityButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MediumQualityButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MediumQualityButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HighQualityButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HighQualityButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> BgmVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> SfxVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> UiVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> ScreenShakeCheckBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> VibrationCheckBox;

	/** @brief 인게임에서만 사용하는 저장/포기 버튼 묶음 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RunActionsPanel;

	/** @brief 런 포기 여부를 한 번 더 확인하는 패널 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> AbandonConfirmPanel;

	/** @brief 런 포기 확인 패널 제목 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AbandonConfirmTitleText;

	/** @brief 런 포기 확인 설명 문구 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AbandonConfirmBodyText;

	/** @brief 런 포기를 확정하는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmAbandonButton;

	/** @brief 런 포기 확정 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ConfirmAbandonButtonText;

	/** @brief 런 포기 확인을 취소하는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelAbandonButton;

	/** @brief 런 포기 취소 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CancelAbandonButtonText;

	/**
	 * @brief 현재 패널 표시 모드
	 *
	 * @details
	 * 같은 WBP를 타이틀과 인게임에서 공유하기 위해 모드로 런 액션 영역 표시만 분기한다.
	 */
	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	ESettingsPanelMode mPanelMode = ESettingsPanelMode::Title;
};
