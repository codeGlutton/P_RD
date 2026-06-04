/*****************************************************************//**
 * @file   CharacterSelectWidget.h
 * @brief  캐릭터 선택 화면 위젯 정의 헤더
 * @author Codex
 * @date   2026-06-03
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Frontend/CharacterSelectTypes.h"
#include "UObject/SoftObjectPath.h"

#include "CharacterSelectWidget.generated.h"

struct FStreamableHandle;

class UButton;
class UCharacterCardWidget;
class UImage;
class UPanelWidget;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCharacterSelectBackRequestedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterSelectStatusChangedDelegate, FText, InStatusText);

/**
 * @brief 캐릭터 선택 화면 전체를 담당하는 위젯
 *
 * @details
 * 이 위젯은 타이틀 메뉴 안에 들어가는 "캐릭터 선택 화면"이다.
 * 이번 UI-only 브랜치에서는 화면 확인용 임시 캐릭터 목록을 직접 만들고,
 * 그 개수만큼 UCharacterCardWidget을 만든다.
 *
 * 실제 화면 배치는 WBP_CharacterSelect에서 만든다.
 * C++은 CharacterCardContainer, ConfirmButton 같은 이름의 위젯을 BindWidget으로 받아서
 * 캐릭터 데이터와 클릭 이벤트만 처리한다.
 *
 * 화면 뒤쪽은 현재 WBP에 검은색 기본 배경으로 둔다.
 * 나중에 캐릭터 선택용 이미지, 움짤, MediaTexture, 머티리얼 배경을 넣을 때
 * 디자이너 WBP에서 배경 레이어만 교체하면 선택 로직은 그대로 쓸 수 있다.
 *
 * 현재 기본 화면에서는 상단 "Character Select" 제목과 하단 "Select a character" 안내 문구를 표시하지 않는다.
 * 캐릭터 후보는 화면 아래쪽의 아이콘 버튼들로만 보여준다.
 *
 * 중요한 구조는 다음과 같다.
 * - TitleMenuWidget: START/BACK 같은 큰 화면 전환만 담당
 * - CharacterSelectWidget: 캐릭터 목록, 선택 상태, Confirm 버튼 담당
 * - CharacterCardWidget: 카드 한 장의 표시와 클릭 이벤트만 담당
 *
 * 실제 캐릭터 목록 제공자, 플레이어 유닛 확정, 런 시작, 방 전환은 이번 PR 범위가 아니다.
 * 해당 API가 담당 파트에서 준비되면 RefreshCharacterOptions()의 임시 목록 생성부를 교체한다.
 *
 * @note
 * 캐릭터 수가 늘어나도 번호별 클릭 함수를 새로 추가하지 않는다.
 * mCharacterOptions 배열 개수만큼 카드 위젯을 만들고,
 * 카드 클릭 이벤트에 들어온 CharacterIndex로 선택 대상을 찾는다.
 *
 * @note
 * C++ fallback 화면은 만들지 않는다.
 * WBP_CharacterSelect에는 필수 BindWidget 이름들이 들어 있어야 하며,
 * 카드 하나의 모양은 WBP_CharacterCard가 담당한다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCharacterSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 기본 문구를 준비함
	 *
	 * @details
	 * 카드 위젯 클래스는 C++ 생성자에서 경로로 찾지 않는다.
	 * WBP_CharacterSelect의 Details 패널에서 CharacterCardWidgetClass를 WBP_CharacterCard로 지정한다.
	 * 이렇게 두면 카드 모양을 바꾸거나 다른 카드 WBP로 교체할 때 C++을 다시 빌드하지 않아도 된다.
	 *
	 * @param ObjectInitializer Unreal 객체 생성에 사용하는 기본 초기화 값
	 */
	UCharacterSelectWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * @brief 캐릭터 선택 화면을 열 때 호출함
	 *
	 * @details
	 * START 버튼을 눌러 이 화면에 들어올 때마다 호출된다.
	 * 캐릭터 목록을 다시 받아오고,
	 * 선택 상세 정보와 상태 문구를 기본값으로 맞춘다.
	 */
	void OpenCharacterSelect();

