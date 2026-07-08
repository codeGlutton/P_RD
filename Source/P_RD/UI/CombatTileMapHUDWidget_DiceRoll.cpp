#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"
#include "Actor/Dice/CombatDiceRollCaptureActor.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"

using namespace RDCombatHUD;


/** @brief 전투 입장 직후 주사위 연출을 "터치 대기" 상태로 초기화한다. */
void UCombatTileMapHUDWidget::PrepareIntroDiceRoll()
{
	EnsureDiceRollPhysicsActor();

	for (FDiceViewData& DiceView : mDiceUIs)
	{
		DiceView.mIsRolled = false;
	}
	RefreshOwnedDiceCards();

	if (mDiceRollPhysicsActor == nullptr || mDiceUIs.IsEmpty())
	{
		SetDiceRollVisibility(ESlateVisibility::Collapsed);
		mIntroDiceRollReady = false;
		mIntroDiceRollActive = false;
		mIntroDiceResultWaitingForDismiss = false;
		return;
	}

	if (mDiceRollPhysicsActor != nullptr)
	{
		TArray<FCombatDiceRollPhysicsSpec> DiceSpecs;
		DiceSpecs.Reserve(mDiceUIs.Num());
		for (const FDiceViewData& DiceView : mDiceUIs)
		{
			FCombatDiceRollPhysicsSpec DiceSpec;
			DiceSpec.mFaceCount = DiceView.mFaceCount;
			DiceSpec.mFaceValues = DiceView.mFaceValues;
			DiceSpec.mFaceTextures = DiceView.mFaceTextures;
			DiceSpec.mDiceColor = RDUIDice::GetDiceRarityColor(DiceView.mRarityType);
			DiceSpecs.Add(MoveTemp(DiceSpec));
		}
		mDiceRollPhysicsActor->ConfigureDice(DiceSpecs);
		mDiceRollPhysicsActor->CaptureDice();
	}

	mIntroDiceRollElapsed = 0.0f;
	mIntroDiceRollActive = false;
	mIntroDiceRollReady = true;
	mIntroDiceResultWaitingForDismiss = false;
	mIntroDiceResultsApplied = false;
	mIntroDiceRollResultsResolved = false;
	mIntroDiceAligning = false;
	mIntroDiceAligned = false;
	mIntroDiceAlignTimer = 0.0f;

	SetDiceRollVisibility(ESlateVisibility::HitTestInvisible);
	if (DiceRollStatusText != nullptr)
	{
		DiceRollStatusText->SetText(FText::GetEmpty());
	}
}

/** @brief 대기 중인 입장 주사위 연출을 실제 굴림 상태로 전환한다. */
void UCombatTileMapHUDWidget::StartIntroDiceRoll()
{
	if (mDiceUIs.IsEmpty() || mDiceRollPhysicsActor == nullptr)
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
	mIntroDiceResultsApplied = false;
	mIntroDiceRollResultsResolved = false;
	mIntroDiceAligning = false;
	mIntroDiceAligned = false;
	mIntroDiceAlignTimer = 0.0f;
	if (mDiceRollPhysicsActor != nullptr)
	{
		mDiceRollPhysicsActor->StartRoll(FMath::Rand());
	}

	SetDiceRollVisibility(ESlateVisibility::HitTestInvisible);
	if (DiceRollStatusText != nullptr)
	{
		DiceRollStatusText->SetText(FText::GetEmpty());
	}
}

