// @file TitleMenuWidget.h
// @brief 타이틀 화면 위젯 정의 헤더
// @date 2026-06-02

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/TitleMenuRuntimeAssets.h"

#include "TitleMenuWidget.generated.h"

class UButton;
class UCanvasPanel;
class UCharacterSelectWidget;
class UImage;
class USettingsPanelWidget;
class UTextBlock;
class UTexture2D;
class UWidget;
class UWidgetSwitcher;

/** @brief 타이틀 화면의 흐름과 런타임 제어를 담당하는 위젯 */
// WBP_TitleMenu는 로고, 버튼 외형, SafeZone, 앵커 같은 정적 비주얼을 소유한다.
// C++은 WBP가 제공한 BindWidget에 의미와 흐름만 붙인다.
// 1) 화면 흐름: START→캐릭터 선택, SETTING→공용 설정 패널 OpenUI(), 하위 화면 BACK→메인 복귀.
// 2) 런타임 제어: WBP의 TitleBackgroundImage에 MediaTexture를 연결해 배경 영상만 재생.
// 단, 게임플레이 경계는 지킨다.
// 캐릭터 목록 구성/카드 갱신은 UCharacterSelectWidget,
// 런 데이터 생성·세이브 로드·방 전환은 GameMode/Subsystem API로 위임한다.
// 타이틀 위젯 안에 그 로직을 직접 넣으면 캐릭터 수·룸 규칙이 바뀔 때마다 같이 고쳐야 한다.
// BP_FrontendGameMode의 HUDClass는 WBP_TitleMenu를 가리켜야 한다.
// 필수 BindWidget 이름이 WBP에서 바뀌면 화면은 떠도 버튼 연결이 끊기므로
// ValidateDesignerBindings() 로그를 먼저 확인한다.
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UTitleMenuWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 기본 문구를 준비함 */
	// 생성자에서는 아직 WidgetTree가 완성되지 않았으므로 버튼을 만들거나 연결하지 않는다.
	// 여기서는 텍스트 기본값만 정한다.
	// @param ObjectInitializer Unreal 객체 생성에 사용하는 기본 초기화 값
	UTitleMenuWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** @brief GameMode 기준 캐릭터 후보 목록을 캐릭터 선택 화면에 다시 반영 */
	void RefreshCharacterOptionsFromGameMode();

	/** @brief GameMode의 타이틀 START API가 요청한 캐릭터 선택 화면 전환을 수행 */
	void OpenCharacterSelectFromTitle();

protected:
	/** @brief 위젯이 화면에 올라올 때 버튼 이벤트와 하위 화면 이벤트를 연결함 */
	// START, CONTINUE, SETTING 버튼을 이 위젯의 핸들러에 연결한다.
	// 캐릭터 선택 위젯이 보내는 BACK 요청도 여기서 받는다.
	// AddUniqueDynamic을 사용하므로 같은 위젯이 다시 Construct 되어도 같은 델리게이트가 중복으로 붙지 않는다.
	void NativeConstruct() override;

	/** @brief 위젯이 화면에서 내려갈 때 연결한 이벤트를 해제함 */
	// UUserWidget은 화면에서 사라졌다가 다시 Construct 될 수 있다.
	// 이때 이전 델리게이트가 남아 있으면 클릭 한 번에 핸들러가 여러 번 호출될 수 있으므로
	// NativeConstruct()에서 연결한 이벤트를 여기서 정리한다.
	void NativeDestruct() override;