public:
	/**
	 * @brief BACK 버튼을 눌렀을 때 부모 TitleMenuWidget에 알려주는 이벤트
	 *
	 * @details
	 * CharacterSelectWidget은 ScreenSwitcher를 직접 만지지 않는다.
	 * 이 이벤트만 보내고, 실제 메인 화면 복귀는 TitleMenuWidget이 처리한다.
	 */
	UPROPERTY(Category = "Character Select", BlueprintAssignable)
	FCharacterSelectBackRequestedDelegate OnBackToMainRequested;

	/**
	 * @brief 상태 문구가 바뀌었을 때 부모 위젯에도 알려주는 이벤트
	 *
	 * @details
	 * 지금 기본 UI에서는 상태 문구를 화면에 표시하지 않는다.
	 * 그래도 기존 WBP나 디버그용 UI가 이 이벤트를 쓰고 있을 수 있으므로 델리게이트는 남겨 둔다.
	 * 즉, 화면 표시용이라기보다 예전 구조와의 호환용 이벤트다.
	 */
	UPROPERTY(Category = "Character Select", BlueprintAssignable)
	FCharacterSelectStatusChangedDelegate OnStatusTextChanged;

protected:
	/**
	 * @brief 위젯이 화면에 올라올 때 버튼 이벤트를 연결하고 첫 목록을 그림
	 */
	void NativeConstruct() override;

	/**
	 * @brief 위젯이 화면에서 내려갈 때 버튼/카드 이벤트와 비동기 로딩을 정리함
	 */
	void NativeDestruct() override;