/** @brief 회전→결과면 보간→결과 표시까지의 입장 주사위 연출 상태기를 한 프레임 진행한다. */
void UCombatTileMapHUDWidget::UpdateIntroDiceRoll(float InDeltaTime)
{
	if (mDiceRollPhysicsActor != nullptr)
	{
		mIntroDiceRollElapsed += InDeltaTime;

		if (mDiceRollPhysicsActor->IsRollComplete() == false)
		{
			return;
		}

		if (mIntroDiceRollResultsResolved == false)
		{
			// A안: 물리 주사위가 멈춘 윗면을 그대로 게임 결과로 사용한다("보이는 면 = 기록 숫자").
			// 뷰모델이 있으면 물리 윗면을 전투 풀에 주입하고, 단독 시안이면 mDiceUIs를 직접 갱신한다.
			ApplyIntroDicePhysicsResults();
		}

		if (mIntroDiceResultsApplied == false)
		{
			MarkAllDiceRolled();
			mIntroDiceResultsApplied = true;
		}

		// 정렬까지 끝났으면 탭 대기 상태 유지(아무것도 더 안 함).
		if (mIntroDiceAligned == true)
		{
			return;
		}

		// 1) 결과를 잠깐 보여준 뒤(시간 간격) 쫙 정렬을 시작한다.
		if (mIntroDiceAligning == false)
		{
			if (DiceRollStatusText != nullptr)
			{
				DiceRollStatusText->SetText(FText::GetEmpty());
			}

			mIntroDiceAlignTimer += InDeltaTime;
			if (mIntroDiceAlignTimer >= mIntroDiceAlignDelay)
			{
				mDiceRollPhysicsActor->StartAlign();
				mIntroDiceAligning = true;
			}
			return;
		}

		// 2) 정렬 애니메이션 진행 중 — 캡처 액터 Tick이 보간/캡처를 처리한다.
		if (mDiceRollPhysicsActor->IsAlignComplete() == false)
		{
			return;
		}

		// 3) 정렬 완료 → 탭하면 닫기 대기.
		mIntroDiceAligning = false;
		mIntroDiceAligned = true;
		mIntroDiceRollActive = false;
		mIntroDiceResultWaitingForDismiss = true;
		if (DiceRollStatusText != nullptr)
		{
			DiceRollStatusText->SetText(FText::GetEmpty());
		}
		return;
	}

	// 물리 캡처 액터가 없으면(스폰 실패 등) 이번 입장 연출은 종료 상태로 둔다.
	mIntroDiceRollActive = false;
}

