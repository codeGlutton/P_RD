/*****************************************************************//**
 * @file   TitleMenuWidget.h
 * @brief  타이틀 화면 위젯 정의 헤더
 * @author Codex
 * @date   2026-06-02
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"

#include "TitleMenuWidget.generated.h"

class UButton;
class UCharacterSelectWidget;
class UFrontendMapWidget;
class UTextBlock;
class UWidget;
class UWidgetSwitcher;

/**
 * @brief 타이틀 UI의 가장 바깥 화면 전환을 담당하는 위젯
 *
 * @details
 * 이 위젯은 게임을 시작하기 전의 큰 메뉴 흐름만 관리한다.
 * START를 누르면 캐릭터 선택 화면으로 넘기고,
 * SETTING을 누르면 설정 화면으로 넘기고,
 * 각 하위 화면에서 BACK 요청이 오면 메인 화면으로 돌아온다.
 *
 * 실제 화면 배치는 WBP_TitleMenu에서 만든다.
 * C++은 ScreenSwitcher, StartButton 같은 이름의 위젯을 BindWidget으로 받아서
 * 버튼 이벤트와 화면 전환만 처리한다.
 *
 * 타이틀 뒤쪽 배경은 현재 WBP에 검은색 기본 레이어로 둔다.
 * 나중에 타이틀 이미지, 움짤, MediaTexture, 머티리얼 배경을 넣을 때는
 * WBP_TitleMenu에서 그 배경 레이어만 교체하면 된다.
 *
 * 캐릭터 목록을 만들거나, 캐릭터 카드를 갱신하거나, Confirm을 막는 일은
 * UCharacterSelectWidget 쪽 책임이다.
 * 실제 런 시작과 방 전환은 이번 UI-only 브랜치에서 다루지 않는다.
 * TitleMenuWidget 안에 그 로직을 다시 넣으면 캐릭터 수가 늘어날 때마다
 * 타이틀 메뉴 코드까지 같이 고쳐야 해서 구조가 다시 꼬인다.
 *
 * @note
 * 이제 C++ fallback 화면은 만들지 않는다.
 * BP_FrontendGameMode의 HUDClass는 WBP_TitleMenu를 가리켜야 한다.
 * WBP에서 필수 BindWidget 이름이 바뀌면 화면은 뜨더라도 버튼 연결이 되지 않으므로
 * ValidateDesignerBindings() 로그를 먼저 확인하면 된다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UTitleMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 기본 문구를 준비함
	 *
	 * @details
	 * 생성자에서는 아직 WidgetTree가 완성되지 않았으므로 버튼을 만들거나 연결하지 않는다.
	 * 여기서는 텍스트 기본값만 정한다.
	 *
	 * @param ObjectInitializer Unreal 객체 생성에 사용하는 기본 초기화 값
	 */
	UTitleMenuWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** @brief GameMode의 캐릭터 DataAsset preload 완료 후 캐릭터 선택 화면을 다시 그림 */
	void RefreshCharacterOptionsFromGameMode();

	/** @brief GameMode의 타이틀 START API가 요청한 캐릭터 선택 화면 전환을 수행 */
	void OpenCharacterSelectFromTitle();

protected:
	/**
	 * @brief 위젯이 화면에 올라올 때 버튼 이벤트와 하위 화면 이벤트를 연결함
	 *
	 * @details
	 * START, CONTINUE, SETTING, SETTINGS BACK 버튼을 이 위젯의 핸들러에 연결한다.
	 * 캐릭터 선택 위젯이 보내는 BACK 요청도 여기서 받는다.
	 * AddUniqueDynamic을 사용하므로 같은 위젯이 다시 Construct 되어도 같은 델리게이트가 중복으로 붙지 않는다.
	 */
	void NativeConstruct() override;

	/**
	 * @brief 위젯이 화면에서 내려갈 때 연결한 이벤트를 해제함
	 *
	 * @details
	 * UUserWidget은 화면에서 사라졌다가 다시 Construct 될 수 있다.
	 * 이때 이전 델리게이트가 남아 있으면 클릭 한 번에 핸들러가 여러 번 호출될 수 있으므로
	 * NativeConstruct()에서 연결한 이벤트를 여기서 정리한다.
	 */
	void NativeDestruct() override;

