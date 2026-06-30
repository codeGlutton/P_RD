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
#include "GameMode/CombatGameMode.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"

using namespace RDCombatHUD;

// ShouldUseFaceOnReadyPose / GetDiceReadyRotation 는 Private 헤더 선언 + DicePreview.cpp 정의를 공용으로 쓴다.

namespace
{
	constexpr float DiceRollSettleStartAlpha = 0.78f;
	constexpr float DiceRollFaceSettleCompleteAlpha = 0.94f;
	constexpr float DiceRollPi = 3.14159265358979323846f;
	constexpr bool bSkipFinalDiceRollSettleForTest = false;

	struct FDiceRollVisualState
	{
		FVector2D Offset = FVector2D::ZeroVector;
		float HopAlpha = 0.0f;
		float ImpactAlpha = 0.0f;
		FVector2D DiceScale = FVector2D::UnitVector;
		FVector2D ShadowScale = FVector2D::UnitVector;
		float ShadowOpacity = 0.68f;
		float TiltAngle = 0.0f;
	};

	float SmoothStep01(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return ClampedAlpha * ClampedAlpha * (3.0f - 2.0f * ClampedAlpha);
	}

	float EaseOutQuad(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return 1.0f - (1.0f - ClampedAlpha) * (1.0f - ClampedAlpha);
	}

	float SinePulse(float Alpha, float StartAlpha, float EndAlpha)
	{
		if (Alpha <= StartAlpha || Alpha >= EndAlpha)
		{
			return 0.0f;
		}

		return FMath::Sin(((Alpha - StartAlpha) / (EndAlpha - StartAlpha)) * DiceRollPi);
	}

	float CollisionPulse(float Alpha, float CenterAlpha, float Width)
	{
		const float Distance = FMath::Abs(Alpha - CenterAlpha);
		const float Pulse = 1.0f - FMath::Clamp(Distance / Width, 0.0f, 1.0f);
		return Pulse * Pulse * (3.0f - 2.0f * Pulse);
	}

	float GetDiceClusterPullX(int32 DiceIndex, int32 DiceCount)
	{
		if (DiceCount <= 1)
		{
			return 0.0f;
		}

		const float CenterIndex = (StaticCast<float>(DiceCount) - 1.0f) * 0.5f;
		const float NormalizedIndex = (StaticCast<float>(DiceIndex) - CenterIndex) / FMath::Max(1.0f, CenterIndex);
		return -NormalizedIndex * 138.0f;
	}

	float GetDiceRollImpactAlpha(int32 DiceIndex, float TravelAlpha)
	{
		const float DicePhase = StaticCast<float>(DiceIndex % 4) * 0.013f;
		return FMath::Clamp(
			CollisionPulse(TravelAlpha, 0.18f + DicePhase, 0.042f) * 0.90f
			+ CollisionPulse(TravelAlpha, 0.34f - DicePhase * 0.65f, 0.052f) * 0.78f
			+ CollisionPulse(TravelAlpha, 0.51f + DicePhase * 0.45f, 0.050f) * 0.68f
			+ CollisionPulse(TravelAlpha, 0.66f - DicePhase * 0.35f, 0.044f) * 0.52f,
			0.0f,
			1.0f
		);
	}

	float GetDiceRollFinalThumpAlpha(int32 DiceIndex, float Alpha)
	{
		const float DiceDelay = StaticCast<float>(DiceIndex % 4) * 0.009f;
		return FMath::Clamp(
			CollisionPulse(Alpha, 0.835f + DiceDelay, 0.035f) * 0.95f
			+ CollisionPulse(Alpha, 0.930f + DiceDelay * 0.5f, 0.026f) * 0.45f,
			0.0f,
			1.0f
		);
	}

	float GetDiceRollBoardImpactAlpha(float Alpha)
	{
		return FMath::Clamp(
			CollisionPulse(Alpha, 0.17f, 0.036f) * 0.70f
			+ CollisionPulse(Alpha, 0.31f, 0.044f) * 0.62f
			+ CollisionPulse(Alpha, 0.48f, 0.044f) * 0.55f
			+ CollisionPulse(Alpha, 0.64f, 0.040f) * 0.45f
			+ CollisionPulse(Alpha, 0.84f, 0.040f) * 0.72f,
			0.0f,
			1.0f
		);
	}