/** @brief 굴림 대기 상태에서 기기를 흔들면(가속도 급변) 자동으로 굴림을 시작한다. */
void UCombatTileMapHUDWidget::UpdateShakeToRoll(float InDeltaTime)
{
	if (mShakeRollCooldown > 0.0f)
	{
		mShakeRollCooldown = FMath::Max(0.0f, mShakeRollCooldown - InDeltaTime);
	}

	// 굴릴 준비가 됐고(터치 대기), 아직 굴리는 중이 아닐 때만 흔들기를 받는다.
	if (mIntroDiceRollReady == false || mIntroDiceRollActive == true || mIntroDiceResultWaitingForDismiss == true)
	{
		mHasShakeBaseline = false;   // 대기 구간을 벗어나면 기준값을 리셋해 복귀 시 첫 프레임 오탐을 막는다.
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController == nullptr)
	{
		return;
	}

	// 모바일 가속도 입력을 한 번만 켠다(에디터/PC에서는 0 벡터라 자연히 트리거되지 않음).
	if (mMotionControlsRequested == false)
	{
		PlayerController->SetMotionControlsEnabled(true);
		mMotionControlsRequested = true;
	}

	FVector Tilt = FVector::ZeroVector;
	FVector RotationRate = FVector::ZeroVector;
	FVector Gravity = FVector::ZeroVector;
	FVector Acceleration = FVector::ZeroVector;
	PlayerController->GetInputMotionState(Tilt, RotationRate, Gravity, Acceleration);

	if (mHasShakeBaseline == false)
	{
		mLastShakeAcceleration = Acceleration;
		mHasShakeBaseline = true;
		return;
	}

	// 가속도의 프레임 간 급변(저크)을 흔들기로 본다 — 중력 성분이 빠진 순수 흔드는 힘만 잡힌다.
	const float Jerk = (Acceleration - mLastShakeAcceleration).Size();
	mLastShakeAcceleration = Acceleration;

	if (Jerk >= mShakeTriggerThreshold && mShakeRollCooldown <= 0.0f)
	{
		mShakeRollCooldown = 1.0f;   // 한 번 흔들면 한 번만 굴리게 한다.
		StartIntroDiceRoll();
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

void UCombatTileMapHUDWidget::ApplyIntroDicePhysicsResults()
{
	if (mIntroDiceRollResultsResolved == true)
	{
		return;
	}

	mIntroDiceRollResultsResolved = true;
	if (mDiceRollPhysicsActor == nullptr)
	{
		return;
	}

	// 물리 윗면 ordinal(1-base) → 물리 면 index(0-base) 배열로 변환.
	TArray<int32> FaceOrdinals;
	mDiceRollPhysicsActor->GetSettledFaceOrdinals(FaceOrdinals);
	TArray<int32> RolledFaceIndices;
	RolledFaceIndices.Reserve(FaceOrdinals.Num());
	for (int32 Ordinal : FaceOrdinals)
	{
		RolledFaceIndices.Add(Ordinal - 1);
	}

	// 뷰모델 연결 시: 물리 결과를 전투 풀에 주입해 데미지/세이브까지 같은 값을 쓰게 한 뒤, 표시 데이터를 모델에서 다시 읽는다.
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->RequestApplyDiceResults(RolledFaceIndices);
		RefreshDiceViewsFromRunData();
		for (FDiceViewData& DiceView : mDiceUIs)
		{
			DiceView.mIsRolled = false;   // 결과 표시는 끝났지만 보유 카드 'Rolled' 전환은 MarkAllDiceRolled에서 일괄 처리.
		}
		RefreshOwnedDiceCards();
		return;
	}

	// 단독 시안(뷰모델 없음): mDiceUIs를 직접 갱신.
	for (int32 DiceIndex = 0; DiceIndex < mDiceUIs.Num(); ++DiceIndex)
	{
		if (RolledFaceIndices.IsValidIndex(DiceIndex) == false)
		{
			continue;
		}

		FDiceViewData& DiceView = mDiceUIs[DiceIndex];
		const int32 FaceCount = FMath::Max(1, DiceView.mFaceValues.Num() > 0 ? DiceView.mFaceValues.Num() : DiceView.mFaceCount);
		DiceView.mRolledFaceIndex = FMath::Clamp(RolledFaceIndices[DiceIndex], 0, FaceCount - 1);
		DiceView.mResultValue = DiceView.mFaceValues.IsValidIndex(DiceView.mRolledFaceIndex)
			? DiceView.mFaceValues[DiceView.mRolledFaceIndex]
			: DiceView.mRolledFaceIndex + 1;
		DiceView.mIsRolled = false;
	}

	RefreshOwnedDiceCards();
}

/** @brief 굴림 오버레이의 시각 요소와 입력 레이어를 같은 visibility로 묶어 상태 불일치를 막는다. */
void UCombatTileMapHUDWidget::SetDiceRollCombatLayerSuppressed(bool bSuppressed)
{
	auto SetWidgetVisibility = [](UWidget* Widget, ESlateVisibility InVisibility)
	{
		if (Widget != nullptr)
		{
			Widget->SetVisibility(InVisibility);
		}
	};

	auto SetWidgetArrayVisibility = [&SetWidgetVisibility](const auto& Widgets, ESlateVisibility InVisibility)
	{
		for (const auto& Widget : Widgets)
		{
			SetWidgetVisibility(Widget.Get(), InVisibility);
		}
	};

	if (bSuppressed == true)
	{
		RefreshDiceAssignmentText();
		UpdateUnitHpBars();
		return;
	}

	SetWidgetVisibility(EndTurnButton.Get(), ESlateVisibility::Visible);
	SetWidgetVisibility(mMoveButton.Get(), ESlateVisibility::Visible);

	SetWidgetArrayVisibility(mOwnedDiceImages, ESlateVisibility::HitTestInvisible);
	SetWidgetArrayVisibility(mOwnedDiceCardWidgets, ESlateVisibility::Visible);
	SetWidgetArrayVisibility(mOwnedDiceTypeTexts, ESlateVisibility::Collapsed);
	SetWidgetArrayVisibility(mOwnedDiceValueTexts, ESlateVisibility::HitTestInvisible);
	SetWidgetArrayVisibility(mSkillInputButtons, ESlateVisibility::Visible);
	SetWidgetArrayVisibility(mEquipmentChips, ESlateVisibility::HitTestInvisible);
	SetWidgetArrayVisibility(mEquipmentChipTexts, ESlateVisibility::HitTestInvisible);
	SetWidgetArrayVisibility(mTurnOrderChips, ESlateVisibility::HitTestInvisible);
	SetWidgetArrayVisibility(mTurnOrderChipTexts, ESlateVisibility::HitTestInvisible);

	// 스킬 레일(패널/아이콘/라벨)은 블랭킷 복원 금지 - 미보유 슬롯은 Collapsed가 정상 상태라
	// 강제로 펴면 UBorder 기본 흰 브러시가 그대로 노출된다(굴림 종료 후 흰 박스 버그).
	// 슬롯별 정상 상태는 Refresh가 데이터 기준으로 다시 세운다.
	RefreshSkillRailWidgets();

	RefreshDiceAssignmentText();
	UpdateUnitHpBars();
}

/** @brief 굴림 오버레이의 시각 요소와 입력 레이어를 같은 visibility로 묶어 상태 불일치를 막는다. */
void UCombatTileMapHUDWidget::SetDiceRollVisibility(ESlateVisibility NewVisibility)
{
	const bool bVisible = NewVisibility != ESlateVisibility::Collapsed && NewVisibility != ESlateVisibility::Hidden;

	if (mDiceRollBackdropPanel != nullptr)
	{
		mDiceRollBackdropPanel->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (mDiceRollBoardImage != nullptr)
	{
		mDiceRollBoardImage->SetVisibility(NewVisibility);
	}

	if (mDiceRollPhysicsImage != nullptr)
	{
		mDiceRollPhysicsImage->SetVisibility(NewVisibility);
	}

	if (DiceRollStatusText != nullptr)
	{
		DiceRollStatusText->SetVisibility(ESlateVisibility::Collapsed);
		DiceRollStatusText->SetText(FText::GetEmpty());
	}

	if (mDiceRollInputButton != nullptr)
	{
		mDiceRollInputButton->SetVisibility(NewVisibility == ESlateVisibility::Collapsed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	SetDiceRollCombatLayerSuppressed(bVisible);
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

	StartIntroDiceRoll();
}

/** @brief 결과 확인 후 입장 굴림 오버레이를 닫고 다음 OpenUI에서 다시 준비되게 상태를 비운다. */
void UCombatTileMapHUDWidget::DismissIntroDiceRoll()
{
	mIntroDiceRollActive = false;
	mIntroDiceRollReady = false;
	mIntroDiceResultWaitingForDismiss = false;
	mIntroDiceRollResultsResolved = false;
	mIntroDiceAligning = false;
	mIntroDiceAligned = false;
	mIntroDiceAlignTimer = 0.0f;
	SetDiceRollVisibility(ESlateVisibility::Collapsed);
}

/**
 * @brief 턴 시작 주사위 굴림 요청(OnDiceRollRequested) 수신 — 굴림 오버레이를 연다.
 * @details 첫 턴은 전투 입장 연출이 열지만, 2턴째부터는 이 경로가 유일한 자동 열림이다.
 *          이미 굴리는 중이거나 결과 확인 대기 중이면 중복 시작하지 않는다.
 */
void UCombatTileMapHUDWidget::HandleCombatDiceRollRequested()
{
	if (mIntroDiceRollActive == true || mIntroDiceResultWaitingForDismiss == true)
	{
		return;
	}

	// 바로 굴리지 않고 터치 대기 상태로만 연다 — 굴리는 맛(터치로 굴림)은 기획 확정 사항(06/15 회의).
	// 실제 굴림은 유저 탭(HandleDiceRollInputButtonClicked)이나 흔들기에서 StartIntroDiceRoll로 이어진다.
	PrepareIntroDiceRoll();
}
