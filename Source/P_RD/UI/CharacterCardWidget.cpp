#include "UI/CharacterCardWidget.h"

#include "Components/Button.h"

/** @brief 카드 데이터는 부모가 주입하므로 생성자는 별도 에셋/목록 조회를 하지 않는다. */
UCharacterCardWidget::UCharacterCardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

/** @brief 부모가 내려준 View 값과 선택 상태를 저장한 뒤 WBP 연출 이벤트로 밀어 넣는다. */
void UCharacterCardWidget::SetCharacterOption(const FFrontendCharacterOption& InOption, bool bSelected)
{
	mCharacterOption = InOption;
	mIsSelected = bSelected;
	SyncCard();
}

/** @brief 후보 값은 유지하고 선택 연출 상태만 다시 보낸다. */
void UCharacterCardWidget::SetSelected(bool bSelected)
{
	mIsSelected = bSelected;
	SyncCard();
}

/** @brief 카드 버튼 바인딩을 검증하고 클릭 이벤트를 이 카드의 안정 index 이벤트로 연결한다. */
void UCharacterCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ValidateDesignerBindings();

	if (CardButton != nullptr)
	{
		CardButton->OnClicked.AddUniqueDynamic(this, &UCharacterCardWidget::HandleCardButtonClicked);
	}

	SyncCard();
}

/** @brief 위젯 재사용/화면 이탈 때 OnClicked 델리게이트를 제거한다. */
void UCharacterCardWidget::NativeDestruct()
{
	if (CardButton != nullptr)
	{
		CardButton->OnClicked.RemoveDynamic(this, &UCharacterCardWidget::HandleCardButtonClicked);
	}

	Super::NativeDestruct();
}

/** @brief 현재 View 값과 선택 상태를 BP 카드 연출에 전달한다. */
void UCharacterCardWidget::SyncCard()
{
	if (CardButton != nullptr)
	{
		// 잠긴 캐릭터도 눌러 상세/잠금 사유를 보여준다. 실제 확정 가능 여부는 선택 화면의 mSelectable 검증이 막는다.
		CardButton->SetIsEnabled(true);
	}

	BP_OnCharacterOptionChanged(mCharacterOption, mIsSelected);
}

/** @brief WBP_CharacterCard가 제공해야 하는 루트 버튼 바인딩 누락을 로그로 노출한다. */
void UCharacterCardWidget::ValidateDesignerBindings() const
{
	if (CardButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterCardWidget: WBP_CharacterCard requires CardButton."));
	}
}

/** @brief 클릭을 배열 위치가 아닌 GameMode가 부여한 CharacterOption.mIndex로 부모에게 전달한다. */
void UCharacterCardWidget::HandleCardButtonClicked()
{
	// 배열 위치가 아니라 GameMode가 내려준 안정적인 option index를 올려 보낸다.
	OnCharacterCardClicked.Broadcast(mCharacterOption.mIndex);
}