	FDiceRollVisualState GetDiceRollVisualState(int32 DiceIndex, int32 DiceCount, float NormalizedRollAlpha)
	{
		FDiceRollVisualState State;
		const float Alpha = FMath::Clamp(NormalizedRollAlpha, 0.0f, 1.0f);
		const float TravelAlpha = FMath::Clamp(Alpha / DiceRollSettleStartAlpha, 0.0f, 1.0f);
		const float SettleAlpha = Alpha > DiceRollSettleStartAlpha
			? EaseOutCubic((Alpha - DiceRollSettleStartAlpha) / (1.0f - DiceRollSettleStartAlpha))
			: 0.0f;
		const float SettleWeight = 1.0f - SettleAlpha;
		const float GatherWeight = SinePulse(TravelAlpha, 0.02f, 0.78f) * SettleWeight;
		const float MixWeight = SinePulse(TravelAlpha, 0.06f, 0.92f) * SettleWeight;
		const float SlowdownWeight = 1.0f - SmoothStep01(TravelAlpha);
		const float PairDirection = (DiceIndex % 2 == 0) ? -1.0f : 1.0f;
		const float RowDirection = (DiceIndex % 3 == 0) ? -1.0f : 1.0f;
		const float Phase = StaticCast<float>(DiceIndex) * 1.371f;
		const float ImpactAlpha = GetDiceRollImpactAlpha(DiceIndex, TravelAlpha) * SettleWeight;
		const float FinalThumpAlpha = GetDiceRollFinalThumpAlpha(DiceIndex, Alpha);

		const float OrbitRadiusX = (82.0f + 18.0f * FMath::Sin(Phase)) * MixWeight * (0.78f + 0.22f * SlowdownWeight);
		const float OrbitRadiusY = (34.0f + 10.0f * FMath::Cos(Phase * 0.7f)) * MixWeight * (0.70f + 0.30f * SlowdownWeight);
		const float OrbitX = FMath::Sin(TravelAlpha * DiceRollPi * 4.1f + Phase) * OrbitRadiusX;
		const float OrbitY = FMath::Cos(TravelAlpha * DiceRollPi * 3.2f + Phase * 0.84f) * OrbitRadiusY;
		const float ClusterPullX = GetDiceClusterPullX(DiceIndex, DiceCount) * GatherWeight;
		const float ClusterPullY = RowDirection * 22.0f * GatherWeight;
		const float PairKnockX = PairDirection * ImpactAlpha * (58.0f + StaticCast<float>(DiceIndex % 3) * 9.0f);
		const float PairKnockY = RowDirection * ImpactAlpha * (14.0f + StaticCast<float>(DiceIndex % 2) * 5.0f);
		const float MicroJitterX = FMath::Sin(TravelAlpha * DiceRollPi * 14.0f + Phase * 1.7f) * 5.0f * SlowdownWeight * SettleWeight;
		const float MicroJitterY = FMath::Cos(TravelAlpha * DiceRollPi * 12.0f + Phase * 1.3f) * 2.5f * SlowdownWeight * SettleWeight;

		State.HopAlpha = FMath::Clamp(
			(FMath::Abs(FMath::Sin(TravelAlpha * DiceRollPi * 5.2f + Phase)) * 0.12f + ImpactAlpha * 0.26f) * MixWeight
			+ FinalThumpAlpha * 0.12f,
			0.0f,
			0.42f
		);
		State.ImpactAlpha = FMath::Clamp(ImpactAlpha + FinalThumpAlpha, 0.0f, 1.0f);
		const float HopY = -8.0f * State.HopAlpha;

		State.Offset = FVector2D(
			FMath::Clamp(ClusterPullX + OrbitX + PairKnockX + MicroJitterX, -164.0f, 164.0f),
			FMath::Clamp(ClusterPullY + OrbitY + PairKnockY + MicroJitterY + HopY, -70.0f, 70.0f)
		);

		const float AirScale = State.HopAlpha * 0.014f;
		State.DiceScale = FVector2D(
			1.0f + AirScale + State.ImpactAlpha * 0.045f,
			1.0f + AirScale - State.ImpactAlpha * 0.035f
		);
		State.ShadowScale = FVector2D(
			1.04f + State.HopAlpha * 0.08f + State.ImpactAlpha * 0.18f,
			0.92f - State.HopAlpha * 0.04f + State.ImpactAlpha * 0.03f
		);
		State.ShadowOpacity = FMath::Clamp(0.78f - State.HopAlpha * 0.08f + State.ImpactAlpha * 0.16f, 0.58f, 0.92f);
		State.TiltAngle = FMath::Clamp(
			FMath::Sin(TravelAlpha * DiceRollPi * 5.7f + Phase) * 3.8f * MixWeight
			+ PairDirection * State.ImpactAlpha * 3.6f,
			-6.0f,
			6.0f
		);
		return State;
	}

