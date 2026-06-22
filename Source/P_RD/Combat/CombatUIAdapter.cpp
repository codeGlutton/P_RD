#include "Combat/CombatUIAdapter.h"

#include "Actor/TileMap/TileMap.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "ObjectView.h"
#include "RDCollision.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Dice/DicePoolModel.h"
#include "Dice/DiceModel.h"
#include "GameFramework/PlayerController.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/DiceViewData.h"

/** @brief 전투 서브시스템에 연결하고 턴 종료 구독 + 초기 주사위 push를 수행한다. */
void UCombatUIAdapter::Build(USRPGCombatSubsystem* InCombat, const URunPersistData* /*InRun*/)
{
	// 재빌드 대비: 이전 subsystem 턴 종료 구독을 먼저 해제한다.
	if (mCombat != nullptr && mEndTurnHandle.IsValid())
	{
		mCombat->GetModel<USRPGCombatModel>()->OnEndAnyTurnUI.Remove(mEndTurnHandle);
		mEndTurnHandle.Reset();
	}

	mCombat = InCombat;

	if (mCombat != nullptr)
	{
		// 턴이 끝날 때마다 이번 턴에 쓴 주사위 잠금을 해제하도록 훅을 건다(계약 D: Begin/EndTurn 리셋).
		mEndTurnHandle = mCombat->GetModel<USRPGCombatModel>()->OnEndAnyTurnUI.AddUObject(this, &UCombatUIAdapter::HandleEndAnyTurn);
	}

	PushDiceUIs();
}

/** @brief UIModel 입력 델리게이트를 이 어댑터에 연결하고 현재 주사위 상태를 즉시 push한다. */
void UCombatUIAdapter::BindUIModel(UCombatUIModel* InUIModel)
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnCombatCommand.RemoveDynamic(this, &UCombatUIAdapter::HandleCombatCommand);
		mUIModel->OnCombatWorldTouch.RemoveDynamic(this, &UCombatUIAdapter::HandleWorldTouch);
	}

	mUIModel = InUIModel;

	if (mUIModel != nullptr)
	{
		mUIModel->OnCombatCommand.AddUniqueDynamic(this, &UCombatUIAdapter::HandleCombatCommand);
		mUIModel->OnCombatWorldTouch.AddUniqueDynamic(this, &UCombatUIAdapter::HandleWorldTouch);
	}

	PushDiceUIs();   // HUD가 열릴 때 주사위 뷰가 이미 있도록 초기 상태도 push.
}

/** @brief UI에서 올라온 의도를 받는다. 굴림/취소만 직접 처리하고 나머지는 액션이 가져갈 예정이다. */
void UCombatUIAdapter::HandleCombatCommand(ECombatInputType Type, int32 /*IntPayload*/)
{
	switch (Type)
	{
	case ECombatInputType::RollDice:
		// 굴림 요청: 컴포넌트(데이터)에서 굴리고 결과 뷰를 push한다.
		RollDice();
		break;

	case ECombatInputType::Cancel:
		ClearPendingAction();
		break;

	case ECombatInputType::SelectSkill:
	case ECombatInputType::ToggleDice:
	case ECombatInputType::Move:
		// TODO(액션 연동): IntPayload(스킬/주사위 index)를 USRPGSkillBuildAction의
		//                  SetSkill / ChangeDices / Move phase 커맨드로 라우팅한다.
		break;

	default:
		break;
	}
}

/** @brief 스크린 터치를 타일로 변환한다. 확정/취소는 액션이 처리할 예정이다. */
void UCombatUIAdapter::HandleWorldTouch(FVector2D ScreenPosition, bool /*bLongPress*/)
{
	FTileIndex Tile;
	if (ResolveTileFromScreen(ScreenPosition, Tile) == false)
	{
		// 타일맵 밖 탭 = 진행 중 하이라이트 취소.
		ClearPendingAction();
		return;
	}

	// TODO(액션 연동): 판정된 Tile을 USRPGSkillBuildAction의 WorldTrace/SetTargetTile 커맨드로 넘긴다.
}