private:
	/**
	 * @brief 캐릭터 선택 데이터 목록을 다시 가져옴
	 *
	 * @details
	 * feature/create-srpg-framework-base 통합 전이라
	 * Warrior, Archer, Magician을 코드에서 직접 만든 임시 하드코딩 목록으로 채운다.
	 * 나중에 실제 캐릭터 데이터 제공자가 준비되면 이 함수의 목록 생성부만 교체한다.
	 *
	 * 목록 개수가 바뀌었을 수 있으므로 선택 인덱스를 보정하고 카드 목록도 다시 맞춘다.
	 */
	void RefreshCharacterOptions();

	/**
	 * @brief Confirm/BACK 버튼 문구와 BACK 버튼 상태를 화면에 반영함
	 *
	 * @details
	 * 현재 기본 화면에서 제거된 제목/상태 문구는 여기서 다루지 않는다.
	 * 이 함수는 실제로 남아 있는 조작 버튼만 동기화한다.
	 */
	void SyncCharacterText() const;

	/**
	 * @brief 현재 캐릭터 목록 개수에 맞게 카드 위젯을 만들거나 줄임
	 *
	 * @details
	 * mCharacterOptions.Num()보다 카드가 적으면 새 카드를 만들고,
	 * 카드가 많으면 남는 카드를 제거한다.
	 * 그 뒤 각 카드에 캐릭터 값과 선택 상태를 다시 넣는다.
	 */
	void RebuildCharacterCards();

	/**
	 * @brief 하단 아이콘들의 선택 표시를 현재 선택 인덱스에 맞춤
	 */
	void SyncCharacterCards() const;

	/**
	 * @brief 카드 클릭으로 들어온 캐릭터 번호를 현재 선택으로 바꿈
	 *
	 * @details
	 * bSelectable이 false인 캐릭터도 선택 상태로 바꾼다.
	 * 예를 들어 Archer, Magician은 잠겨 있지만 눌렀을 때 상세 영역에 잠김 사유가 보여야 한다.
	 * 대신 SyncSelectedCharacter()에서 Confirm 버튼을 비활성화하고,
	 * HandleConfirmButtonClicked()에서도 한 번 더 막는다.
	 *
	 * @param CharacterIndex 선택할 캐릭터 배열 번호
	 */
	void SelectCharacter(int32 CharacterIndex);

	/**
	 * @brief 현재 선택된 캐릭터 정보를 상세 영역에 보여줌
	 *
	 * @details
	 * 이름, 역할, 설명, 스탯, 잠김 사유, 초상화를 갱신한다.
	 * 선택 가능한 캐릭터일 때만 Confirm 버튼을 활성화한다.
	 * 잠긴 캐릭터는 선택 표시는 되지만 Confirm 버튼이 꺼져 있어서 다음 단계로 넘어갈 수 없다.
	 */
	void SyncSelectedCharacter();

	/**
	 * @brief 선택할 캐릭터 데이터가 없을 때 상세 영역을 비움
	 */
	void ClearSelectedCharacter();

	/**
	 * @brief 선택된 캐릭터 초상화 이미지를 비동기로 불러옴
	 *
	 * @details
	 * 이미 로드된 텍스처면 바로 표시한다.
	 * 아직 로드되지 않았으면 AssetManager를 통해 비동기 로딩을 요청한다.
	 *
	 * @param Portrait 표시할 초상화 soft pointer
	 */
	void SetPortraitImage(const TSoftObjectPtr<UTexture2D>& Portrait);

	/**
	 * @brief 초상화 비동기 로딩이 끝났을 때 화면에 적용함
	 *
	 * @param PortraitPath 로딩이 끝난 초상화 경로
	 */
	void HandlePortraitLoaded(FSoftObjectPath PortraitPath);

	/**
	 * @brief 초상화 이미지 위젯을 숨김
	 */
	void ClearPortraitImage() const;

	/**
	 * @brief 이전 초상화 비동기 로딩 요청을 취소함
	 *
	 * @details
	 * 선택을 빠르게 바꾸면 이전 캐릭터의 초상화 로딩이 늦게 끝날 수 있다.
	 * 그 경우 잘못된 이미지가 표시되지 않도록 새 요청 전에 이전 요청을 취소한다.
	 */
	void CancelPortraitLoad();

	/**
	 * @brief 상태 문구 값을 갱신하고 부모 위젯에도 알림
	 *
	 * @details
	 * 현재 기본 UI에서는 상태 TextBlock을 만들지 않으므로 화면에는 표시되지 않는다.
	 * 기존 WBP에 StatusText가 남아 있다면 비워 두고 숨긴다.
	 * 델리게이트 방송은 예전 UI나 디버그 위젯이 상태 변화를 받아볼 수 있게 유지한다.
	 *
	 * @param InText 새로 표시할 상태 문구
	 */
	void SetStatusText(const FText& InText);

	/**
	 * @brief 저장해 둔 캐릭터 목록에서 번호에 맞는 항목을 찾음
	 *
	 * @param CharacterIndex 찾을 캐릭터 배열 번호
	 * @return 유효한 번호면 캐릭터 값 주소, 아니면 nullptr
	 */
	const FFrontendCharacterOption* GetCharacterOption(int32 CharacterIndex) const;

	/**
	 * @brief 화면에 꼭 필요한 위젯이 연결됐는지 로그로 확인함
	 */
	void ValidateDesignerBindings() const;

	/**
	 * @brief 카드 클릭 이벤트를 받아 선택 캐릭터를 바꿈
	 *
	 * @param CharacterIndex 클릭된 카드가 가리키는 캐릭터 배열 번호
	 */
	UFUNCTION()
	void HandleCharacterCardClicked(int32 CharacterIndex);

	/**
	 * @brief Confirm 버튼 클릭을 처리함
	 *
	 * @details
	 * 선택 캐릭터가 유효하고 선택 가능한지 먼저 확인한다.
	 * 이번 PR은 UI 구조 작업만 담기 때문에 실제 다음 화면 이동, StartRun, 방 전환은 호출하지 않는다.
	 * 즉, Warrior를 선택하고 CONFIRM을 눌러도 캐릭터 선택 다음 화면으로 넘어가지 않는다.
	 */
	UFUNCTION()
	void HandleConfirmButtonClicked();

	/**
	 * @brief BACK 버튼 클릭을 부모 위젯으로 전달함
	 */
	UFUNCTION()
	void HandleBackToMainButtonClicked();