	FVector2D GetDiceRollGroundVelocity(int32 DiceIndex, int32 DiceCount, float NormalizedRollAlpha)
	{
		const float CurrentAlpha = FMath::Clamp(NormalizedRollAlpha, 0.0f, DiceRollSettleStartAlpha);
		const float PreviousAlpha = FMath::Max(0.0f, CurrentAlpha - 0.018f);
		const FVector2D CurrentOffset = GetDiceRollVisualState(DiceIndex, DiceCount, CurrentAlpha).Offset;
		const FVector2D PreviousOffset = GetDiceRollVisualState(DiceIndex, DiceCount, PreviousAlpha).Offset;
		return CurrentOffset - PreviousOffset;
	}

	FRotator GetDiceRollSpinRotation(int32 DiceIndex, int32 DiceCount, float NormalizedRollAlpha)
	{
		const float SpinAlpha = FMath::Clamp(NormalizedRollAlpha / DiceRollSettleStartAlpha, 0.0f, 1.0f);
		const float DiceOffset = StaticCast<float>(DiceIndex);
		const float Phase = DiceOffset * 1.371f;
		const float SpinTravel = EaseOutQuad(SpinAlpha);
		const float ImpactAlpha = GetDiceRollImpactAlpha(DiceIndex, SpinAlpha);
		const FVector2D GroundVelocity = GetDiceRollGroundVelocity(DiceIndex, DiceCount, NormalizedRollAlpha);
		const float GroundSpeed = GroundVelocity.Size();
		const float GroundSpeedAlpha = FMath::Clamp(GroundSpeed / 10.0f, 0.0f, 1.0f);
		const float DirectionRadians = GroundVelocity.SizeSquared() > 0.25f
			? FMath::Atan2(GroundVelocity.Y, GroundVelocity.X)
			: Phase;
		const float DirectionDegrees = FMath::RadiansToDegrees(DirectionRadians);
		const float RollDistance = SpinTravel * (760.0f + DiceOffset * 64.0f);
		const float GroundGrip = FMath::Lerp(0.48f, 1.0f, GroundSpeedAlpha);
		const float ContactRock = ImpactAlpha * (16.0f + GroundSpeedAlpha * 10.0f);
		const float Wobble = FMath::Sin(SpinAlpha * DiceRollPi * 5.0f + Phase) * (1.0f - SmoothStep01(SpinAlpha)) * 8.0f;

		return FRotator(
			22.0f + FMath::Cos(DirectionRadians) * RollDistance * GroundGrip + ContactRock,
			DirectionDegrees * 0.55f + DiceOffset * 28.0f + FMath::Sin(SpinAlpha * DiceRollPi * 1.7f + Phase) * 12.0f,
			14.0f - FMath::Sin(DirectionRadians) * RollDistance * GroundGrip + Wobble - ContactRock * 0.35f
		);
	}
}

