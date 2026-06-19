#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"

using namespace RDCombatHUD;

// ShouldUseFaceOnReadyPose / GetDiceReadyRotation 는 Private 헤더 선언 + DicePreview.cpp 정의를 공용으로 쓴다.

/** @brief 전투 입장 직후 주사위 연출을 "터치 대기" 상태로 초기화한다. */
void UCombatTileMapHUDWidget::PrepareIntroDiceRoll()
{
	EnsureDicePreviewActors();

	for (FDiceViewData& DiceView : mDiceUIs)
	{
		DiceView.mIsRolled = false;
	}
	RefreshOwnedDiceCards();

	if (mDicePreviewActors.IsEmpty() || mDiceUIs.IsEmpty())
	{
		SetDiceRollVisibility(ESlateVisibility::Collapsed);
		mIntroDiceRollReady = false;
		mIntroDiceRollActive = false;
		mIntroDiceResultWaitingForDismiss = false;
		return;
	}

	for (int32 DiceIndex = 0; DiceIndex < mDicePreviewActors.Num(); ++DiceIndex)
	{
		if (ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex])
		{
			const FLinearColor DiceColor = mDiceUIs.IsValidIndex(DiceIndex)
				? RDUIDice::GetDiceRarityColor(mDiceUIs[DiceIndex].mRarityType)
				: RDUIDice::GetDiceRarityColor(ERarityType::Common);
			const FDiceViewData* DiceView = mDiceUIs.IsValidIndex(DiceIndex) ? &mDiceUIs[DiceIndex] : nullptr;
			const FRotator StartRotation = GetDiceReadyRotation(DiceView, DiceIndex, DicePreviewActor);
			DicePreviewActor->SetDiceColor(DiceColor);
			DicePreviewActor->SetDiceRotation(StartRotation);
			if (mIntroDiceSettleStartRotations.IsValidIndex(DiceIndex))
			{
				mIntroDiceSettleStartRotations[DiceIndex] = StartRotation;
			}
			DicePreviewActor->CaptureDice();
		}
	}

	mIntroDiceRollElapsed = 0.0f;
	mIntroDiceRollActive = false;
	mIntroDiceRollReady = true;
	mIntroDiceResultWaitingForDismiss = false;
	mIntroDiceSettling = false;
	mIntroDiceResultsApplied = false;

	SetDiceRollVisibility(ESlateVisibility::HitTestInvisible);
	if (DiceRollStatusText != nullptr)
	{
		DiceRollStatusText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceReadyAllFormat", "TAP TO ROLL\n{0} DICE"),
			FText::AsNumber(mDiceUIs.Num())
		));
	}
}

/** @brief 대기 중인 입장 주사위 연출을 실제 굴림 상태로 전환한다. */
void UCombatTileMapHUDWidget::StartIntroDiceRoll()
{
	if (mIntroDiceRollReady == false)
	{
		PrepareIntroDiceRoll();
	}

	if (mIntroDiceRollReady == false || mDicePreviewActors.IsEmpty() || mDiceUIs.IsEmpty())
	{
		SetDiceRollVisibility(ESlateVisibility::Collapsed);
		return;
	}

	for (FDiceViewData& DiceView : mDiceUIs)
	{
		DiceView.mIsRolled = false;
	}
	RefreshOwnedDiceCards();

	mIntroDiceRollElapsed = 0.0f;
	mIntroDiceRollActive = true;
	mIntroDiceRollReady = false;
	mIntroDiceResultWaitingForDismiss = false;
	mIntroDiceSettling = false;
	mIntroDiceResultsApplied = false;

	SetDiceRollVisibility(ESlateVisibility::HitTestInvisible);
	if (DiceRollStatusText != nullptr)
	{
		DiceRollStatusText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceRollingAllFormat", "ROLLING DICE\n{0} DICE"),
			FText::AsNumber(mDiceUIs.Num())
		));
	}
}

