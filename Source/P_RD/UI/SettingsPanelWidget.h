/**
 * @file SettingsPanelWidget.h
 * @brief 타이틀과 인게임에서 함께 쓰는 설정 패널 위젯.
 *
 * @details
 * 같은 설정 화면을 타이틀 메뉴와 인게임 팝업에서 재사용하기 위해, 이 파일은 화면 입력과 표시 상태만 담당한다.
 * 실제 저장, 런 포기, 타이틀 이동은 이 위젯을 여는 쪽이 처리한다.
 *
 * 설정 패널은 "공통 UI 껍데기" 역할이다. 버튼, 슬라이더, 체크박스 같은 WBP 입력을 C++ 이벤트로 바꿔서
 * 밖으로 내보내고, 현재 화면이 타이틀인지 인게임인지에 따라 필요한 영역만 보여준다.
 * 그래서 이 클래스 안에는 세이브 파일 작성, 방 전환, 런 폐기 같은 게임 진행 로직을 넣지 않는다.
 */

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/SettingsPanelTypes.h"

#include "SettingsPanelWidget.generated.h"

class UButton;
class UCheckBox;
class USlider;
class UTextBlock;
class UWidget;

/**
 * @brief 공통 설정 UI의 표시 상태를 관리하고 버튼/슬라이더 입력을 이벤트로 전달한다.
 *
 * 타이틀과 인게임은 같은 WBP를 사용하며, 저장 후 종료/포기하기처럼 런 상태가 필요한 액션만
 * InGame 모드에서 표시한다.
 *
 * 이 위젯은 설정값 저장, 런 종료, 타이틀 이동을 직접 수행하지 않는다. 버튼 입력을 On... 이벤트로
 * 올려 보내면 TitleMenuWidget, 전투 HUD, GameMode가 각 화면의 권한에 맞게 처리한다.
 *
 * 왜 이벤트만 내보내는가:
 * 설정 패널 안에 저장/전환 로직을 넣으면 타이틀에서 열린 패널도 인게임 런 상태를 알아야 한다.
 * 반대로 입력만 이벤트로 올리면 같은 WBP를 두 화면에서 공유하면서도, 실제 처리는 각 화면의 책임으로 남길 수 있다.
 *
 * 기능 범위:
 * 이 클래스는 WBP 바인딩, 입력 이벤트 변환, 모드별 영역 표시, 확인 패널 표시만 맡는다.
 * 설정 값 적용, 세이브 파일 작성, 현재 런 폐기, 타이틀 이동은 이 위젯의 책임이 아니다.
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

	/** @brief 설정창도 프론트엔드 UI라 공용 버튼 클릭 사운드/누름 효과를 적용한다(타이틀/클래스선택과 동일). */
	virtual bool ShouldApplyButtonFeedback() const override { return true; }

	/**
	 * @brief 패널 모드를 바꾸고 모드별 영역 표시를 다시 맞춘다.
	 *
	 * @details
	 * 같은 설정 패널을 타이틀과 인게임에서 모두 쓰기 때문에, 호출한 쪽이 자신에게 맞는 모드를 지정한다.
	 * 모드가 바뀌면 런 액션 패널처럼 화면별로 달라지는 영역의 표시 상태만 갱신한다.
	 * 모드 변경은 UI 표시 정책일 뿐이며, 실제 타이틀 이동이나 런 상태 조회를 수행하지 않는다.
	 *
	 * @param NewPanelMode 새 설정 패널 표시 모드
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void SetPanelMode(ESettingsPanelMode NewPanelMode);

	/**
	 * @brief 현재 설정 패널 모드를 반환한다.
	 *
	 * @return 타이틀/인게임 중 현재 모드
	 */
	UFUNCTION(BlueprintPure, Category = "UI|Settings")
	ESettingsPanelMode GetPanelMode() const;

	/**
	 * @brief 런 상태에 따라 저장 후 종료/포기하기 버튼 활성 상태를 갱신한다.
	 *
	 * @details
	 * 저장 가능 여부와 런 포기 가능 여부는 GameMode나 저장 시스템이 판단한다.
	 * 설정 패널은 그 판단 결과를 받아 버튼 상태만 바꾼다.
	 *
	 * 왜 이렇게 나누는가:
	 * UI가 직접 런 상태를 검사하면 설정 패널이 특정 GameMode와 저장 구조에 묶인다.
	 * bool 결과만 받으면 타이틀에서는 런 액션을 숨기고, 인게임에서는 외부 정책에 맞게 버튼만 표시할 수 있다.
	 *
	 * @param bCanSaveRun 저장 후 종료 버튼을 누를 수 있는지 여부
	 * @param bCanAbandonRun 런 포기 버튼을 누를 수 있는지 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void RefreshPanelState(bool bCanSaveRun, bool bCanAbandonRun);

	/**
	 * @brief 저장/포기 처리 결과처럼 외부 흐름이 결정한 상태 문구만 표시한다.
	 *
	 * @details
	 * 저장 중, 저장 실패, 런 포기 불가 같은 문구는 실제 처리를 수행한 쪽에서 가장 정확히 알 수 있다.
	 * 그래서 패널은 문구를 만들지 않고 전달받은 텍스트를 보여주며, 빈 텍스트가 들어오면 상태 영역을 숨긴다.
	 * 이 함수는 입력 잠금이나 화면 전환을 동반하지 않는 순수 표시 API다.
	 *
	 * @param Text 상태 영역에 표시할 문구
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void SetStatusText(const FText& Text) const;

	/**
	 * @brief 외부 설정 시스템에서 전달한 현재 설정 값을 패널 입력 위젯에 반영한다.
	 *
	 * @details
	 * 패널은 값을 저장하지 않고 화면 상태만 맞춘다. 값 적용 중 발생하는 위젯 변경 콜백은 외부 이벤트로 다시 내보내지 않는다.
	 *
	 * @param ValueModel 패널에 반영할 설정 값 묶음
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void ApplyValueModel(const FSettingsPanelValueModel& ValueModel);

	/**
	 * @brief 현재 저장/프로필 옵션 값을 읽어 설정 패널 입력 상태에 다시 반영한다.
	 *
	 * @details
	 * 설정창이 열릴 때 저장된 볼륨, 언어, FPS 제한을 화면에 맞추는 진입점이다.
	 * 값 적용 중 발생하는 슬라이더/체크박스 콜백은 외부 이벤트로 다시 나가지 않는다.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void RefreshValueModelFromCurrentOptions();

	/**
	 * @brief 패널이 마지막으로 받은/입력한 설정 값 묶음을 반환한다.
	 */
	UFUNCTION(BlueprintPure, Category = "UI|Settings")
	FSettingsPanelValueModel GetValueModel() const;

	/**
	 * @brief 처리 중 중복 입력을 막기 위해 런 액션 버튼만 활성/비활성화한다.
	 *
	 * @details
	 * 저장 후 종료나 런 포기 처리가 진행 중일 때 같은 요청이 연속으로 들어오면 저장/전환 흐름이 꼬일 수 있다.
	 * 공통 설정 조작은 그대로 두고, 런 흐름에 직접 영향을 주는 버튼만 잠근다.
	 *
	 * @param bEnabled true면 런 액션 버튼을 활성화하고 false면 비활성화한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void SetRunActionsEnabled(bool bEnabled) const;

	/**
	 * @brief 런 포기 확인 패널을 보여준다.
	 *
	 * @details
	 * 런 포기는 되돌리기 어려운 액션이므로 첫 버튼 클릭에서는 바로 실행하지 않고 확인 패널만 연다.
	 * 실제 런 종료는 Confirm 버튼이 눌린 뒤 OnAbandonRunConfirmed를 받은 외부 흐름이 수행한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void ShowAbandonConfirm() const;

	/**
	 * @brief 런 포기 확인 패널을 숨긴다.
	 *
	 * @details
	 * 취소 버튼, 패널 재초기화, 다른 화면으로 돌아가는 흐름에서 확인 UI만 닫을 때 사용한다.
	 * 확인 패널을 닫아도 런 상태나 설정 값은 변경하지 않는다.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void HideAbandonConfirm() const;

	/**
	 * @brief Back 버튼이 눌렸을 때 외부 화면 흐름에 복귀를 요청하는 이벤트
	 *
	 * @details
	 * 타이틀에서는 타이틀 메뉴로, 인게임에서는 상단 메뉴나 이전 팝업 상태로 돌아갈 수 있다.
	 * 이 위젯은 자신이 어느 화면 스택에 올라와 있는지 모르므로 복귀 처리를 직접 하지 않는다.
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
	 *
	 * @details
	 * 확인 패널에서 최종 승인까지 끝난 뒤에만 발생한다.
	 * 현재 런 정리, 저장 데이터 폐기, 타이틀 이동은 이벤트를 받은 인게임 흐름이 처리한다.
	 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelEvent OnAbandonRunConfirmed;

	/**
	 * @brief 설정 초기화 버튼이 눌렸을 때 외부 흐름으로 알리는 이벤트
	 *
	 * @details
	 * 어떤 설정을 기본값으로 되돌릴지, 되돌린 값을 즉시 저장할지는 설정 시스템의 정책이다.
	 * 패널은 Reset 버튼 입력을 한 번의 요청으로 전달한다.
	 */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelEvent OnResetRequested;

	/** @brief BGM 볼륨 슬라이더 값 변경 이벤트. 전달 값은 슬라이더의 현재 값이다. */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelFloatEvent OnBgmVolumeChanged;

	/** @brief 효과음 볼륨 슬라이더 값 변경 이벤트. 전달 값은 슬라이더의 현재 값이다. */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelFloatEvent OnSfxVolumeChanged;

	/** @brief UI 볼륨 슬라이더 값 변경 이벤트. 전달 값은 슬라이더의 현재 값이다. */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelFloatEvent OnUiVolumeChanged;

	/** @brief 화면 흔들림 체크 상태 변경 이벤트. true면 흔들림 허용, false면 비활성 요청이다. */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelBoolEvent OnScreenShakeChanged;

	/** @brief 진동 체크 상태 변경 이벤트. true면 진동 허용, false면 비활성 요청이다. */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelBoolEvent OnVibrationChanged;

	/** @brief 그래픽 품질 선택 요청 이벤트. 현재 버튼 매핑은 0=낮음, 1=중간, 2=높음이다. */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelIntEvent OnQualityRequested;

	/** @brief 전체(마스터) 볼륨 슬라이더 값 변경 이벤트. */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelFloatEvent OnMasterVolumeChanged;

	/** @brief FPS 제한 선택 요청 이벤트. 전달 값은 30 또는 60. 수신 시스템이 아직 없다(값 전달만). */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelIntEvent OnFpsLimitRequested;

	/** @brief 전투 이펙트 표시 체크 상태 변경 이벤트. 수신 시스템이 아직 없다(값 전달만). */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelBoolEvent OnEffectsChanged;

	/** @brief 언어 선택 요청 이벤트. 전달 값은 ELanguageType 정수(0=한국어, 1=English). */
	UPROPERTY(Category = "UI|Settings", BlueprintAssignable)
	FSettingsPanelIntEvent OnLanguageRequested;

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
	 *
	 * @details
	 * UUserWidget은 화면에서 제거됐다가 다시 생성되거나, 같은 인스턴스가 생명주기상 Destruct를 거칠 수 있다.
	 * 이때 버튼/슬라이더 Delegate 연결을 정리해 두면 다음 Construct에서 입력이 중복 호출되는 상황을 피할 수 있다.
	 */
	void NativeDestruct() override;

	/**
	 * @brief 매 틱 뷰포트 화면비를 보고 폴드(<1.68) 모달 스케일 변형을 전환한다.
	 *
	 * @details
	 * 중앙(0.5,0.5) 점앵커 위젯들(=시안 모달 그리드)에 렌더 트랜스폼 균일 스케일을 얹어
	 * 좁은 화면(폴드 내부)에서 2단 그리드가 화면 폭 안에 들어오게 한다. 스케일 비율은
	 * WBP의 폴드 마커(Set_grid_fold/Set_grid_base 폭 비율)에서 읽는다 — C++은 계산하지 않고
	 * 디자이너 데이터를 적용만 한다(전투 HUD 폴드 컷과 동일 규약, 컷 1.68 동기).
	 */
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** @brief 폴드 모달 스케일 변형이 현재 적용 중인지. */
	bool mFoldScaleActive = false;

	/** @brief 슬라이더 값 채움 바(빌더 생성 ProgressBar) 포인터 캐시. 없으면(구 WBP) 동기 생략. */
	TWeakObjectPtr<class UProgressBar> mMasterFillBar;
	TWeakObjectPtr<class UProgressBar> mBgmFillBar;
	TWeakObjectPtr<class UProgressBar> mSfxFillBar;
	TWeakObjectPtr<class UProgressBar> mUiFillBar;
	bool mFillBarsResolved = false;

	/** @brief 채움 바를 각 슬라이더 값과 동기한다(매 틱, 위젯 없으면 no-op). */
	void SyncSliderFillBars();

	/** @brief [진단] 설정 레이아웃 전수 로그: 레터박스 체인 + 위젯별 슬롯 vs 실제 geometry vs 텍스트 실측. */
	void LogSettingsMetrics(const FVector2D& ViewportSize);

	/** @brief [진단] 마지막 로그 시점 뷰포트. 변할 때만 재로그. */
	FVector2D mMetricsLastViewport = FVector2D(-1.0f, -1.0f);

	/** @brief [진단] geometry 안정화 대기 카운터(뷰포트 변경 후 N프레임 뒤 1회 로그). */
	int32 mMetricsCountdown = 0;

	/** @brief 폴드/기준 전환 실행부. NativeTick이 화면비 컷(1.68) 교차 시에만 호출한다. */
	void ApplyFoldScaleVariant(const FVector2D& ViewportSize);

	/**
	 * @brief 설정 패널의 기본 표시 문구를 WBP TextBlock에 반영한다.
	 *
	 * @details
	 * WBP에 텍스트가 비어 있거나 임시 문구가 들어 있어도 C++ 생성 시점에 기본 문구를 채워 화면을 안정적으로 보여준다.
	 * 실제 언어 설정/로컬라이징 정책이 붙어도 이 함수는 기본 키를 제공하는 역할로 남길 수 있다.
	 */
	void SyncText() const;

	/**
	 * @brief 품질(360p/720p/1080p)·FPS(30/60) 버튼 라벨 색으로 현재 선택 상태를 표시한다.
	 *
	 * @details
	 * WBP에 선택 상태 전용 위젯이 없어 라벨 색(선택=골드, 비선택=버튼 크림)으로 표시한다.
	 * 값 모델이 바뀌는 모든 경로(버튼 클릭, 패널 열 때 저장값 복원)에서 호출해 표시를 값과 일치시킨다.
	 */
	void UpdateGraphicsSelectionIndicators() const;

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
	 * @brief WBP_SettingsPanel에 필수 바인딩이 연결되어 있는지 로그로 확인한다.
	 *
	 * @details
	 * 대부분의 바인딩은 Optional로 둔다. UI를 단계적으로 이관하는 동안 일부 위젯이 아직 WBP에 없더라도
	 * C++ 생성 자체는 깨지지 않아야 하기 때문이다. 대신 반드시 있어야 하는 입력만 경고 로그로 잡는다.
	 */
	void ValidateDesignerBindings() const;

	/**
	 * @brief 버튼 포인터가 유효할 때만 활성 상태를 변경한다.
	 *
	 * @param Button 활성 상태를 변경할 버튼
	 * @param bEnabled 새 활성 상태
	 */
	void SetButtonEnabled(UButton* Button, bool bEnabled) const;

	/** @brief BackButton 클릭을 OnBackRequested 이벤트로 변환한다. */
	UFUNCTION()
	void HandleBackButtonClicked();

	/** @brief SaveAndExitButton 클릭을 OnSaveAndExitRequested 이벤트로 변환한다. */
	UFUNCTION()
	void HandleSaveAndExitButtonClicked();

	/** @brief AbandonRunButton 클릭 시 즉시 포기하지 않고 확인 패널만 연다. */
	UFUNCTION()
	void HandleAbandonRunButtonClicked();

	/** @brief ConfirmAbandonButton 클릭을 최종 런 포기 확정 이벤트로 변환한다. */
	UFUNCTION()
	void HandleConfirmAbandonButtonClicked();

	/** @brief CancelAbandonButton 클릭 시 확인 패널만 닫는다. */
	UFUNCTION()
	void HandleCancelAbandonButtonClicked();

	/** @brief ResetButton 클릭을 설정 초기화 요청 이벤트로 변환한다. */
	UFUNCTION()
	void HandleResetButtonClicked();

	/** @brief 낮음 품질 버튼 클릭을 품질 값 0으로 전달한다. */
	UFUNCTION()
	void HandleLowQualityButtonClicked();

	/** @brief 중간 품질 버튼 클릭을 품질 값 1로 전달한다. */
	UFUNCTION()
	void HandleMediumQualityButtonClicked();

	/** @brief 높음 품질 버튼 클릭을 품질 값 2로 전달한다. */
	UFUNCTION()
	void HandleHighQualityButtonClicked();

	/** @brief BGM 슬라이더 값을 외부 볼륨 정책으로 전달한다. */
	UFUNCTION()
	void HandleBgmVolumeChanged(float Value);

	/** @brief 효과음 슬라이더 값을 외부 볼륨 정책으로 전달한다. */
	UFUNCTION()
	void HandleSfxVolumeChanged(float Value);

	/** @brief UI 슬라이더 값을 외부 볼륨 정책으로 전달한다. */
	UFUNCTION()
	void HandleUiVolumeChanged(float Value);

	/** @brief 화면 흔들림 체크박스 상태를 외부 설정 정책으로 전달한다. */
	UFUNCTION()
	void HandleScreenShakeChanged(bool bChecked);

	/** @brief 진동 체크박스 상태를 외부 설정 정책으로 전달한다. */
	UFUNCTION()
	void HandleVibrationChanged(bool bChecked);

	/** @brief 전체(마스터) 슬라이더 값을 이벤트로 올리고 프로필 볼륨에 기본 적용한다. */
	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	/** @brief FPS 30 선택을 이벤트로 올린다(수신 시스템 미구현). */
	UFUNCTION()
	void HandleFpsThirtyButtonClicked();

	/** @brief FPS 60 선택을 이벤트로 올린다(수신 시스템 미구현). */
	UFUNCTION()
	void HandleFpsSixtyButtonClicked();

	/** @brief 한국어 선택을 이벤트로 올리고 로컬라이제이션에 기본 적용한다. */
	UFUNCTION()
	void HandleLanguageKoreanButtonClicked();

	/** @brief English 선택을 이벤트로 올리고 로컬라이제이션에 기본 적용한다. */
	UFUNCTION()
	void HandleLanguageEnglishButtonClicked();

	/** @brief 이펙트 표시 체크 상태를 이벤트로 올린다(수신 시스템 미구현). */
	UFUNCTION()
	void HandleEffectsChanged(bool bChecked);

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

	/** @brief 오디오 설정 섹션 제목 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AudioSectionHeader;

	/** @brief 마스터 볼륨 행 라벨. 현재 WBP 표시용이며 실제 조작 이벤트는 아직 별도로 연결하지 않는다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MasterVolumeRow_Label;

	/** @brief BGM 볼륨 행 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BGMVolumeRow_Label;

	/** @brief 효과음 볼륨 행 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SFXVolumeRow_Label;

	/** @brief UI 볼륨 행 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UIVolumeRow_Label;

	/** @brief 화면/그래픽 설정 섹션 제목 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DisplaySectionHeader;

	/** @brief 밝기 설정 행 라벨. 현재는 표시만 하고 값 변경 이벤트는 별도 구현 대상이다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BrightnessRow_Label;

	/** @brief 화면 흔들림 설정 행 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScreenShakeRow_Label;

	/** @brief 진동 설정 행 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VibrationRow_Label;

	/** @brief 그래픽 품질 설정 행 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QualityRow_Label;

	/** @brief 게임 플레이 설정 섹션 제목 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GameplaySectionHeader;

	/** @brief 빠른 진행 모드 행 라벨. 현재는 WBP 레이아웃 보존용 표시 항목이다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FastModeRow_Label;

	/** @brief 연출 스킵 행 라벨. 현재는 WBP 레이아웃 보존용 표시 항목이다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkipAnimationRow_Label;

	/** @brief 자동 턴 종료 행 라벨. 현재는 WBP 레이아웃 보존용 표시 항목이다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AutoEndTurnRow_Label;

	/** @brief 정보 섹션 제목 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InfoSectionHeader;

	/** @brief 크레딧 행 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CreditsRow_Label;

	/** @brief 라이선스 행 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LicenseRow_Label;

	/** @brief 크레딧 열기 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CreditsOpenButtonText;

	/** @brief 라이선스 열기 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LicenseOpenButtonText;

	/** @brief 낮음 품질 선택 버튼. 클릭 시 OnQualityRequested(0)을 발생시킨다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LowQualityButton;

	/** @brief 낮음 품질 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LowQualityButtonText;

	/** @brief 중간 품질 선택 버튼. 클릭 시 OnQualityRequested(1)을 발생시킨다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MediumQualityButton;

	/** @brief 중간 품질 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MediumQualityButtonText;

	/** @brief 높음 품질 선택 버튼. 클릭 시 OnQualityRequested(2)을 발생시킨다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HighQualityButton;

	/** @brief 높음 품질 버튼 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HighQualityButtonText;

	/** @brief BGM 볼륨 입력 슬라이더 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> BgmVolumeSlider;

	/** @brief 효과음 볼륨 입력 슬라이더 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> SfxVolumeSlider;

	/** @brief UI 볼륨 입력 슬라이더 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> UiVolumeSlider;

	/** @brief 화면 흔들림 사용 여부 입력 체크박스 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> ScreenShakeCheckBox;

	/** @brief 진동 사용 여부 입력 체크박스 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> VibrationCheckBox;

	/** @brief 전체(마스터) 볼륨 입력 슬라이더 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> MasterVolumeSlider;

	/** @brief FPS 30 선택 버튼(세그먼트 좌측) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> FpsThirtyButton;

	/** @brief FPS 60 선택 버튼(세그먼트 우측) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> FpsSixtyButton;

	/** @brief 한국어 선택 버튼(언어 세그먼트 좌측) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LanguageKoreanButton;

	/** @brief English 선택 버튼(언어 세그먼트 우측) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LanguageEnglishButton;

	/** @brief 전투 이펙트 표시 여부 입력 체크박스 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> EffectsCheckBox;

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

	/**
	 * @brief 패널 입력이 마지막으로 반영한 설정 값 모델
	 */
	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FSettingsPanelValueModel mValueModel;

	/** @brief 외부 값 적용 중 위젯 콜백이 다시 외부 이벤트로 나가지 않게 막는 플래그 */
	bool mIsApplyingValueModel = false;
};