private:
	/** @brief 원하는 화면을 ScreenSwitcher의 현재 화면으로 바꿈 */
	// Screen이 nullptr이면 메인 화면 인덱스로 되돌린다.
	// 연결이 깨진 WBP에서도 앱이 바로 죽지 않게 하기 위한 방어 코드다.
	// @param Screen 보여줄 화면 위젯
	void ShowScreen(UWidget* Screen) const;

	/** @brief 타이틀 메인 화면을 보여줌 */
	// 게임 시작 전 기본 메뉴 화면으로 돌아갈 때 사용한다.
	// 캐릭터 선택 화면이나 설정 화면이 직접 ScreenSwitcher를 만지지 않게 하기 위한 작은 래퍼다.
	void ShowMainScreen() const;

	/** @brief 캐릭터 선택 화면을 준비한 뒤 보여줌 */
	// WBP_TitleMenu 안에 직접 배치된 CharacterSelectWidget을 사용한다.
	// OpenCharacterSelect()를 호출해 캐릭터 목록을 새로 받아오고 선택 화면 상태를 초기화한다.
	void ShowCharacterScreen();

	/** @brief 공용 설정 패널 월드 위젯을 타이틀 모드로 열어 보여줌 */
	// 설정 화면은 타이틀 HUD 안에 직접 끼워 넣지 않고 InGameSettings 월드 위젯을 OpenUI()로 연다.
	// 타이틀과 인게임이 같은 WBP_SettingsPanel 생명주기를 공유해야 Back/Close, 입력, 팝업 ZOrder 규칙이 갈라지지 않는다.
	// 타이틀에는 현재 런 액션이 없으므로 Title 모드로 맞춰 저장 후 종료/포기하기 영역만 숨긴다.
	void OpenSettingsPanel();

	/** @brief 타이틀 메인 화면의 텍스트를 현재 설정값으로 채움 */
	// 생성자에서 정한 기본 문구나 WBP/에디터에서 바꾼 문구를 실제 TextBlock에 반영한다.
	// 텍스트 적용을 한 곳에 모아 두면 나중에 로컬라이징이나 옵션 설정을 붙일 때 바꾸는 위치가 줄어든다.
	void SyncMainText() const;

	/** @brief 저장된 런 여부에 맞춰 타이틀 메인 메뉴 버튼 구성을 바꿈 */
	// 저장된 런이 없으면 START / SETTING만 보이고,
	// 저장된 런이 있으면 CONTINUE / NEW START / SETTING 구성을 보여준다.
	// 버튼 배치는 WBP_TitleMenu가 맡고, C++은 이미 복구된 PersistentData에
	// 이어갈 Run이 있는지만 보고 표시 상태를 바꾼다.
	void RefreshMainMenuState() const;

	/** @brief 현재 복구된 Run을 이어갈 수 있는지 확인함 */
	// 세이브 파일 로드는 Intro 단계에서 끝나 있어야 한다.
	// 타이틀 UI는 파일을 직접 읽지 않고, FrontendGameMode가 현재 PersistentData 요약을 만들 수 있는지만 확인한다.
	// @return 현재 방으로 이어갈 수 있는 활성 Run 데이터가 있으면 true
	bool CanContinueRun() const;

	/** @brief 타이틀에서 열 공용 설정 패널을 얻음 */
	// 타이틀 설정도 인게임과 같은 WBP_SettingsPanel을 써야 한다.
	// 이 함수는 WBP_TitleMenu 안의 하위 위젯을 찾지 않고, FrontendGameMode가 준비한 InGameSettings 월드 위젯만 돌려준다.
	// 그래야 모든 설정 패널이 OpenUI()/CloseUI() 흐름을 타고 AddToViewport, ZOrder, 닫기 이벤트 규칙을 공유한다.
	USettingsPanelWidget* GetTitleSettingsPanel() const;

	/** @brief 타이틀 WBP에 남아 있는 하단 상태 문구를 숨김 */
	// 현재 타이틀 화면에서는 Ready, Settings 같은 하단 상태 문구를 사용하지 않으므로,
	// WBP에 StatusText가 아직 남아 있어도 비워 두고 Collapsed 상태로 만든다.
	// @param InText 기존 호출부와의 호환을 위해 받지만 화면에는 표시하지 않는다.
	void SetStatusText(const FText& InText) const;

	/** @brief 타이틀 배경 영상 재생을 시작한다. */
	// 정적 비주얼(로고/버튼/레이아웃)은 WBP_TitleMenu가 책임진다.
	// C++은 표시용 Image(WBP의 TitleBackgroundImage)에 MediaTexture를 물려 영상만 재생한다.
	// TitleBackgroundImage가 없으면 배경 영상은 생략한다.
	void StartTitleBackgroundVideo();

	/** @brief 타이틀 배경 영상용 MediaPlayer/MediaTexture/FileMediaSource를 준비한다. */
	void EnsureTitleBackgroundMediaObjects();

	/** @brief MediaTexture를 WBP 배경 Image(TitleBackgroundImage) Brush에 반영한다. */
	void ApplyTitleBackgroundVideoBrush();

	/** @brief 타이틀 배경 영상을 반복 재생한다. */
	void PlayTitleBackgroundVideo();

	/** @brief 타이틀 배경 영상 재생을 정리한다. */
	void StopTitleBackgroundVideo();

	/** @brief Content 기준 상대 경로를 실제 파일 경로로 바꾼다. */
	// [합의필요] SVN raw media를 Content 밑 파일 경로로 직접 읽는 계약은 패키징 규칙과 함께 유지되어야 한다.
	FString ResolveTitleBackgroundVideoPath() const;

	/** @brief MediaPlayer가 파일을 열었을 때 반복 재생 상태를 확정한다. */
	UFUNCTION()
	void HandleTitleBackgroundMediaOpened(FString OpenedUrl);

	/** @brief 패키징/경로 누락을 타이틀 화면 실패 대신 로그로만 남긴다. */
	UFUNCTION()
	void HandleTitleBackgroundMediaOpenFailed(FString FailedUrl);

	/** @brief 화면에 필요한 위젯 연결 상태를 로그로 확인함 */
	// BindWidget 이름이 WBP에서 바뀌면 C++ 멤버가 nullptr이 된다.
	// 이 함수는 어떤 필드가 비었는지 로그로 바로 볼 수 있게 한다.
	// 정적 비주얼이 빠진 경우에도 C++ fallback을 새로 만들지 않는다.
	// WBP가 화면 구조의 원본이어야 리뷰와 해상도 프리뷰가 한곳으로 모인다.
	void ValidateDesignerBindings() const;

	UFUNCTION()
	void HandleStartButtonClicked();

	/** @brief CONTINUE 버튼 클릭을 활성 Run의 현재 방 입장으로 연결한다. */
	// 실제 세이브 파일 로드는 여기서 하지 않는다.
	// Intro에서 복구된 RunPersistData가 있을 때 FrontendGameMode에 현재 방 전환을 요청한다.
	UFUNCTION()
	void HandleContinueButtonClicked();

	/** @brief SETTING 버튼 클릭을 공용 SettingsPanelWidget 표시로 연결한다. */
	UFUNCTION()
	void HandleSettingsButtonClicked();

	/** @brief 설정 패널의 Back 요청을 타이틀 메인 화면 복귀로 처리한다. */
	UFUNCTION()
	void HandleSettingsPanelBackRequested();

	/** @brief 캐릭터 선택 화면의 Back 요청을 타이틀 메인 화면 복귀로 처리한다. */
	UFUNCTION()
	void HandleCharacterBackToMainRequested();

