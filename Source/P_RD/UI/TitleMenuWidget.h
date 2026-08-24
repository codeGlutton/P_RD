/**
 * @file TitleMenuWidget.h
 * @brief 타이틀 화면 위젯(타이틀 메뉴 + 배경 영상 표시) 정의 헤더
 * @author 박용수
 * @date 2026-06-26
 */
// @date 2026-06-02

#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/TitleMenuRuntimeAssets.h"

#include "TitleMenuWidget.generated.h"

// WBP에서 BindWidget으로 채워질 자식 위젯/서브 화면 타입의 전방 선언.
// 헤더 의존을 줄이려고 포인터 멤버는 여기서 전방 선언만 하고 실제 포함은 .cpp에서 한다.
class UButton;
class UCanvasPanel;
class UImage;
class USettingsPanelWidget;
class UTextBlock;
class UWidgetSwitcher;

/** @brief 타이틀 화면의 흐름과 런타임 제어를 담당하는 위젯 */
// WBP_TitleMenu는 로고, 버튼 외형, SafeZone, 앵커 같은 정적 비주얼을 소유한다.
// C++은 WBP가 제공한 BindWidget에 의미와 흐름만 붙인다.
// 1) 화면 흐름: START→GameMode 캐릭터 선택 요청, SETTING→공용 설정 패널 OpenUI().
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

	/** @brief 화면에 올라오기 전 타이틀 배경 영상을 미리 열어 둔다. */
	void PrimeTitleBackgroundVideo();

protected:
	/** @brief 타이틀 화면 — 공용 버튼 누름 효과를 켠다(프론트엔드 한정). */
	virtual bool ShouldApplyButtonFeedback() const override { return true; }

	/** @brief 위젯이 화면에 올라올 때 버튼 이벤트와 하위 화면 이벤트를 연결함 */
	// START, CONTINUE, SETTING 버튼을 이 위젯의 핸들러에 연결한다.
	// AddUniqueDynamic을 사용하므로 같은 위젯이 다시 Construct 되어도 같은 델리게이트가 중복으로 붙지 않는다.
	void NativeConstruct() override;

	/** @brief 배경 영상 cover-crop 배치를 현재 화면 크기에 맞춰 갱신한다. */
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** @brief 위젯이 화면에서 내려갈 때 연결한 이벤트를 해제함 */
	// UUserWidget은 화면에서 사라졌다가 다시 Construct 될 수 있다.
	// 이때 이전 델리게이트가 남아 있으면 클릭 한 번에 핸들러가 여러 번 호출될 수 있으므로
	// NativeConstruct()에서 연결한 이벤트를 여기서 정리한다.
	void NativeDestruct() override;