/** @brief 회전→결과면 보간→결과 표시까지의 입장 주사위 연출 상태기를 한 프레임 진행한다. */
void UCombatTileMapHUDWidget::UpdateIntroDiceRoll(float InDeltaTime)
{
	if (mDicePreviewActors.IsEmpty())
	{
		mIntroDiceRollActive = false;
		return;
	}

	mIntroDiceRollElapsed += InDeltaTime;

	if (mIntroDiceRollElapsed < mIntroDiceRollDuration)
	{
		const float Alpha = FMath::Clamp(mIntroDiceRollElapsed / mIntroDiceRollDuration, 0.0f, 1.0f);
		// [합의필요] 전체 연출의 마지막 34%를 결과면 정착에 쓴다. UX 타이밍 확정 시 데이터화 대상.
		const float SettleStartAlpha = 0.66f;

		if (Alpha < SettleStartAlpha)
		{
			const float SpinTime = mIntroDiceRollElapsed;
			for (int32 DiceIndex = 0; DiceIndex < mDicePreviewActors.Num(); ++DiceIndex)
			{
				ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex];
				if (DicePreviewActor == nullptr)
				{
					continue;
				}

				const float DiceOffset = StaticCast<float>(DiceIndex);
				// 축별 속도를 일부러 서로 다르게 둬 같은 결과라도 주사위마다 반복 패턴이 덜 보이게 한다.
				const FRotator RollingRotation(
					26.0f + SpinTime * (980.0f + DiceOffset * 145.0f),
					-35.0f + SpinTime * (1360.0f + DiceOffset * 125.0f),
					18.0f + SpinTime * (720.0f + DiceOffset * 95.0f)
				);

				if (mIntroDiceSettleStartRotations.IsValidIndex(DiceIndex))
				{
					mIntroDiceSettleStartRotations[DiceIndex] = RollingRotation;
				}
				DicePreviewActor->SetDiceRotation(RollingRotation);
				DicePreviewActor->CaptureDice();
			}
			return;
		}

		if (mIntroDiceSettling == false)
		{
			mIntroDiceSettling = true;
		}

		const float SettleAlpha = EaseOutCubic((Alpha - SettleStartAlpha) / (1.0f - SettleStartAlpha));
		for (int32 DiceIndex = 0; DiceIndex < mDicePreviewActors.Num(); ++DiceIndex)
		{
			ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex];
			if (DicePreviewActor == nullptr || mIntroDiceSettleStartRotations.IsValidIndex(DiceIndex) == false)
			{
				continue;
			}

			const int32 SettledFaceOrdinal = mDiceUIs.IsValidIndex(DiceIndex) ? GetDiceSettledFaceOrdinal(mDiceUIs[DiceIndex]) : 1;
			const FQuat StartQuat = mIntroDiceSettleStartRotations[DiceIndex].Quaternion();
			const FQuat EndQuat = DicePreviewActor->GetSettledFaceRotation(SettledFaceOrdinal).Quaternion();
			DicePreviewActor->SetDiceRotation(FQuat::Slerp(StartQuat, EndQuat, SettleAlpha).Rotator());
			DicePreviewActor->CaptureDice();
		}
		return;
	}

	for (int32 DiceIndex = 0; DiceIndex < mDicePreviewActors.Num(); ++DiceIndex)
	{
		if (ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex])
		{
			const int32 SettledFaceOrdinal = mDiceUIs.IsValidIndex(DiceIndex) ? GetDiceSettledFaceOrdinal(mDiceUIs[DiceIndex]) : 1;
			DicePreviewActor->SettleToFace(SettledFaceOrdinal);
			DicePreviewActor->CaptureDice();
		}
	}

	if (mIntroDiceResultsApplied == false)
	{
		MarkAllDiceRolled();
		mIntroDiceResultsApplied = true;
	}

	const float HoldElapsed = mIntroDiceRollElapsed - mIntroDiceRollDuration;
	if (DiceRollStatusText != nullptr)
	{
		FString ResultText;
		for (int32 DiceIndex = 0; DiceIndex < mDiceUIs.Num(); ++DiceIndex)
		{
			if (DiceIndex > 0)
			{
				ResultText += TEXT(" / ");
			}
			ResultText += FString::FromInt(mDiceUIs[DiceIndex].mResultValue);
		}

		DiceRollStatusText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceResultAllFormat", "DICE RESULT\n{0}"),
			FText::FromString(ResultText)
		));
	}

	if (HoldElapsed >= mIntroDiceHoldDuration)
	{
		mIntroDiceRollActive = false;
		mIntroDiceResultWaitingForDismiss = true;
		if (DiceRollStatusText != nullptr)
		{
			FString ResultText;
			for (int32 DiceIndex = 0; DiceIndex < mDiceUIs.Num(); ++DiceIndex)
			{
				if (DiceIndex > 0)
				{
					ResultText += TEXT(" / ");
				}
				ResultText += FString::FromInt(mDiceUIs[DiceIndex].mResultValue);
			}

			DiceRollStatusText->SetText(FText::Format(
				NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceResultDismissAllFormat", "DICE RESULT\n{0}\nTAP TO CLOSE"),
				FText::FromString(ResultText)
			));
		}
	}
}

/** @brief 결과 연출이 끝난 시점에 보유 주사위 카드 상태를 한 번만 Rolled로 전환한다. */
void UCombatTileMapHUDWidget::MarkAllDiceRolled()
{
	for (FDiceViewData& DiceView : mDiceUIs)
	{
		DiceView.mIsRolled = true;
	}
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}

/** @brief 굴림 오버레이의 시각 요소와 입력 레이어를 같은 visibility로 묶어 상태 불일치를 막는다. */
void UCombatTileMapHUDWidget::SetDiceRollVisibility(ESlateVisibility NewVisibility) const
{
	for (UImage* DiceImage : mDiceRollImages)
	{
		if (DiceImage != nullptr)
		{
			DiceImage->SetVisibility(NewVisibility);
		}
	}

	if (DiceRollStatusText != nullptr)
	{
		DiceRollStatusText->SetVisibility(NewVisibility);
	}

	if (mDiceRollInputButton != nullptr)
	{
		mDiceRollInputButton->SetVisibility(NewVisibility == ESlateVisibility::Collapsed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

/** @brief 입장 굴림 입력의 단일 진입점. 뷰모델 연결 시 결과 생성은 전투 계층에 위임한다. */
void UCombatTileMapHUDWidget::HandleDiceRollInputButtonClicked()
{
	if (mIntroDiceRollActive == true)
	{
		return;
	}

	if (mIntroDiceResultWaitingForDismiss == true)
	{
		DismissIntroDiceRoll();
		return;
	}

	// 뷰모델이 연결돼 있으면 굴림은 게임플레이가 수행한다(의도만 전달). 결과가 뷰모델에 들어온 뒤
	// 그 값으로 연출을 시작하도록 mDiceUIs를 다시 읽는다. 미연결 시에는 기존 단독 연출.
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->RequestRollDice();
		RefreshDiceViewsFromRunData();
	}

	StartIntroDiceRoll();
}

/** @brief 결과 확인 후 입장 굴림 오버레이를 닫고 다음 OpenUI에서 다시 준비되게 상태를 비운다. */
void UCombatTileMapHUDWidget::DismissIntroDiceRoll()
{
	mIntroDiceRollActive = false;
	mIntroDiceRollReady = false;
	mIntroDiceResultWaitingForDismiss = false;
	SetDiceRollVisibility(ESlateVisibility::Collapsed);
}