private:
	/**
	 * @brief 원하는 화면을 ScreenSwitcher의 현재 화면으로 바꿈
	 *
	 * @details
	 * Screen이 nullptr이면 메인 화면 인덱스로 되돌린다.
	 * 연결이 깨진 WBP에서도 앱이 바로 죽지 않게 하기 위한 방어 코드다.
	 *
	 * @param Screen 보여줄 화면 위젯
	 */
	void ShowScreen(UWidget* Screen) const;

	/**
	 * @brief 타이틀 메인 화면을 보여줌
	 *
	 * @details
	 * 게임 시작 전 기본 메뉴 화면으로 돌아갈 때 사용한다.
	 * 캐릭터 선택 화면이나 설정 화면이 직접 ScreenSwitcher를 만지지 않게 하기 위한 작은 래퍼다.
	 */
	void ShowMainScreen() const;

	/**
	 * @brief 캐릭터 선택 화면을 준비한 뒤 보여줌
	 *
	 * @details
	 * WBP_TitleMenu 안에 직접 배치된 CharacterSelectWidget을 사용한다.
	 * OpenCharacterSelect()를 호출해 캐릭터 목록을 새로 받아오고 선택 화면 상태를 초기화한다.
	 */
	void ShowCharacterScreen();

	/**
	 * @brief 설정 화면을 보여줌
	 *
	 * @details
	 * SettingsScreen이 있는 경우 해당 화면으로 전환한다.
	 * WBP에서 설정 화면을 아직 만들지 않은 경우에는 메인 화면에 머무르고 상태 문구만 바꾼다.
	 */
	void ShowSettingsScreen();

	/**
	 * @brief WBP_TitleMenu에 직접 배치된 지도 화면 연결을 확인함
	 *
	 * @details
	 * 지도 화면은 WBP_TitleMenu 안의 MapScreen과 FrontendMapWidget 바인딩으로 제공되어야 한다.
	 * C++은 빠진 화면을 임시로 만들지 않고, 연결이 깨졌으면 로그를 남기고 메인 화면으로 돌아간다.
	 */
	bool EnsureMapScreen();

	/**
	 * @brief 지도 화면을 보여주고 지도 내용을 갱신함
	 */
	void ShowMapScreen();

	/**
	 * @brief 타이틀 메인 화면의 텍스트를 현재 설정값으로 채움
	 *
	 * @details
	 * 생성자에서 정한 기본 문구나 WBP/에디터에서 바꾼 문구를 실제 TextBlock에 반영한다.
	 * 텍스트 적용을 한 곳에 모아 두면 나중에 로컬라이징이나 옵션 설정을 붙일 때 바꾸는 위치가 줄어든다.
	 */
	void SyncMainText() const;

	/**
	 * @brief 저장된 런 여부에 맞춰 타이틀 메인 메뉴 버튼 구성을 바꿈
	 *
	 * @details
	 * 저장된 런이 없으면 START / SETTING만 보이고,
	 * 저장된 런이 있으면 CONTINUE / NEW START / SETTING 구성을 보여준다.
	 * 버튼 배치는 WBP_TitleMenu가 맡고, C++은 저장 상태에 따른 표시 상태만 바꾼다.
	 */
	void RefreshMainMenuState() const;

	/**
	 * @brief 오래된 타이틀 WBP에 남아 있는 하단 상태 문구를 숨김
	 *
	 * @details
	 * 예전 구조에서는 타이틀 메뉴 아래에 Ready, Settings 같은 상태 문구를 표시했다.
	 * 현재 타이틀 화면에서는 이 줄을 완전히 사용하지 않으므로,
	 * WBP에 StatusText가 아직 남아 있어도 비워 두고 Collapsed 상태로 만든다.
	 *
	 * @param InText 예전 호출부와의 호환을 위해 받지만 화면에는 표시하지 않는다.
	 */
	void SetStatusText(const FText& InText) const;

	/**
	 * @brief 화면에 필요한 위젯 연결 상태를 로그로 확인함
	 *
	 * @details
	 * BindWidget 이름이 WBP에서 바뀌면 C++ 멤버가 nullptr이 된다.
	 * 이 함수는 어떤 필드가 비었는지 로그로 바로 볼 수 있게 한다.
	 * 화면을 C++로 다시 만들지는 않는다.
	 */
	void ValidateDesignerBindings() const;

	UFUNCTION()
	void HandleStartButtonClicked();

	UFUNCTION()
	void HandleContinueButtonClicked();

	UFUNCTION()
	void HandleSettingsButtonClicked();

	UFUNCTION()
	void HandleSettingsBackButtonClicked();

	UFUNCTION()
	void HandleCharacterBackToMainRequested();

	UFUNCTION()
	void HandleCharacterRunPreviewReady();

	UFUNCTION()
	void HandleMapBackRequested();

