#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "GameMode/FrontendGameMode.h"
#include "UI/CharacterSelectWidgetPrivate.h"

bool UCharacterSelectWidget::BeginFirstRoomEntryWithSelectedCharacter()
{
	AFrontendGameMode* FrontendGameMode = GetFrontendGameMode();
	if (FrontendGameMode == nullptr || !mSelectedPlayerUnitId.IsValid())
	{
		return false;
	}

	// 난이도 선택 UI는 아직 없으므로 프론트엔드 기본 난이도 1로 새 Run 생성을 요청한다.
	if (!FrontendGameMode->StartNewRun(mSelectedPlayerUnitId, 1))
	{
		return false;
	}

	return true;
}

AFrontendGameMode* UCharacterSelectWidget::GetFrontendGameMode() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetAuthGameMode<AFrontendGameMode>();
	}

	return nullptr;
}

void UCharacterSelectWidget::HandleConfirmButtonClicked()
{
	if (mStartRequested)
	{
		return;
	}

	const FFrontendCharacterOption* SelectedOption = GetSelectedCharacterOption();
	if (SelectedOption == nullptr || !SelectedOption->mSelectable || !mSelectedPlayerUnitId.IsValid())
	{
		// 잠긴 카드도 선택은 가능하지만 Confirm은 여기서 막고, GameMode가 내려준 잠금 사유를 그대로 보여준다.
		SetStatusText(SelectedOption != nullptr && !SelectedOption->mDisabledReason.IsEmpty()
			? SelectedOption->mDisabledReason
			: RDCharacterSelect::Text(TEXT("CharacterLockedStatus")));
		return;
	}

	// 여기서부터는 중복 Confirm/Back 입력을 막고 GameMode의 방 전환 요청 결과만 기다린다.
	mStartRequested = true;
	SetConfirmButtonText(mLoadingStatusText);
	SetStatusText(mLoadingStatusText);
	if (mConfirmButton != nullptr)
	{
		mConfirmButton->SetIsEnabled(false);
	}

	if (!BeginFirstRoomEntryWithSelectedCharacter())
	{
		mStartRequested = false;
		SetConfirmButtonText(mConfirmText);
		SetStatusText(mFailedStatusText);
		if (mConfirmButton != nullptr)
		{
			mConfirmButton->SetIsEnabled(true);
		}
	}
}

void UCharacterSelectWidget::HandleBackToMainButtonClicked()
{
	if (!mStartRequested)
	{
		OnBackToMainRequested.Broadcast();
	}
}