/** @brief 턴 종료 UI 이벤트에서 이번 턴 사용한 주사위 잠금을 해제하고 Dice 도메인을 다시 push한다. */
void UCombatUIAdapter::HandleEndAnyTurn(TSharedPtr<FPresentationBarrier> /*Barrier*/, const USRPGTurnContext* /*TurnContext*/, ESRPGTurnResult /*Result*/)
{
	// 턴이 끝나면 이번 턴에 쓴 주사위 잠금을 해제한다(다음 턴 다시 사용 가능).
	// Barrier는 즉시 처리(애니 대기 없음)라 붙잡지 않는다 → 스코프 종료 시 카운트가 줄어 로직이 이어진다.
	if (mDicePool != nullptr)
	{
		mDicePool->ResetUsed();
		PushDiceUIs();
	}
}

/** @brief UObject 파괴 시 전투 서브시스템에 남은 턴 종료 구독을 해제한다. */
void UCombatUIAdapter::BeginDestroy()
{
	if (mCombat != nullptr && mEndTurnHandle.IsValid())
	{
		mCombat->GetModel<USRPGCombatModel>()->OnEndAnyTurnUI.Remove(mEndTurnHandle);
		mEndTurnHandle.Reset();
	}

	Super::BeginDestroy();
}

/** @brief 타일맵의 하이라이트 레이어를 단일 타일/단일 플래그 상태로 맞춘다. */
void UCombatUIAdapter::SetSingleTileHighlight(const FTileIndex& Tile, ETileHighlightFlag Flag) const
{
	UTileMapModel* TileMapModel = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap() : nullptr;
	ATileMap* TileMap = TileMapModel != nullptr ? TileMapModel->GetView<ATileMap>() : nullptr;
	if (TileMap == nullptr)
	{
		return;
	}
	// 세 상태 모두 끄고 원하는 한 가지만 켠다(단계 전환).
	TileMap->ClearTileHighlight(ETileHighlightFlag::Aim);
	TileMap->ClearTileHighlight(ETileHighlightFlag::Select);
	TileMap->ClearTileHighlight(ETileHighlightFlag::Effect);
	TArray<FTileIndex> Tiles;
	Tiles.Add(Tile);
	TileMap->SetTileHighlight(Tiles, Flag);
}

/** @brief Aim/Select/Effect 하이라이트를 모두 끈다. */
void UCombatUIAdapter::ClearAllHighlight() const
{
	UTileMapModel* TileMapModel = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap() : nullptr;
	ATileMap* TileMap = TileMapModel != nullptr ? TileMapModel->GetView<ATileMap>() : nullptr;
	if (TileMap == nullptr)
	{
		return;
	}
	TileMap->ClearTileHighlight(ETileHighlightFlag::Aim);
	TileMap->ClearTileHighlight(ETileHighlightFlag::Select);
	TileMap->ClearTileHighlight(ETileHighlightFlag::Effect);
}

/** @brief UIModel에 push된 주사위 결과를 액션 계산 값으로 읽어 UI 표시와 계산 입력을 일치시킨다. */
int32 UCombatUIAdapter::GetRolledDiceValue(int32 DiceIndex) const
{
	if (mUIModel == nullptr)
	{
		return 0;
	}
	const TArray<FDiceSlotUI>& Dice = mUIModel->GetDiceUIs();
	if (Dice.IsValidIndex(DiceIndex) == false)
	{
		return 0;
	}
	return Dice[DiceIndex].mIsRolled ? Dice[DiceIndex].mResultValue : 0;
}

/** @brief 현재 플레이어 DiceComponent를 굴리고 결과를 FDiceSlotUI로 다시 push한다. */
void UCombatUIAdapter::RollDice()
{
	if (mDicePool == nullptr)
	{
		return;
	}

	/*
	 * 어댑터 단독 단계라 임시 난수 스트림을 쓴다.
	 * 게임플레이/런 통합 시 run의 RandomStream(URandomStreamFunctionLibrary)을 주입해 결정론을 맞춘다.
	 */
	FRandomStream Stream;
	Stream.GenerateNewSeed();
	mDicePool->RollAll(Stream);
	PushDiceUIs();
}