private:
	/** @brief 공용 설정 패널 월드 위젯을 타이틀 모드로 열어 보여줌 */
	// 설정 화면은 타이틀 HUD 안에 직접 끼워 넣지 않고 InGameSettings 월드 위젯을 OpenUI()로 연다.
	// 타이틀과 인게임이 같은 WBP_SettingsPanel 생명주기를 공유해야 Back/Close, 입력, 팝업 ZOrder 규칙이 갈라지지 않는다.
	// 타이틀에는 현재 런 액션이 없으므로 Title 모드로 맞춰 저장 후 종료/포기하기 영역만 숨긴다.
	void OpenSettingsPanel();

	/** @brief 타이틀 메인 화면의 텍스트를 현재 설정값으로 채움 */
	// 생성자에서 정한 기본 문구나 WBP/에디터에서 바꾼 문구를 실제 TextBlock에 반영한다.
	// 텍스트 적용을 한 곳에 모아 두면 나중에 로컬라이징이나 옵션 설정을 붙일 때 바꾸는 위치가 줄어든다.
	void SyncMainText();




	/** @brief WBP에 있는 모든 타이틀 메뉴 버튼을 같은 입력 핸들러에 연결한다. */
	void BindMainMenuButtons();

	/** @brief Construct에서 연결한 타이틀 메뉴 버튼 입력을 해제한다. */
	void UnbindMainMenuButtons();

	/** @brief WBP에서 넘어온 메뉴 TextBlock을 버튼 내부 중앙에 맞춘다. */
	// RectEditor의 textAlignH/textAlignV가 WBP 생성 때 누락될 수 있어 런타임에서 보정한다.
	void AlignMainMenuTextBlocks();

	/** @brief 현재 문자열의 실제 글리프 잉크 경계를 측정해 메뉴 글자를 광학 중앙에 맞춘다. */
	void ApplyMainMenuTextOpticalAlignment();

	/** @brief TextBlock 하나를 원래 Canvas 위치를 유지한 채 Overlay로 감싸 세로 중앙 정렬한다. */
	void AlignMenuTextBlock(UTextBlock* TextBlock);

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
	// 공유 텍스처를 못 쓸 때만 이 위젯 전용 미디어 객체를 새로 만들어 mBackgroundRuntime에 보관한다.
	void EnsureTitleBackgroundMediaObjects();

	/** @brief GameInstanceSubsystem이 미리 연 MediaTexture를 타이틀 배경으로 연결한다. */
	// UTitleBackgroundVideoSubsystem이 인트로 단계에서 영상을 미리 열어 두므로,
	// 가능하면 공유 텍스처를 재사용해 타이틀 진입 시 영상이 처음부터 끊김 없이 보이게 한다.
	// @return 공유 MediaTexture를 배경으로 연결했으면 true, 직접 준비가 필요하면 false
	bool TryUseSharedTitleBackgroundVideo();

	/** @brief MediaTexture를 WBP 배경 Image(TitleBackgroundImage) Brush에 반영한다. */
	// 머티리얼/브러시의 ResourceObject를 MediaTexture로 바꿔, Image가 매 프레임 영상 프레임을 그리게 한다.
	void ApplyTitleBackgroundVideoBrush();

	/** @brief 타이틀 배경 영상을 반복 재생한다. */
	void PlayTitleBackgroundVideo();

	/** @brief 타이틀 배경 영상 Image를 원본 비율 유지 cover-crop으로 배치한다. */
	// 좌우는 뷰포트 너비에 맞추고(letterbox 없이 가득 채움), 비율 초과분은 상하를 잘라낸다(crop).
	// 영상 원본 비율과 화면 비율이 다를 때 검은 띠 없이 화면을 가득 덮기 위한 cover 방식 배치다.
	void FitTitleBackgroundVideoToViewport();

	/** @brief 타이틀 배경 영상 재생을 정리한다. */
	void StopTitleBackgroundVideo();

	/** @brief 게임 설정의 타이틀 배경 영상 경로를 실제 파일 경로로 바꾼다. */
	FString ResolveTitleBackgroundVideoPath() const;

	/** @brief MP4 파일에서 비디오 트랙 표시 해상도를 읽는다. */
	FVector2D ReadTitleBackgroundVideoFileDimensions(const FString& VideoPath) const;

	/** @brief MediaPlayer가 파일을 열었을 때 반복 재생 상태를 확정한다. */
	// MediaPlayer.OnMediaOpened 델리게이트 콜백. 열기 시점이 비동기라 여기서 Looping/Play를 확정한다.
	// @param OpenedUrl 실제로 열린 미디어 URL
	UFUNCTION()
	void HandleTitleBackgroundMediaOpened(FString OpenedUrl);

	/** @brief 패키징/경로 누락을 타이틀 화면 실패 대신 로그로만 남긴다. */
	// 배경 영상은 보조 연출이므로 열기에 실패해도 타이틀 화면 자체는 떠야 한다. 그래서 로그만 남기고 넘어간다.
	// @param FailedUrl 열기에 실패한 미디어 URL
	UFUNCTION()
	void HandleTitleBackgroundMediaOpenFailed(FString FailedUrl);

	/** @brief 화면에 필요한 위젯 연결 상태를 로그로 확인함 */
	// BindWidget 이름이 WBP에서 바뀌면 C++ 멤버가 nullptr이 된다.
	// 이 함수는 어떤 필드가 비었는지 로그로 바로 볼 수 있게 한다.
	// 정적 비주얼이 빠진 경우에도 C++ fallback을 새로 만들지 않는다.
	// WBP가 화면 구조의 원본이어야 리뷰와 해상도 프리뷰가 한곳으로 모인다.
	void ValidateDesignerBindings() const;

	/** @brief START 버튼 클릭을 캐릭터 선택 화면 진입으로 연결한다. */
	// 저장된 런이 있을 때는 이 버튼이 NEW START(새 런 시작) 의미로 쓰인다.
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

private:
	/** @brief 캐릭터 선택 화면으로 넘어가는 START 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartButton;

	/** @brief 저장된 런이 있을 때 현재 저장된 방으로 이어가는 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton;

	/** @brief 설정 화면으로 넘어가는 SETTING 버튼 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton;

	/** @brief (레거시) 게임 타이틀명 TextBlock. 이제 WBP의 TitleLogoImage가 타이틀을 대체하므로 Optional. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** @brief START 버튼 안에 표시할 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StartButtonText;

	/** @brief CONTINUE 버튼 안에 표시할 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContinueButtonText;

	/** @brief SETTING 버튼 안에 표시할 라벨 */
	UPROPERTY(meta = (BindWidgetOptional))
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

	/** @brief WBP가 제공하는 배경 영상 표시용 Image (BindWidgetOptional) */
	// 위치/레이아웃은 WBP가 잡고, C++은 여기에 MediaTexture를 물려 영상만 재생한다.
	// WBP에 없으면 배경 영상은 생략한다. (로고/버튼 등 나머지 정적 비주얼은 전부 WBP 책임)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TitleBackgroundImage;


	/** @brief 배경 영상 재생용 런타임 객체(MediaPlayer/Texture/Source/브러시). 정적 비주얼은 WBP. */
	// Transient: 세이브/직렬화 대상이 아닌 순수 런타임 핸들 묶음이라 저장하지 않는다.
	// 공유 텍스처(TryUseSharedTitleBackgroundVideo) 사용 시 일부 필드는 비어 있을 수 있다.
	UPROPERTY(Transient)
	FTitleMenuBackgroundRuntimeAssets mBackgroundRuntime;




};