private:
	/**
	 * @brief 캐릭터 카드 위젯들이 들어갈 패널
	 *
	 * @details
	 * RebuildCharacterCards()가 이 패널 안에 UCharacterCardWidget을 캐릭터 수만큼 넣는다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> CharacterCardContainer;

	/**
	 * @brief 캐릭터 카드 하나를 만들 때 사용할 WBP 클래스
	 *
	 * @details
	 * WBP_CharacterSelect 자산에서 /Game/UI/WBP_CharacterCard로 설정한다.
	 * RebuildCharacterCards()는 캐릭터 데이터 개수만큼 이 WBP 인스턴스를 만든다.
	 * 이건 화면을 C++로 그리는 것이 아니라, 이미 만들어진 WBP 카드 양식을 목록 개수만큼 배치하는 처리다.
	 */
	UPROPERTY(Category = "Character Select", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TSubclassOf<UCharacterCardWidget> CharacterCardWidgetClass;

	/** @brief 상세 영역에 표시할 선택 캐릭터 이름 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SelectedCharacterNameText;

	/** @brief 상세 영역에 표시할 선택 캐릭터 역할 문구 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SelectedCharacterRoleText;

	/** @brief 상세 영역에 표시할 선택 캐릭터 설명 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SelectedCharacterDescriptionText;

	/** @brief 상세 영역에 표시할 선택 캐릭터 스탯 요약 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SelectedCharacterStatText;

	/** @brief 선택할 수 없는 캐릭터를 눌렀을 때 보여줄 사유 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SelectedCharacterDisabledReasonText;

	/** @brief 상세 영역에 표시할 선택 캐릭터 초상화 이미지 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SelectedCharacterPortraitImage;

	/** @brief 선택한 캐릭터를 확정하는 버튼. 이번 PR에서는 다음 화면으로 이동하지 않는다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton;

	/** @brief Confirm 버튼 안에 표시할 라벨 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ConfirmButtonText;

	/** @brief 타이틀 메인 화면으로 돌아가는 버튼 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackToMainButton;

	/** @brief Back 버튼 안에 표시할 라벨 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BackToMainButtonText;

	/**
	 * @brief 예전 캐릭터 선택 화면 아래에 있던 상태 문구
	 *
	 * @details
	 * 지금 기본 UI에서는 "Select a character" 같은 하단 문구를 표시하지 않는다.
	 * 기존 WBP에 StatusText가 남아 있으면 숨기기 위해 Optional로만 받는다.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** @brief 선택 가능한 캐릭터를 고르라고 안내하는 기본 상태 문구 */
	UPROPERTY(Category = "Character Select|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mCharacterSelectStatusText;

	/** @brief Confirm을 눌렀지만 다음 화면 이동이 이번 PR 범위 밖이라 막혔을 때 쓰는 상태 문구 */
	UPROPERTY(Category = "Character Select|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mCharacterTransitionBlockedStatusText;

	/** @brief 고를 수 없는 캐릭터를 눌렀거나 Confirm 했을 때 보여주는 상태 문구 */
	UPROPERTY(Category = "Character Select|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mCharacterUnavailableStatusText;

	/** @brief 선택 인덱스에 해당하는 캐릭터 데이터가 없을 때 보여주는 상태 문구 */
	UPROPERTY(Category = "Character Select|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mCharacterDataMissingStatusText;

	/** @brief Confirm 버튼 기본 문구 */
	UPROPERTY(Category = "Character Select|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mConfirmButtonText;

	/** @brief Back 버튼 기본 문구 */
	UPROPERTY(Category = "Character Select|Text", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FText mBackButtonText;

private:
	/**
	 * @brief 현재 선택한 캐릭터 배열 번호
	 *
	 * @details
	 * mCharacterOptions의 인덱스다.
	 * 목록이 갱신되었을 때 범위를 벗어나면 0으로 보정한다.
	 */
	int32 mSelectedCharacterIndex = 0;

	/**
	 * @brief 현재 화면에 보여줄 캐릭터 목록
	 *
	 * @details
	 * 이번 PR에서는 UI 확인용 임시 목록이다.
	 * UI는 이 배열 개수를 기준으로 카드를 만든다.
	 * 캐릭터가 3명인지 5명인지는 이 배열이 결정한다.
	 */
	TArray<FFrontendCharacterOption> mCharacterOptions;

	/**
	 * @brief 화면에 실제로 만들어 둔 카드 위젯들
	 *
	 * @details
	 * RebuildCharacterCards()가 이 배열을 mCharacterOptions 개수와 맞춘다.
	 */
	UPROPERTY()
	TArray<TObjectPtr<UCharacterCardWidget>> mCharacterCardWidgets;

	/**
	 * @brief 지금 불러오는 중인 초상화 비동기 로딩 핸들
	 */
	TSharedPtr<FStreamableHandle> mPortraitLoadHandle = nullptr;

	/**
	 * @brief 마지막으로 요청한 초상화 경로
	 *
	 * @details
	 * 늦게 완료된 이전 로딩 결과가 현재 선택 캐릭터 이미지를 덮어쓰지 않게 확인할 때 사용한다.
	 */
	FSoftObjectPath mPendingPortraitPath;
};