/** @brief 전투 입장 직후 주사위 연출을 "터치 대기" 상태로 초기화한다. */
void UCombatTileMapHUDWidget::PrepareIntroDiceRoll()
{
	EnsureDiceRollPhysicsActor();

	for (UImage* DiceImage : mDiceRollImages)
	{
		if (DiceImage != nullptr)
		{
			DiceImage->RemoveFromParent();
		}
	}
	mDiceRollImages.Reset();
	for (UImage* ShadowImage : mDiceRollShadowImages)
	{
		if (ShadowImage != nullptr)
		{
			ShadowImage->RemoveFromParent();
		}
	}
	mDiceRollShadowImages.Reset();
	DestroyDiceCaptureActors(mDicePreviewActors);
	mIntroDiceSettleStartRotations.Reset();

	// 연출 준비: 보유 카드를 'pending' 모양으로(권위 mIsRolled는 그대로, 연출 게이트만 닫는다).
	mIntroDiceCardsRevealed = false;
	RefreshOwnedDiceCards();

	if ((mDiceRollPhysicsActor == nullptr || mDiceUIs.IsEmpty()) && mDicePreviewActors.IsEmpty())
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
			DiceSpec.mDiceColor = RDUIDice::GetDiceRarityColor(DiceView);
			DiceSpecs.Add(MoveTemp(DiceSpec));
		}
		mDiceRollPhysicsActor->ConfigureDice(DiceSpecs);
		mDiceRollPhysicsActor->CaptureDice();
	}

	mIntroDiceRollElapsed = 0.0f;
	mIntroDiceRollActive = false;
	mIntroDiceRollReady = true;
	mIntroDiceResultWaitingForDismiss = false;
	mIntroDiceSettling = false;
	mIntroDiceResultsApplied = false;
	mIntroDiceRollResultsResolved = false;
	mIntroDiceAligning = false;
	mIntroDiceAligned = false;
	mIntroDiceAlignTimer = 0.0f;
	ResetDiceRollWidgetTransforms();

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

	if (mIntroDiceRollReady == false || mDiceUIs.IsEmpty() || (mDiceRollPhysicsActor == nullptr && mDicePreviewActors.IsEmpty()))
	{
		SetDiceRollVisibility(ESlateVisibility::Collapsed);
		return;
	}

	mIntroDiceCardsRevealed = false;
	RefreshOwnedDiceCards();

	mIntroDiceRollElapsed = 0.0f;
	mIntroDiceRollActive = true;
	mIntroDiceRollReady = false;
	mIntroDiceResultWaitingForDismiss = false;
	mIntroDiceSettling = false;
	mIntroDiceResultsApplied = false;
	mIntroDiceRollResultsResolved = false;
	mIntroDiceAligning = false;
	mIntroDiceAligned = false;
	mIntroDiceAlignTimer = 0.0f;
	UpdateDiceRollWidgetTransforms(0.0f);
	if (mDiceRollPhysicsActor != nullptr)
	{
		// 연출 전용 시드(결과값과 무관). UI는 RNG로 결과를 만들지 않으므로 굴림마다 증가하는 결정적 값만 넘긴다.
		mDiceRollPhysicsActor->StartRoll(static_cast<int32>(++mIntroDiceVisualSeed));
	}

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
	if (mDiceRollPhysicsActor != nullptr)
	{
		mIntroDiceRollElapsed += InDeltaTime;

		if (mDiceRollPhysicsActor->IsRollComplete() == false)
		{
			return;
		}

		if (mIntroDiceRollResultsResolved == false)
		{
			// 물리 텀블은 연출 전용이다. 권위 결과값은 게임플레이가 정하며(Phase 2에서 경계로 주입),
			// 위젯은 그 값을 읽어(RefreshDiceViewsFromUIModel) 표시만 한다. 물리 윗면은 결과를 결정하지 않는다.
			ApplyIntroDicePhysicsResults();
		}

		if (mIntroDiceResultsApplied == false)
		{
			MarkAllDiceRolled();
			mIntroDiceResultsApplied = true;
		}

		// 결과 숫자 문자열(공용 빌더). 정렬된 주사위 줄과 맞추기 위해 값 오름차순으로 표시한다.
		auto BuildResultText = [this]() -> FString
		{
			TArray<int32> SortedValues;
			SortedValues.Reserve(mDiceUIs.Num());
			for (const FDiceViewData& DiceView : mDiceUIs)
			{
				SortedValues.Add(DiceView.mResultValue);
			}
			SortedValues.Sort();

			FString ResultText;
			for (int32 Index = 0; Index < SortedValues.Num(); ++Index)
			{
				if (Index > 0)
				{
					ResultText += TEXT(" / ");
				}
				ResultText += FString::FromInt(SortedValues[Index]);
			}
			return ResultText;
		};

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
				DiceRollStatusText->SetText(FText::Format(
					NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceResultAllFormat", "DICE RESULT\n{0}"),
					FText::FromString(BuildResultText())
				));
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
			DiceRollStatusText->SetText(FText::Format(
				NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceResultDismissAllFormat", "DICE RESULT\n{0}\nTAP TO CLOSE"),
				FText::FromString(BuildResultText())
			));
		}
		return;
	}

	if (mDicePreviewActors.IsEmpty())
	{
		mIntroDiceRollActive = false;
		return;
	}

	mIntroDiceRollElapsed += InDeltaTime;

	if (mIntroDiceRollElapsed < mIntroDiceRollDuration)
	{
		const float Alpha = FMath::Clamp(mIntroDiceRollElapsed / mIntroDiceRollDuration, 0.0f, 1.0f);
		const float VisualAlpha = bSkipFinalDiceRollSettleForTest == true
			? FMath::Min(Alpha, DiceRollSettleStartAlpha - KINDA_SMALL_NUMBER)
			: Alpha;
		UpdateDiceRollWidgetTransforms(VisualAlpha);

		if (Alpha < DiceRollSettleStartAlpha || bSkipFinalDiceRollSettleForTest == true)
		{
			for (int32 DiceIndex = 0; DiceIndex < mDicePreviewActors.Num(); ++DiceIndex)
			{
				ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex];
				if (DicePreviewActor == nullptr)
				{
					continue;
				}

				const FRotator RollingRotation = GetDiceRollSpinRotation(DiceIndex, mDicePreviewActors.Num(), Alpha);

				if (mIntroDiceSettleStartRotations.IsValidIndex(DiceIndex))
				{
					mIntroDiceSettleStartRotations[DiceIndex] = RollingRotation;
				}
				DicePreviewActor->SetDiceRotation(RollingRotation);
				DicePreviewActor->CaptureDice();
			}

			if (Alpha >= DiceRollSettleStartAlpha)
			{
				if (mIntroDiceSettling == false)
				{
					mIntroDiceSettling = true;
				}
				ResolveIntroDiceRollResults();
			}
			return;
		}

		if (mIntroDiceSettling == false)
		{
			mIntroDiceSettling = true;
		}
		ResolveIntroDiceRollResults();

		const float FaceSettleAlpha = FMath::Clamp(
			(Alpha - DiceRollSettleStartAlpha) / (DiceRollFaceSettleCompleteAlpha - DiceRollSettleStartAlpha),
			0.0f,
			1.0f
		);
		const float SettleAlpha = EaseOutCubic(FaceSettleAlpha);
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
	UpdateDiceRollWidgetTransforms(1.0f);

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
		if (ACombatGameMode* CombatGameMode = GetWorld()->GetAuthGameMode<ACombatGameMode>())
		{
			CombatGameMode->RollDices();
		}
		StartIntroDiceRoll();
	}
}

