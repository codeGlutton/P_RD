#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "GameMode/FrontendGameMode.h"
#include "UI/CharacterSelectWidgetPrivate.h"

/** @brief 선택된 PlayerUnitId로 새 Run 생성을 요청하고 첫 방 전환 시작 여부를 돌려받는다. */
bool UCharacterSelectWidget::BeginFirstRoomEntryWithSelectedCharacter()
{
	AFrontendGameMode* FrontendGameMode = GetFrontendGameMode();
	if (FrontendGameMode == nullptr || !mSelectedPlayerUnitId.IsValid())
	{
		return false;
	}

	// [합의필요] 난이도 선택 UI는 아직 없으므로 프론트엔드 기본 난이도 1로 새 Run 생성을 요청한다.
	// 이 화면은 한 명만 고른다. 파티로 넘기는 자리라 한 명짜리 파티로 싼다 --
	// 런을 만드는 쪽이 한 명인지 셋인지 알 필요는 없다.
	if (!FrontendGameMode->StartNewRun({ mSelectedPlayerUnitId }, 1))
	{
		return false;
	}

	return true;
}

/** @brief 프론트엔드 방의 Auth GameMode를 가져온다; WBP 프리뷰/테스트에서는 nullptr을 허용한다. */
AFrontendGameMode* UCharacterSelectWidget::GetFrontendGameMode() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetAuthGameMode<AFrontendGameMode>();
	}

	return nullptr;
}

/** @brief Confirm 입력을 검증, 중복 방지, GameMode 새 Run 요청으로 연결한다. */
void UCharacterSelectWidget::HandleConfirmButtonClicked()
{
	if (mStartRequested)
	{
		return;
	}

	const FFrontendCharacterOption* SelectedOption = GetSelectedCharacterOption();
	if (SelectedOption == nullptr || !SelectedOption->mSelectable || !mSelectedPlayerUnitId.IsValid())
	{
		// 잠긴 카드는 Confirm만 막고, 상태 문구는 비운다(기획: "준비 중" 문구 노출 삭제).
		SetStatusText(FText::GetEmpty());
		return;
	}

	// 여기서부터는 중복 Confirm/Back 입력을 막고 GameMode의 방 전환 요청 결과만 기다린다.
	// 확정 버튼 라벨은 그대로 두고(기획: "Loading" 문구 노출 삭제) 버튼만 비활성화해 재입력을 막는다.
	mStartRequested = true;
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

/** @brief 시작 요청이 아직 나가지 않은 경우에만 타이틀 메인 복귀를 부모에게 알린다. */
void UCharacterSelectWidget::HandleBackToMainButtonClicked()
{
	if (!mStartRequested)
	{
		OnBackToMainRequested.Broadcast();
	}
}
