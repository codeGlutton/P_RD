// @file CharacterSelectWidget.h
// @brief 캐릭터 선택 화면 위젯 정의 헤더
// @date 2026-06-04

#pragma once

#include "RDMinimal.h"
#include "Frontend/CharacterSelectTypes.h"
#include "UI/RDUserWidget.h"

#include "CharacterSelectWidget.generated.h"

class AFrontendGameMode;
class UButton;
class UCharacterCardWidget;
class UImage;
class UPanelWidget;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCharacterSelectSimpleEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterSelectStatusChangedDelegate, FText, InStatusText);

/** @brief 타이틀 흐름 안의 캐릭터 선택 화면 */
// 이 위젯은 "캐릭터 선택"만 담당한다.
// 타이틀 메인 버튼, 월드맵 화면 전환, 설정 오버레이는 UTitleMenuWidget이 담당하고,
// 이 클래스는 GameMode에서 캐릭터 후보를 받아 WBP에 배치된 카드에 값과 이벤트만 연결한다.
// 실제 배치는 WBP_CharacterSelect에 있다.
// C++은 mCharacterCardContainer, mConfirmButton, mSelectedCharacterNameText 같은 이름의 위젯을 받아서
// 값과 이벤트만 연결한다.
// 주의: 캐릭터 수가 3명에서 5명으로 늘어나면 WBP_CharacterSelect의 카드 배치만 늘린다.
// C++은 배치된 WBP_CharacterCard 순서대로 View 값을 넣고, 카드가 보내준 mIndex로 선택 대상을 찾는다.
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCharacterSelectWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UCharacterSelectWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** @brief START로 캐릭터 선택 화면에 들어올 때 목록/선택/상태를 새로 맞춤 */
	void OpenCharacterSelect();

	/** @brief GameMode 기준 캐릭터 후보 목록을 다시 그림 */
	void RefreshCharacterOptionsFromGameMode();

public:
	/** @brief BACK 버튼을 눌렀을 때 TitleMenuWidget에 메인 화면 복귀를 요청함 */
	UPROPERTY(Category = "Character Select", BlueprintAssignable)
	FCharacterSelectSimpleEvent OnBackToMainRequested;

	/** @brief 상태 문구가 바뀌었음을 외부 디버그 UI 등에 알려주는 호환용 이벤트 */
	UPROPERTY(Category = "Character Select", BlueprintAssignable)
	FCharacterSelectStatusChangedDelegate OnStatusTextChanged;

protected:
	/** @brief WBP 바인딩/버튼 스타일/카드 목록을 현재 GameMode 상태에 맞춰 초기화한다. */
	void NativeConstruct() override;

	/** @brief 뷰포트/DPI 레이아웃 확정 뒤에도 클래스 배경 일러스트 cover 배치를 유지한다. */
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** @brief 화면 이탈 시 버튼/카드 델리게이트를 해제해 재Construct 중복 호출을 막는다. */
	void NativeDestruct() override;

	/** @brief 선택된 캐릭터 View 값만 BP 연출로 넘긴다; BP는 런 생성 로직을 호출하지 않는다. */
	UFUNCTION(Category = "Character Select", BlueprintImplementableEvent)
	void BP_OnSelectedCharacterChanged(const FFrontendCharacterOption& CharacterOption);

	/** @brief 선택 가능한 캐릭터가 없거나 목록 갱신 중 선택이 비었음을 BP 연출에 알린다. */
	UFUNCTION(Category = "Character Select", BlueprintImplementableEvent)
	void BP_OnSelectedCharacterCleared();