/** @brief 결과 연출이 끝난 시점에 보유 주사위 카드 상태를 한 번만 Rolled로 전환한다. */
void UCombatTileMapHUDWidget::MarkAllDiceRolled()
{
	// 연출 종료: 카드 드러내기 게이트만 연다. 어떤 주사위가 'rolled'인지는 권위(mIsRolled)가 결정한다.
	mIntroDiceCardsRevealed = true;
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}

void UCombatTileMapHUDWidget::ResolveIntroDiceRollResults()
{
	if (mIntroDiceRollResultsResolved == true)
	{
		return;
	}

	mIntroDiceRollResultsResolved = true;

	// 굴림 결과의 권위는 GameMode 입력 → DicePoolModel → CombatUIModel push 경로에 있다.
	RefreshDiceViewsFromUIModel();

	for (int32 DiceIndex = 0; DiceIndex < mDiceUIs.Num(); ++DiceIndex)
	{
		FDiceViewData& DiceView = mDiceUIs[DiceIndex];

		if (mDicePreviewActors.IsValidIndex(DiceIndex))
		{
			if (ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex])
			{
				DicePreviewActor->SetDiceType(DiceView.mFaceCount);
				DicePreviewActor->SetFaceData(DiceView.mFaceValues, DiceView.mFaceTextures);
				DicePreviewActor->SetDiceColor(RDUIDice::GetDiceRarityColor(DiceView));
			}
		}
	}

	RefreshOwnedDiceCards();
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

	// 굴림 결과의 권위는 GameMode 입력 → DicePoolModel → CombatUIModel push 경로에 있다.
	RefreshDiceViewsFromUIModel();
	// 카드 드러내기는 MarkAllDiceRolled에서 게이트를 열 때 일괄 처리한다(권위 mIsRolled는 건드리지 않는다).
	RefreshOwnedDiceCards();
}

