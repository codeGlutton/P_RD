/*****************************************************************//**
 * @file   CharacterSelectWidget.h
 * @brief  캐릭터 선택 화면 위젯 정의 헤더
 * @author Codex
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Frontend/CharacterSelectTypes.h"
#include "UObject/SoftObjectPath.h"

#include "CharacterSelectWidget.generated.h"

class AFrontendGameMode;
class UButton;
class UCharacterCardWidget;
class UImage;
class UPanelWidget;
class UTextBlock;
class UTexture2D;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCharacterSelectSimpleEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterSelectStatusChangedDelegate, FText, InStatusText);

/**
 * @brief 타이틀 흐름 안의 캐릭터 선택 화면
 *
 * @details
 * 이 위젯은 "캐릭터 선택"만 담당한다.
 * 타이틀 메인 버튼, 월드맵 화면 전환, 설정 오버레이는 UTitleMenuWidget이 담당하고,
 * 이 클래스는 GameMode에서 캐릭터 후보를 받아 카드 WBP를 만들고 선택/Confirm 처리만 한다.
 *
 * 실제 배치는 WBP_CharacterSelect에 있다.
 * C++은 mCharacterCardContainer, mConfirmButton, mSelectedCharacterNameText 같은 이름의 위젯을 받아서
 * 값과 이벤트만 연결한다.
 *
 * @note 캐릭터 수가 3명에서 5명으로 늘어나도 numbered 함수는 추가하지 않는다.
 * mCharacterOptions 개수만큼 WBP_CharacterCard를 만들고, 카드가 보내준 mIndex로 선택 대상을 찾는다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCharacterSelectWidget : public UUserWidget
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

private:
	void RefreshLocalizedTextCache();
	void BindEvents();
	void UnbindEvents();
	void RefreshCharacterOptions();
	void RebuildCharacterCards();
	void SyncCharacterCards() const;
	void SelectCharacter(int32 CharacterIndex);
	void SyncSelectedCharacter();
	void ClearSelectedCharacter();
	void SetStatusText(const FText& InText);
	void SetConfirmButtonText(const FText& InText) const;
	void SetPortraitImage(const TSoftObjectPtr<UTexture2D>& Portrait);
	void ApplyPortraitImage(UTexture2D* Texture) const;
	void HandlePortraitLoaded(FSoftObjectPath PortraitPath);
	void CancelPortraitLoad();
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

	UPROPERTY(Category = "Character Select", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TSubclassOf<UCharacterCardWidget> CharacterCardWidgetClass;

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
	TObjectPtr<UTextBlock> mSelectedCharacterPortraitFallbackText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> mCharacterStatusText;

	UPROPERTY()
	TArray<TObjectPtr<UCharacterCardWidget>> CharacterCardWidgets;

	TArray<FFrontendCharacterOption> mCharacterOptions;
	FPrimaryAssetId mSelectedPlayerUnitId;
	int32 mSelectedCharacterIndex = INDEX_NONE;
	TSharedPtr<FStreamableHandle> mPortraitLoadHandle = nullptr;
	FSoftObjectPath mPendingPortraitPath;
	bool bStartRequested = false;

	FText mConfirmText;
	FText mBackText;
	FText mReadyStatusText;
	FText mLoadingStatusText;
	FText mFailedStatusText;
	FText mNoCharacterStatusText;
	FText mCharacterSelectText;
};