private:
	/** @brief 코드에서 사용하는 고정 문구를 한 번에 다시 읽어 로컬라이징 갱신 지점을 좁힌다. */
	void RefreshLocalizedTextCache();

	/** @brief Confirm/Back 버튼 이벤트를 연결한다; 카드 이벤트는 RebuildCharacterCards()가 소유한다. */
	void BindEvents();

	/** @brief Construct에서 연결한 버튼과 현재 카드 이벤트를 모두 정리한다. */
	void UnbindEvents();

	/** @brief 확인/뒤로 버튼에 타이틀과 동일한 UI_Button 텍스처 브러시를 입힌다(기존 WBP 버튼 스타일 fallback). */
	void ApplyButtonStyles() const;

	/** @brief 클래스 일러스트를 덮는 레거시 배경 패널 색/표시를 제거한다. */
	void HideBackgroundObscuringWidgets() const;

	/** @brief FrontendGameMode가 만든 후보 View 목록을 다시 받아 선택 상태와 카드를 동기화한다. */
	void RefreshCharacterOptions();

	/** @brief WBP에 배치된 카드 인스턴스에 후보 데이터를 주입하고 남는 카드를 숨긴다. */
	void RebuildCharacterCards();

	/** @brief WBP 계층을 재귀 탐색해 카드 위젯을 디자이너 배치 순서대로 모은다. */
	void CollectDesignerCharacterCards(UPanelWidget* RootPanel, TArray<UCharacterCardWidget*>& OutCards) const;

	/** @brief 현재 선택 index와 각 카드의 CharacterOption.mIndex를 비교해 선택 연출만 갱신한다. */
	void SyncCharacterCards() const;

	/** @brief 카드 클릭에서 올라온 안정 index를 현재 선택으로 저장하고 상세 패널을 갱신한다. */
	void SelectCharacter(int32 CharacterIndex);

	/** @brief 선택된 View 값을 상세 텍스트/아트/BP 이벤트/Confirm 상태에 반영한다. */
	void SyncSelectedCharacter();

	/** @brief 선택이 없거나 후보 로드가 실패한 상태로 상세 패널과 Confirm 버튼을 되돌린다. */
	void ClearSelectedCharacter();

	/** @brief 상태 문구를 선택 화면 안과 외부 호환 이벤트에 동시에 반영한다. */
	void SetStatusText(const FText& InText);

	/** @brief Confirm/Back 버튼 라벨을 런 시작 상태와 로컬라이징 캐시에 맞춰 반영한다. */
	void SetConfirmButtonText(const FText& InText) const;

	/** @brief 선택 직업에 맞는 액션 일러스트만 보이고 구형 포트레이트 슬롯은 fallback으로 사용한다. */
	void SyncSelectedCharacterArt(EPlayerJobType JobType);

	/** @brief 현재 선택된 직업 액션 일러스트를 화면 전체 cover 방식으로 중앙 배치한다. */
	void FitSelectedCharacterArtToViewport() const;

	/** @brief 직업별 SVN Texture2D uasset을 런타임 로드한다. 한 번 로드한 텍스처는 캐시해 재사용한다. */
	// [합의필요] SVN 임포트 uasset 경로 계약은 아트 교체/패키징 규칙과 함께 갱신되어야 한다.
	UTexture2D* GetOrLoadJobIllustration(EPlayerJobType JobType);

	/** @brief 선택된 PlayerUnitId로 새 Run 생성을 요청하고 첫 방 전환 시작 여부만 돌려준다. */
	bool BeginFirstRoomEntryWithSelectedCharacter();

	/** @brief 프론트엔드 방에서만 유효한 Auth GameMode를 가져온다; 에디터 프리뷰에서는 nullptr일 수 있다. */
	AFrontendGameMode* GetFrontendGameMode() const;

	/** @brief GameMode가 부여한 안정 index로 후보 View 값을 찾는다. */
	const FFrontendCharacterOption* GetCharacterOption(int32 CharacterIndex) const;

	/** @brief 현재 선택 index가 가리키는 후보 View 값을 찾는다. */
	const FFrontendCharacterOption* GetSelectedCharacterOption() const;

	/** @brief GameMode가 준 요약이 없을 때만 UI 표시용 스탯 문자열을 조립한다. */
	FText BuildCharacterStatText(const FFrontendCharacterOption& Option) const;

	/** @brief WBP_CharacterSelect가 제공해야 하는 BindWidget 누락을 로그로 드러낸다. */
	void ValidateDesignerBindings() const;

	/** @brief 카드 한 장의 클릭 이벤트를 선택 상태 변경으로 연결한다. */
	UFUNCTION()
	void HandleCharacterCardClicked(int32 CharacterIndex);

	/** @brief Confirm 입력을 중복 방지 게이트와 GameMode StartNewRun 호출로 연결한다. */
	UFUNCTION()
	void HandleConfirmButtonClicked();

	/** @brief 런 시작 전 Back 입력만 타이틀 메인 복귀 이벤트로 올려 보낸다. */
	UFUNCTION()
	void HandleBackToMainButtonClicked();