void UCombatTileMapHUDWidget::UpdateDiceRollWidgetTransforms(float NormalizedRollAlpha) const
{
	const float ClampedAlpha = FMath::Clamp(NormalizedRollAlpha, 0.0f, 1.0f);
	if (mDiceRollBoardImage != nullptr)
	{
		const float BoardImpact = GetDiceRollBoardImpactAlpha(ClampedAlpha);
		mDiceRollBoardImage->SetRenderTranslation(FVector2D(
			FMath::Sin(ClampedAlpha * DiceRollPi * 18.0f) * 4.0f * BoardImpact,
			FMath::Cos(ClampedAlpha * DiceRollPi * 14.0f) * 3.0f * BoardImpact
		));
		mDiceRollBoardImage->SetRenderScale(FVector2D(1.0f + BoardImpact * 0.006f, 1.0f + BoardImpact * 0.004f));
	}

	for (int32 DiceIndex = 0; DiceIndex < mDiceRollImages.Num(); ++DiceIndex)
	{
		const FDiceRollVisualState VisualState = GetDiceRollVisualState(DiceIndex, mDiceRollImages.Num(), ClampedAlpha);

		if (UImage* DiceImage = mDiceRollImages[DiceIndex])
		{
			DiceImage->SetRenderTranslation(VisualState.Offset);
			DiceImage->SetRenderScale(VisualState.DiceScale);
			DiceImage->SetRenderTransformAngle(VisualState.TiltAngle);
			DiceImage->SetRenderOpacity(1.0f);
		}

		if (mDiceRollShadowImages.IsValidIndex(DiceIndex))
		{
			if (UImage* ShadowImage = mDiceRollShadowImages[DiceIndex])
			{
				ShadowImage->SetRenderTranslation(FVector2D(
					VisualState.Offset.X * 0.94f,
					VisualState.Offset.Y * 0.88f + 5.0f + VisualState.HopAlpha * 5.0f + VisualState.ImpactAlpha * 1.5f
				));
				ShadowImage->SetRenderScale(VisualState.ShadowScale);
				ShadowImage->SetRenderOpacity(VisualState.ShadowOpacity);
			}
		}
	}
}

void UCombatTileMapHUDWidget::ResetDiceRollWidgetTransforms() const
{
	if (mDiceRollBoardImage != nullptr)
	{
		mDiceRollBoardImage->SetRenderTranslation(FVector2D::ZeroVector);
		mDiceRollBoardImage->SetRenderScale(FVector2D::UnitVector);
		mDiceRollBoardImage->SetRenderTransformAngle(0.0f);
	}

	for (UImage* DiceImage : mDiceRollImages)
	{
		if (DiceImage != nullptr)
		{
			DiceImage->SetRenderTranslation(FVector2D::ZeroVector);
			DiceImage->SetRenderScale(FVector2D::UnitVector);
			DiceImage->SetRenderTransformAngle(0.0f);
			DiceImage->SetRenderOpacity(1.0f);
		}
	}

	for (UImage* ShadowImage : mDiceRollShadowImages)
	{
		if (ShadowImage != nullptr)
		{
			ShadowImage->SetRenderTranslation(FVector2D::ZeroVector);
			ShadowImage->SetRenderScale(FVector2D::UnitVector);
			ShadowImage->SetRenderOpacity(0.68f);
		}
	}
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
	SetWidgetArrayVisibility(mOwnedDiceTypeTexts, ESlateVisibility::HitTestInvisible);
	SetWidgetArrayVisibility(mSkillRailPanels, ESlateVisibility::HitTestInvisible);
	SetWidgetArrayVisibility(mSkillRailTexts, ESlateVisibility::HitTestInvisible);
	SetWidgetArrayVisibility(mSkillInputButtons, ESlateVisibility::Visible);
	SetWidgetArrayVisibility(mEquipmentChips, ESlateVisibility::HitTestInvisible);
	SetWidgetArrayVisibility(mEquipmentChipTexts, ESlateVisibility::HitTestInvisible);
	SetWidgetArrayVisibility(mTurnOrderChips, ESlateVisibility::HitTestInvisible);
	SetWidgetArrayVisibility(mTurnOrderChipTexts, ESlateVisibility::HitTestInvisible);

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

	for (UImage* ShadowImage : mDiceRollShadowImages)
	{
		if (ShadowImage != nullptr)
		{
			ShadowImage->SetVisibility(NewVisibility);
		}
	}

	for (UImage* DiceImage : mDiceRollImages)
	{
		if (DiceImage != nullptr)
		{
			DiceImage->SetVisibility(NewVisibility);
		}
	}

	if (DiceRollStatusText != nullptr)
	{
		DiceRollStatusText->SetText(FText::GetEmpty());
		DiceRollStatusText->SetVisibility(ESlateVisibility::Collapsed);
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

	if (ACombatGameMode* CombatGameMode = GetWorld()->GetAuthGameMode<ACombatGameMode>())
	{
		CombatGameMode->RollDices();
	}
	StartIntroDiceRoll();
}

void UCombatTileMapHUDWidget::HandleShowDicePanelAnyTurn()
{
	RefreshDiceViewsFromUIModel();
	RebuildOwnedDiceCards();
	PrepareIntroDiceRoll();
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
	ResetDiceRollWidgetTransforms();
	SetDiceRollVisibility(ESlateVisibility::Collapsed);
}