private:
	/** @brief 타이틀 메인/캐릭터 선택 화면을 전환하는 위젯 */
	// TitleMenuWidget은 직접 위젯을 숨기고 보이기보다 이 ScreenSwitcher의 ActiveWidget을 바꾼다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

	/** @brief START, CONTINUE, SETTING 버튼이 있는 타이틀 메인 화면 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> StartScreen;

	/** @brief CharacterSelectWidget을 담는 캐릭터 선택 화면 자리 */
	// WBP_TitleMenu의 ScreenSwitcher 안에서 캐릭터 선택 화면으로 쓰는 슬롯이다.
	// 이 화면 안에는 WBP_CharacterSelect가 CharacterSelectWidget이라는 이름으로 직접 배치되어 있어야 한다.
	// C++은 이 화면의 자식을 새로 만들거나 갈아 끼우지 않는다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> CharacterScreen;

	/** @brief WBP에 직접 배치한 캐릭터 선택 위젯 */
	// WBP_TitleMenu의 CharacterScreen 안에 WBP_CharacterSelect를 직접 배치하면 이 멤버에 바인딩된다.
	// 이제 C++ fallback 생성은 하지 않는다.
	// 이 값이 nullptr이면 WBP_TitleMenu 자산이 잘못된 것이므로 ValidateDesignerBindings()에서 로그를 남긴다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCharacterSelectWidget> CharacterSelectWidget;

	/** @brief 캐릭터 선택 화면으로 넘어가는 START 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;

	/** @brief 저장된 런이 있을 때 현재 저장된 방으로 이어가는 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ContinueButton;

	/** @brief 설정 화면으로 넘어가는 SETTING 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;

	/** @brief (레거시) 게임 타이틀명 TextBlock. 이제 WBP의 TitleLogoImage가 타이틀을 대체하므로 Optional. */
	UPROPERTY(meta = (BindWidgetOptional))
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

	/** @brief 현재 타이틀 화면에서는 숨기는 하단 상태 문구 */
	// 지금 UI에서는 READY 같은 하단 문구를 표시하지 않는다.
	// 기존 WBP에 같은 이름의 TextBlock이 남아 있더라도 NativeConstruct()에서 숨긴다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

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

	/** @brief 아직 실제 기능이 연결되지 않은 메뉴를 눌렀을 때 보여줄 문구 */
	UPROPERTY(Category = "Title Menu|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mMainOnlyStatusText;

	/** @brief 캐릭터 선택 위젯을 만들거나 붙일 수 없을 때 보여줄 문구 */
	UPROPERTY(Category = "Title Menu|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mCharacterSelectUnavailableText;

	/** @brief WBP가 제공하는 배경 영상 표시용 Image (BindWidgetOptional) */
	// 위치/레이아웃은 WBP가 잡고, C++은 여기에 MediaTexture를 물려 영상만 재생한다.
	// WBP에 없으면 배경 영상은 생략한다. (로고/버튼 등 나머지 정적 비주얼은 전부 WBP 책임)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TitleBackgroundImage;

	/** @brief 타이틀 메인 화면에서 반복 재생할 MP4 파일의 Content 기준 상대 경로 */
	UPROPERTY(Category = "Title Menu|Background", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true, DisplayName = "Title Background Video Path"))
	FString mTitleBackgroundVideoPath = TEXT("SVN/OutSideAsset/AICreation/MS_TitleLoop_01.mp4");

	/** @brief 배경 영상 재생용 런타임 객체(MediaPlayer/Texture/Source/브러시). 정적 비주얼은 WBP. */
	UPROPERTY(Transient)
	FTitleMenuBackgroundRuntimeAssets mBackgroundRuntime;

};
