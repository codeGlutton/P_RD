/*****************************************************************//**
 * @file   CharacterSelectWidget.h
 * @brief  캐릭터 선택 화면 위젯 정의 헤더
 * @date   2026-06-04
 *********************************************************************/

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

/**
 * @brief 타이틀 흐름 안의 캐릭터 선택 화면
 *
 * @details
 * 이 위젯은 "캐릭터 선택"만 담당한다.
 * 타이틀 메인 버튼, 월드맵 화면 전환, 설정 오버레이는 UTitleMenuWidget이 담당하고,
 * 이 클래스는 GameMode에서 캐릭터 후보를 받아 WBP에 배치된 카드에 값과 이벤트만 연결한다.
 *
 * 실제 배치는 WBP_CharacterSelect에 있다.
 * C++은 mCharacterCardContainer, mConfirmButton, mSelectedCharacterNameText 같은 이름의 위젯을 받아서
 * 값과 이벤트만 연결한다.
 *
 * @note 캐릭터 수가 3명에서 5명으로 늘어나면 WBP_CharacterSelect의 카드 배치만 늘린다.
 * C++은 배치된 WBP_CharacterCard 순서대로 View 값을 넣고, 카드가 보내준 mIndex로 선택 대상을 찾는다.
 */
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
	void NativeConstruct() override;
	void NativeDestruct() override;

	UFUNCTION(Category = "Character Select", BlueprintImplementableEvent)
	void BP_OnSelectedCharacterChanged(const FFrontendCharacterOption& CharacterOption);

	UFUNCTION(Category = "Character Select", BlueprintImplementableEvent)
	void BP_OnSelectedCharacterCleared();

private:
	void RefreshLocalizedTextCache();
	void BindEvents();
	void UnbindEvents();

	/** @brief 확인/뒤로 버튼에 타이틀과 동일한 UI_Button 텍스처 브러시를 입힌다(밋밋한 기본 버튼 대신). */
	void ApplyButtonStyles() const;
	void RefreshCharacterOptions();
	void RebuildCharacterCards();
	void CollectDesignerCharacterCards(UPanelWidget* RootPanel, TArray<UCharacterCardWidget*>& OutCards) const;
	void SyncCharacterCards() const;
	void SelectCharacter(int32 CharacterIndex);
	void SyncSelectedCharacter();
	void ClearSelectedCharacter();
	void SetStatusText(const FText& InText);
	void SetConfirmButtonText(const FText& InText) const;
	void SyncSelectedCharacterArt(EPlayerJobType JobType);

	/** @brief 직업별 SVN 일러스트 PNG를 런타임 로드한다. 한 번 로드한 텍스처는 캐시해 재사용한다. */
	UTexture2D* GetOrLoadJobIllustration(EPlayerJobType JobType);
	bool BeginFirstRoomEntryWithSelectedCharacter();
	AFrontendGameMode* GetFrontendGameMode() const;
	const FFrontendCharacterOption* GetCharacterOption(int32 CharacterIndex) const;
	const FFrontendCharacterOption* GetSelectedCharacterOption() const;
	FText BuildCharacterStatText(const FFrontendCharacterOption& Option) const;
	void ValidateDesignerBindings() const;

	UFUNCTION()
	void HandleCharacterCardClicked(int32 CharacterIndex);

	UFUNCTION()
	void HandleConfirmButtonClicked();

	UFUNCTION()
	void HandleBackToMainButtonClicked();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> mCharacterCardContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> mConfirmButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mConfirmButtonText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> mBackToMainButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mBackToMainButtonText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mSelectedCharacterNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mSelectedCharacterRoleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mSelectedCharacterStatText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> mSelectedCharacterDescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> mSelectedCharacterPortraitImage;

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

	TArray<FFrontendCharacterOption> mCharacterOptions;
	FPrimaryAssetId mSelectedPlayerUnitId;
	int32 mSelectedCharacterIndex = INDEX_NONE;
	bool mStartRequested = false;

	FText mConfirmText;
	FText mBackText;
	FText mReadyStatusText;
	FText mLoadingStatusText;
	FText mFailedStatusText;
	FText mNoCharacterStatusText;
	FText mCharacterSelectText;
};