private:
	/**
	 * @brief 타이틀 메인/캐릭터 선택/설정 화면을 전환하는 위젯
	 *
	 * @details
	 * TitleMenuWidget은 직접 위젯을 숨기고 보이기보다 이 ScreenSwitcher의 ActiveWidget을 바꾼다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

	/** @brief START, CONTINUE, SETTING 버튼이 있는 타이틀 메인 화면 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> StartScreen;

	/**
	 * @brief CharacterSelectWidget을 담는 캐릭터 선택 화면 자리
	 *
	 * @details
	 * WBP_TitleMenu의 ScreenSwitcher 안에서 캐릭터 선택 화면으로 쓰는 슬롯이다.
	 * 이 화면 안에는 WBP_CharacterSelect가 CharacterSelectWidget이라는 이름으로 직접 배치되어 있어야 한다.
	 * C++은 이 화면의 자식을 새로 만들거나 갈아 끼우지 않는다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> CharacterScreen;

	/** @brief 설정 화면 자리. WBP에서 아직 만들지 않았을 수도 있어서 Optional이다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SettingsScreen;

	/** @brief 지도 화면 자리. 이번 단계에서는 WBP_TitleMenu 안의 ScreenSwitcher 자식으로 둔다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MapScreen;

	/**
	 * @brief WBP에 직접 배치한 캐릭터 선택 위젯
	 *
	 * @details
	 * WBP_TitleMenu의 CharacterScreen 안에 WBP_CharacterSelect를 직접 배치하면 이 멤버에 바인딩된다.
	 * 이제 C++ fallback 생성은 하지 않는다.
	 * 이 값이 nullptr이면 WBP_TitleMenu 자산이 잘못된 것이므로 ValidateDesignerBindings()에서 로그를 남긴다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCharacterSelectWidget> CharacterSelectWidget;

	/** @brief MapScreen 안에 직접 배치한 지도 위젯 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UFrontendMapWidget> FrontendMapWidget;

	/** @brief 캐릭터 선택 화면으로 넘어가는 START 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;

	/** @brief 이어하기 버튼. 현재는 실제 이어하기 기능이 없어 비활성/안내 상태로 둔다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ContinueButton;

	/** @brief 설정 화면으로 넘어가는 SETTING 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;

	/** @brief 게임 타이틀명을 표시하는 TextBlock */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	/** @brief START 버튼 안에 표시할 라벨 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StartButtonText;

	/** @brief CONTINUE 버튼 안에 표시할 라벨 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ContinueButtonText;

	/** @brief SETTING 버튼 안에 표시할 라벨 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SettingsButtonText;

	/**
	 * @brief 예전 타이틀 화면 아래에 있던 상태 문구
	 *
	 * @details
	 * 지금 UI에서는 READY 같은 하단 문구를 표시하지 않는다.
	 * 기존 WBP에 같은 이름의 TextBlock이 남아 있더라도 NativeConstruct()에서 숨긴다.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** @brief 설정 화면에서 타이틀 메인으로 돌아가는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsBackButton;

	/** @brief 설정 화면 뒤로 가기 버튼 안에 표시할 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsBackButtonText;

	/** @brief 게임 타이틀명 기본 문구 */
	UPROPERTY(Category = "Title Menu|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mTitleText;

	/** @brief START 버튼 기본 문구 */
	UPROPERTY(Category = "Title Menu|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mStartButtonText;

	/** @brief 저장된 런이 있을 때 새로 시작 버튼에 표시할 문구 */
	UPROPERTY(Category = "Title Menu|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mNewStartButtonText;

	/** @brief CONTINUE 버튼 기본 문구 */
	UPROPERTY(Category = "Title Menu|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mContinueButtonText;

	/** @brief SETTING 버튼 기본 문구 */
	UPROPERTY(Category = "Title Menu|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mSettingsButtonText;

	/** @brief 설정 화면에 들어갔을 때 보여줄 상태/제목 문구 */
	UPROPERTY(Category = "Title Menu|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mSettingsStatusText;

	/** @brief 아직 실제 기능이 연결되지 않은 메뉴를 눌렀을 때 보여줄 문구 */
	UPROPERTY(Category = "Title Menu|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mMainOnlyStatusText;

	/** @brief 캐릭터 선택 위젯을 만들거나 붙일 수 없을 때 보여줄 문구 */
	UPROPERTY(Category = "Title Menu|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mCharacterSelectUnavailableText;

	/** @brief BACK 버튼 공통 기본 문구 */
	UPROPERTY(Category = "Title Menu|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mBackButtonText;
};