private:
	/** @brief WBP가 카드들을 배치하는 컨테이너; C++은 자식 카드 생성 없이 여기서 수집만 한다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> mCharacterCardContainer;

	/** @brief 선택 확정 버튼; 실제 시작 가능 여부는 선택된 View의 mSelectable/PlayerUnitId가 결정한다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> mConfirmButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mConfirmButtonText;

	/** @brief 타이틀 메인으로 돌아가는 버튼; 런 시작 요청 이후에는 입력을 무시한다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> mBackToMainButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mBackToMainButtonText;

	/** @brief 선택 상세 패널의 이름 표시 영역; 데이터는 FFrontendCharacterOption에서만 온다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mSelectedCharacterNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mSelectedCharacterRoleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mSelectedCharacterStatText;

	/** @brief 스탯 아이콘+값 행 컨테이너. 아이콘/배치는 WBP가 소유하고 C++은 값 텍스트만 갱신한다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UVerticalBox> mStatRowBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mMaxHPStatValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mDiceStatValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mGoldStatValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mSelectedCharacterDescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> mSelectedCharacterPortraitImage;

	/** @brief 직업별 액션 일러스트 슬롯; 존재하면 공용 Portrait 슬롯은 fallback으로만 사용한다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> mKnightActionImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> mRogueActionImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> mMageActionImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mSelectedCharacterPortraitFallbackText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mCharacterStatusText;

	UPROPERTY()
	TArray<TObjectPtr<UCharacterCardWidget>> mCharacterCardWidgets;

	/** @brief 직업별 일러스트 텍스처(SVN 임포트 uasset) 참조. 기본값은 생성자에서 지정, WBP에서 덮어쓸 수 있다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Select|Art", meta = (AllowPrivateAccess = true))
	TMap<EPlayerJobType, TSoftObjectPtr<UTexture2D>> mJobIllustrationAssets;

	/** @brief 직업별 런타임 일러스트 텍스처 캐시. GC 방지를 위해 UPROPERTY로 들고 있는다. */
	UPROPERTY(Transient)
	TMap<EPlayerJobType, TObjectPtr<UTexture2D>> mJobIllustrationCache;

	/** @brief FrontendGameMode가 마지막으로 내려준 후보 View 목록; UI는 이 배열을 직접 계산하지 않는다. */
	TArray<FFrontendCharacterOption> mCharacterOptions;

	/** @brief Confirm 때 GameMode로 되돌려 보낼 실제 PlayerUnit 식별자; 잠긴 선택에서는 빈 값이다. */
	FPrimaryAssetId mSelectedPlayerUnitId;

	/** @brief 현재 선택된 후보의 안정 index; 배열 인덱스가 아니므로 INDEX_NONE을 명시적인 비선택 상태로 쓴다. */
	int32 mSelectedCharacterIndex = INDEX_NONE;

	/** @brief StartNewRun 요청 이후 Confirm/Back/카드 입력 재진입을 막는 단방향 게이트. */
	bool mStartRequested = false;

	FText mConfirmText;
	FText mBackText;
	FText mReadyStatusText;
	FText mLoadingStatusText;
	FText mFailedStatusText;
	FText mNoCharacterStatusText;
	FText mCharacterSelectText;
};