/** @brief UDicePoolModel의 런타임 주사위 상태를 UI 전용 FDiceSlotUI 배열로 변환한다. */
void UCombatUIAdapter::PushDiceUIs() const
{
	if (mUIModel == nullptr || mDicePool == nullptr)
	{
		return;
	}

	const TArray<TObjectPtr<UDiceModel>>& Dice = mDicePool->GetDice();

	TArray<FDiceSlotUI> Views;
	Views.Reserve(Dice.Num());

	for (const TObjectPtr<UDiceModel>& DicePtr : Dice)
	{
		if (DicePtr == nullptr)
		{
			continue;
		}

		FDiceSlotUI View;
		View.mDiceId = DicePtr->GetSourceDiceId();
		View.mResultValue = DicePtr->GetCurrentValue();
		View.mRolledFaceIndex = DicePtr->GetRolledFaceIndex();
		View.mIsRolled = DicePtr->IsRolled();
		View.mIsUsed = DicePtr->IsUsed();
		View.mRarityColor = RDUIDice::GetDiceRarityColor(DicePtr->GetRarity());
		View.mRarityText = RDUIDice::GetDiceRarityText(DicePtr->GetRarity());
		View.mFaceCount = DicePtr->GetFaceCount();   // 종류 표시(d6/d20 등)용 면 수
		View.mFaceValues = DicePtr->GetFaceValues();
		View.mFaceTextures = DicePtr->GetFaceTextures();
		Views.Add(MoveTemp(View));
	}

	mUIModel->SetDiceUIs(Views);
}

/** @brief 진행 중 하이라이트를 끄고 UI 선택 강조 해제를 알린다. */
void UCombatUIAdapter::ClearPendingAction()
{
	ClearAllHighlight();

	// UI에 스킬/주사위 선택 강조를 풀라고 알린다(확정/취소 공통).
	if (mUIModel != nullptr)
	{
		mUIModel->NotifyActionResolved();
	}
}

/** @brief 카메라 스크린 좌표를 타일맵 평면과 교차시켜 유효한 FTileIndex로 변환한다. */
bool UCombatUIAdapter::ResolveTileFromScreen(const FVector2D& ScreenPosition, FTileIndex& OutTile) const
{
	UTileMapModel* TileMapModel = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap() : nullptr;
	ATileMap* TileMap = TileMapModel != nullptr ? TileMapModel->GetView<ATileMap>() : nullptr;
	UWorld* World = mCombat != nullptr ? mCombat->GetWorld() : nullptr;
	if (TileMap == nullptr || World == nullptr)
	{
		return false;
	}
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController == nullptr)
	{
		return false;
	}

	// 라인 트레이스(터치 전용 트레이스 채널 TileOnlyTrace, PR #72)로 타일 판정.
	// 손가락/커서 위치를 엔진이 직접 추적하므로 화면좌표 변환(평면 역투영)이 필요 없어
	// 고DPI 기기에서도 어긋나지 않는다(정석 — RDCollision.h가 명시한 GetHitResultUnderFinger 경로).
	// ScreenPosition은 계약상 받지만, 트레이스가 실제 입력 위치를 직접 쓰므로 여기선 사용하지 않는다.
	FHitResult HitResult;
	bool bHit = PlayerController->GetHitResultUnderFinger(ETouchIndex::Touch1, RDTraceChannels::TileOnlyTrace, false, HitResult);
	if (bHit == false)
	{
		// 폴백: 손가락 추적이 비어있으면(또는 마우스), 전달받은 화면좌표로 직접 트레이스한다.
		// GetHitResultAtScreenPosition은 뷰포트 픽셀을 기대하므로 Slate 절대좌표를 보정해 넘긴다.
		FVector2D ViewportPixel;
		FVector2D ViewportDPIScaled;
		USlateBlueprintLibrary::AbsoluteToViewport(World, ScreenPosition, ViewportPixel, ViewportDPIScaled);
		bHit = PlayerController->GetHitResultAtScreenPosition(ViewportPixel, RDTraceChannels::TileOnlyTrace, false, HitResult);
	}
	if (bHit == false)
	{
		return false;   // 타일맵 밖 또는 입력 없음.
	}

	const FTileIndex Tile = TileMap->WorldToTileIndex(HitResult.ImpactPoint);
	if (TileMap->IsValidIndex(Tile) == false)
	{
		return false;   // 타일맵 밖.
	}
	OutTile = Tile;
	return true;
}
