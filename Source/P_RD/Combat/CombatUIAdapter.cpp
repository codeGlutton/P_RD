#include "Combat/CombatUIAdapter.h"

#include "Actor/TileMap/TileMap.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RDCollision.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Dice/DicePoolModel.h"
#include "Dice/DiceModel.h"
#include "GameFramework/PlayerController.h"
#include "Pawn/Unit.h"
#include "ObjectView.h"
#include "Pawn/UnitModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "SRPGFramework/SRPGTurnContext.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/DiceViewData.h"

namespace
{
	// 스킬 레일 index (UI와 합의): BASIC=0, STEP=5.
	// [합의필요] 실제 스킬 데이터가 연결되면 레일 index 하드코딩 대신 SkillId/BuildAction 매핑으로 교체해야 한다.
	constexpr int32 SkillIndexBasic = 0;
	constexpr int32 SkillIndexStep = 5;

	// [합의필요] HP/Gold/이동력 진짜 소스 = UUnitData(GAS 폐기 후). 아래는 임시 placeholder —
	//           게임플레이가 UUnitData를 주면 이 상수 대신 거기서 읽어 SetUnitUIs/SetPlayerMeta를 채운다.
	//           (다이스는 이미 진짜 — APlayerUnit::UDicePoolModel에서 읽음. HP/Gold만 미연결.)
	constexpr float PlayerStartHP = 100.0f;
	constexpr float EnemyStartHP = 30.0f;

	// (임시) BASIC 평타 사거리. 범위 "모양"이 보이도록 보드 일부만 덮는 값. 실제 스킬 데이터(StaticSkillData 사거리) 연결 시 교체.
	constexpr int32 BasicAttackRange = 4;
	constexpr int32 PlayerStartMovePoint = 0;

	// 가상 적 시작 타일(9x9 보드, 플레이어 (0,0) 코너 기준 유효 좌표).
	// [합의필요] 적 스폰/룸 데이터 연결 전까지 HUD 타겟/HP바 검증용 fixture로만 사용한다.
	const FTileIndex VirtualEnemyTiles[] = {
		FTileIndex(4, 4),
		FTileIndex(6, 6),
		FTileIndex(2, 5),
	};
}

/** @brief 전투 서브시스템과 런 데이터를 기준으로 임시 유닛 상태를 재구성하고 첫 View push를 수행한다. */
void UCombatUIAdapter::Build(USRPGCombatSubsystem* InCombat, const URunPersistData* InRun)
{
	// 재빌드 대비: 이전 subsystem 턴 종료 구독을 먼저 해제한다.
	// 전환/teardown 시 모델이 이미 파괴돼 GetModel()이 null일 수 있으므로 모델까지 null 가드한다.
	if (mCombat != nullptr && mEndTurnHandle.IsValid())
	{
		if (USRPGCombatModel* PrevModel = mCombat->GetModel<USRPGCombatModel>())
		{
			PrevModel->OnEndAnyTurnUI.Remove(mEndTurnHandle);
		}
		mEndTurnHandle.Reset();
	}

	mCombat = InCombat;
	mPlayerLevel = InRun != nullptr ? InRun->GetPlayerLevel() : 1;
	mUnitStates.Reset();
	mNextUnitId = 0;

	// 플레이어: 실제 스폰된 유닛에서 타일/액터를 가져온다.
	USRPGCombatModel* CombatModel = (mCombat != nullptr) ? mCombat->GetModel<USRPGCombatModel>() : nullptr;
	if (CombatModel != nullptr)
	{
		// 턴이 끝날 때마다 이번 턴에 쓴 주사위 잠금을 해제하도록 훅을 건다(계약 D: Begin/EndTurn 리셋).
		mEndTurnHandle = CombatModel->OnEndAnyTurnUI.AddUObject(this, &UCombatUIAdapter::HandleEndAnyTurn);

		for (const TObjectPtr<UUnitModel>& Unit : CombatModel->GetUnits())
		{
			if (Unit == nullptr || Unit->IsPlayerUnitModel() == false)
			{
				continue;
			}
			FCombatUnitState State;
			State.mId = mNextUnitId++;
			State.mIsPlayer = true;
			State.mTile = Unit->GetTileTransform().mIndex;
			// 타일 트랜스폼이 신뢰 안 될 수 있어((0,0) 등) 액터 실제 위치 기준으로 보정.
			if (UTileMapModel* TileMap = mCombat->GetModel<USRPGCombatModel>()->GetTileMap())
			{
				const FTileIndex ActorTile = TileMap->WorldToTileIndex(Unit->GetView<AActor>()->GetActorLocation());
				if (TileMap->IsValidIndex(ActorTile))
				{
					State.mTile = ActorTile;
				}
			}
			// 진짜 속성값(TAS AttributeSet, 커브테이블 초기화)에서 HP를 읽는다.
			// 단 속성 초기화가 안 된 경우(MaxHP=0/비정상)는 placeholder로 폴백.
			float RealMaxHP = 0.f;
			float RealHP = 0.f;
			if (UAttributeSetComponentModel* AttrComp = Unit->GetAttributeComponentModel())
			{
				bool bFound = false;
				const float MaxHPValue = AttrComp->GetAttributeCurrentValue(UUnitAttributeSet::GetMaxHPAttribute(), bFound);
				if (bFound)
				{
					RealMaxHP = MaxHPValue;
					RealHP = AttrComp->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute(), bFound);
				}
			}
			if (RealMaxHP > 0.f && RealMaxHP < 100000.f)
			{
				State.mMaxHP = RealMaxHP;
				State.mHP = (RealHP > 0.f && RealHP < 100000.f) ? RealHP : RealMaxHP;
			}
			else
			{
				State.mMaxHP = PlayerStartHP;
				State.mHP = PlayerStartHP;
			}
			State.mMovePoint = PlayerStartMovePoint;
			State.mActor = Unit->GetView<AUnit>();
			mUnitStates.Add(State);
		}
	}

	// 적: 액터 스폰 대신 가상 유닛으로 타일 위에 둔다(스폰 크래시 회피).
	for (const FTileIndex& Tile : VirtualEnemyTiles)
	{
		FCombatUnitState State;
		State.mId = mNextUnitId++;
		State.mIsPlayer = false;
		State.mTile = Tile;
		State.mMaxHP = EnemyStartHP;
		State.mHP = EnemyStartHP;
		mUnitStates.Add(State);
	}

	PushAll();
}

/** @brief UIModel 입력 델리게이트를 이 어댑터에 연결하고 현재 상태를 즉시 push한다. */
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

	PushAll();
	PushDiceUIs();   // HUD가 열릴 때 주사위 뷰가 이미 있도록 초기 상태도 push.
}

/** @brief UI에서 올라온 index 기반 의도를 임시 BASIC/STEP/MOVE 상태 머신으로 해석한다. */
void UCombatUIAdapter::HandleCombatCommand(ECombatInputType Type, int32 IntPayload)
{
	switch (Type)
	{
	case ECombatInputType::SelectSkill:
		mSelectedSkillIndex = IntPayload;
		mPendingAttackDamage = -1;
		mMovePending = false;
		break;

	case ECombatInputType::ToggleDice:
	{
		// 이미 쓴 주사위는 무시(턴 종료/다음 턴 시작까지 잠금).
		if (mUIModel != nullptr)
		{
			const TArray<FDiceSlotUI>& Dice = mUIModel->GetDiceUIs();
			if (Dice.IsValidIndex(IntPayload) && Dice[IntPayload].mIsUsed)
			{
				break;
			}
		}

		const int32 DiceValue = GetRolledDiceValue(IntPayload);
		if (DiceValue <= 0)
		{
			break;
		}
		mPendingDiceIndex = IntPayload;   // 확정 시 '사용됨' 처리할 주사위.
		if (mSelectedSkillIndex == SkillIndexStep)
		{
			// STEP: 주사위 배치 → 본인 타일을 회색(Aim)으로. 탭마다 노랑→빨강(확정).
			mPendingStepValue = DiceValue;
			mStepStage = 0;
			mStepTile = GetPlayerTile();
			SetSingleTileHighlight(mStepTile, ETileHighlightFlag::Aim);
		}
		else if (mSelectedSkillIndex == SkillIndexBasic)
		{
			// BASIC: 주사위 배치 → 사거리(회색 Aim)를 깐다. 그 안에서 타깃 탭=노랑, 재탭=빨강+실행.
			mPendingAttackDamage = DiceValue;
			mAttackTargetTile = FTileIndex::Invalid;
			if (UTileMapModel* TileMap = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap() : nullptr)
			{
				// 사거리 기준점 = 플레이어 타일. 타일 트랜스폼이 신뢰 안 될 수 있어((0,0) 등) 액터 실제 위치로 잡는다.
				FTileIndex Origin = GetPlayerTile();
				if (const FCombatUnitState* PlayerState = FindPlayerState())
				{
					if (PlayerState->mActor.IsValid())
					{
						Origin = TileMap->WorldToTileIndex(PlayerState->mActor->GetActorLocation());
					}
				}
				// 사거리 = Square 범위(타일맵 GetAimableTiles, pyramidmine 구현). 점유 타일 포함(적 조준).
				mAimTiles = TileMap->GetAimableTiles(Origin, BasicAttackRange, EAimPattern::Square, true, false);
				ClearAllHighlight();
				TileMap->GetView<ATileMap>()->SetTileHighlight(mAimTiles, ETileHighlightFlag::Aim);   // 회색 사거리
			}
		}
		break;
	}

	case ECombatInputType::RollDice:
		// 굴림 요청: 컴포넌트(데이터)에서 굴리고 결과 뷰를 push한다.
		RollDice();
		break;

	case ECombatInputType::Move:
	{
		// MOVE 모드: 도달 가능 범위(BFS 경로 기반, 이동력 기준)를 회색으로 깔고 그 안만 이동.
		mMovePending = true;
		mSelectedSkillIndex = INDEX_NONE;
		mPendingAttackDamage = -1;
		mAttackTargetTile = FTileIndex::Invalid;
		if (UTileMapModel* TileMap = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap() : nullptr)
		{
			mAimTiles = TileMap->GetReachableTiles(GetPlayerTile(), GetPlayerMovePoint());
			ClearAllHighlight();
			TileMap->GetView<ATileMap>()->SetTileHighlight(mAimTiles, ETileHighlightFlag::Aim);
		}
		break;
	}

	case ECombatInputType::Cancel:
		ClearPendingAction();
		break;

	default:
		break;
	}
}

/** @brief 스크린 터치를 타일로 변환한 뒤 현재 대기 중인 액션의 확정/취소로 소비한다. */
void UCombatUIAdapter::HandleWorldTouch(FVector2D ScreenPosition, bool bLongPress)
{
	// 롱프레스 여부는 후속 상세 패널 계약을 위해 받지만, 이 임시 어댑터는 일반 탭 액션만 처리한다.
	FTileIndex Tile;
	if (ResolveTileFromScreen(ScreenPosition, Tile) == false)
	{
		// 타일맵 밖 탭 = 진행 중 스킬/이동 취소.
		ClearPendingAction();
		return;
	}

	// STEP 단계 확정: 본인 타일을 탭할 때마다 회색→노랑→빨강(확정).
	if (mPendingStepValue >= 0)
	{
		if (Tile.mX == mStepTile.mX && Tile.mY == mStepTile.mY)
		{
			++mStepStage;
			if (mStepStage == 1)
			{
				SetSingleTileHighlight(mStepTile, ETileHighlightFlag::Select);   // 노랑
			}
			else if (mStepStage >= 2)
			{
				SetSingleTileHighlight(mStepTile, ETileHighlightFlag::Effect);   // 빨강 = 확정
				ApplyStep(mPendingStepValue);                                    // 이동력 += 주사위값
				if (mDicePool != nullptr && mPendingDiceIndex != INDEX_NONE)
				{
					mDicePool->MarkDiceUsed(mPendingDiceIndex);             // 쓴 주사위 잠금
					PushDiceUIs();                                             // 잠금 상태를 UI에 반영
				}
				ClearPendingAction();
			}
		}
		else
		{
			// 다른 곳 탭 = 취소.
			ClearPendingAction();
		}
		return;
	}

	// BASIC 평타: 사거리(회색) 안에서 타깃 탭=노랑, 같은 칸 재탭=빨강+실행.
	if (mPendingAttackDamage >= 0)
	{
		UTileMapModel* TileMap = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap() : nullptr;

		// 사거리 밖 탭 = 취소.
		if (mAimTiles.Contains(Tile) == false)
		{
			ClearPendingAction();
			return;
		}

		const bool bSameTile = (mAttackTargetTile.mX == Tile.mX && mAttackTargetTile.mY == Tile.mY);
		if (bSameTile == false)
		{
			// 1차 선택(또는 다른 칸 재선택): 회색 사거리는 유지하고 선택칸만 노랑(Select 최우선).
			mAttackTargetTile = Tile;
			if (TileMap != nullptr)
			{
				ATileMap* TileMapView = TileMap->GetView<ATileMap>();

				TileMapView->ClearTileHighlight(ETileHighlightFlag::Select);
				TileMapView->ClearTileHighlight(ETileHighlightFlag::Effect);
				TileMapView->SetTileHighlight(mAimTiles, ETileHighlightFlag::Aim);
				TArray<FTileIndex> Picked;
				Picked.Add(Tile);
				TileMapView->SetTileHighlight(Picked, ETileHighlightFlag::Select);   // 노랑
			}
			return;
		}

		// 2차(같은 칸 재탭): 빨강 + 실행.
		if (TileMap != nullptr)
		{
			ATileMap* TileMapView = TileMap->GetView<ATileMap>();

			TileMapView->ClearTileHighlight(ETileHighlightFlag::Select);
			TArray<FTileIndex> Picked;
			Picked.Add(Tile);
			TileMapView->SetTileHighlight(Picked, ETileHighlightFlag::Effect);   // 빨강
		}
		const int32 TargetId = FindUnitIdAtTile(Tile);
		FCombatUnitState* Target = FindStateById(TargetId);
		if (Target != nullptr && Target->mIsPlayer == false)
		{
			ApplyBasicAttack(TargetId, mPendingAttackDamage);
			if (mDicePool != nullptr && mPendingDiceIndex != INDEX_NONE)
			{
				mDicePool->MarkDiceUsed(mPendingDiceIndex);   // 적을 친 경우에만 주사위 소모.
				PushDiceUIs();                                   // 잠금 상태를 UI에 반영
			}
		}
		ClearPendingAction();
		return;
	}

	// MOVE: 도달 범위 안에서 칸 탭=노랑(선택), 같은 칸 재탭=빨강(확정)+이동. (BASIC과 동일 흐름)
	if (mMovePending)
	{
		// 도달 범위 밖 탭 = 취소.
		if (mAimTiles.Contains(Tile) == false)
		{
			ClearPendingAction();
			return;
		}

		UTileMapModel* TileMap = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap() : nullptr;
		const bool bSameTile = (mAttackTargetTile.mX == Tile.mX && mAttackTargetTile.mY == Tile.mY);
		if (bSameTile == false)
		{
			// 1차 선택: 회색 범위 유지 + 선택칸만 노랑.
			mAttackTargetTile = Tile;
			if (TileMap != nullptr)
			{
				ATileMap* TileMapView = TileMap->GetView<ATileMap>();

				TileMapView->ClearTileHighlight(ETileHighlightFlag::Select);
				TileMapView->ClearTileHighlight(ETileHighlightFlag::Effect);
				TileMapView->SetTileHighlight(mAimTiles, ETileHighlightFlag::Aim);
				TArray<FTileIndex> Picked;
				Picked.Add(Tile);
				TileMapView->SetTileHighlight(Picked, ETileHighlightFlag::Select);   // 노랑
			}
			return;
		}

		// 2차(같은 칸 재탭): 빨강 + 이동.
		if (TileMap != nullptr)
		{
			ATileMap* TileMapView = TileMap->GetView<ATileMap>();

			TileMapView->ClearTileHighlight(ETileHighlightFlag::Select);
			TArray<FTileIndex> Picked;
			Picked.Add(Tile);
			TileMapView->SetTileHighlight(Picked, ETileHighlightFlag::Effect);   // 빨강
		}
		if (TryMovePlayer(Tile))
		{
			// 이동 후 남은 이동력으로 도달범위 재계산(다시 선택 가능), 없으면 종료.
			if (GetPlayerMovePoint() > 0 && TileMap != nullptr)
			{
				ATileMap* TileMapView = TileMap->GetView<ATileMap>();

				mAttackTargetTile = FTileIndex::Invalid;
				mAimTiles = TileMap->GetReachableTiles(GetPlayerTile(), GetPlayerMovePoint());
				ClearAllHighlight();
				TileMapView->SetTileHighlight(mAimTiles, ETileHighlightFlag::Aim);
			}
			else
			{
				ClearPendingAction();
			}
		}
		else
		{
			ClearPendingAction();
		}
		return;
	}
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
	// 전환/teardown 시 모델이 이미 파괴돼 GetModel()이 null일 수 있으므로 모델까지 null 가드한다.
	if (mCombat != nullptr && mEndTurnHandle.IsValid())
	{
		if (USRPGCombatModel* CombatModel = mCombat->GetModel<USRPGCombatModel>())
		{
			CombatModel->OnEndAnyTurnUI.Remove(mEndTurnHandle);
		}
		mEndTurnHandle.Reset();
	}

	Super::BeginDestroy();
}

/** @brief 타일맵의 하이라이트 레이어를 단일 타일/단일 플래그 상태로 맞춘다. */
void UCombatUIAdapter::SetSingleTileHighlight(const FTileIndex& Tile, ETileHighlightFlag Flag) const
{
	ATileMap* TileMap = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap()->GetView<ATileMap>() : nullptr;
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

/** @brief Aim/Select/Effect 하이라이트를 모두 끄고 pending 액션의 시각 상태를 초기화한다. */
void UCombatUIAdapter::ClearAllHighlight() const
{
	ATileMap* TileMap = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap()->GetView<ATileMap>() : nullptr;
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

/** @brief 스킬/주사위/이동 pending 상태와 하이라이트를 모두 초기화하고 UI 선택 강조 해제를 알린다. */
void UCombatUIAdapter::ClearPendingAction()
{
	mSelectedSkillIndex = INDEX_NONE;
	mPendingAttackDamage = -1;
	mMovePending = false;
	mPendingStepValue = -1;
	mStepStage = 0;
	mPendingDiceIndex = INDEX_NONE;
	mAimTiles.Reset();
	mAttackTargetTile = FTileIndex::Invalid;
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
	ATileMap* TileMap = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap()->GetView<ATileMap>() : nullptr;
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

/** @brief Actor가 없는 가상 유닛의 화면 표시 위치를 타일맵 좌표에서 얻는다. */
FVector UCombatUIAdapter::TileToWorld(const FTileIndex& Tile) const
{
	const ATileMap* TileMap = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap()->GetView<ATileMap>() : nullptr;
	return TileMap != nullptr ? TileMap->TileToWorldLocation(Tile) : FVector::ZeroVector;
}

/** @brief 현재 임시 전투 상태를 Unit/Meta/Turn/Equipment 도메인으로 변환해 UIModel에 push한다. */
void UCombatUIAdapter::PushAll()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	TArray<FUnitUI> UnitUIs;
	UnitUIs.Reserve(mUnitStates.Num());
	int32 PlayerUnitId = INDEX_NONE;
	for (const FCombatUnitState& State : mUnitStates)
	{
		FUnitUI View;
		View.mUnitId = State.mId;
		View.mIsPlayer = State.mIsPlayer;
		View.mHP = State.mHP;
		View.mMaxHP = State.mMaxHP;
		View.mMovementPoint = State.mMovePoint;
		View.mMaxMovementPoint = State.mMaxMovePoint;
		View.mTile = State.mTile;
		// 실제 액터가 있으면 그 위치, 가상이면 타일→월드 변환.
		View.mWorldLocation = State.mActor.IsValid()
			? State.mActor->GetActorLocation()
			: TileToWorld(State.mTile);

		if (State.mIsPlayer)
		{
			PlayerUnitId = State.mId;
		}
		UnitUIs.Add(View);
	}
	mUIModel->SetUnitUIs(UnitUIs);

	FPlayerMetaUI Meta;
	Meta.mLevel = mPlayerLevel;
	Meta.mGold = mPlayerGold;   // 폴백
	// 진짜 골드(TAS UPlayerUnitAttributeSet.Money). 전투 모델에서 플레이어 유닛 → AttributeComponent로 읽는다.
	if (USRPGCombatModel* CombatModel = (mCombat != nullptr) ? mCombat->GetModel<USRPGCombatModel>() : nullptr)
	{
		for (const TObjectPtr<UUnitModel>& Unit : CombatModel->GetUnits())
		{
			if (Unit == nullptr || Unit->IsPlayerUnitModel() == false)
			{
				continue;
			}
			if (UAttributeSetComponentModel* AttrComp = Unit->GetAttributeComponentModel())
			{
				bool bFound = false;
				const float Money = AttrComp->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMoneyAttribute(), bFound);
				if (bFound)
				{
					Meta.mGold = FMath::RoundToInt(Money);
				}
			}
			break;
		}
	}
	mUIModel->SetPlayerMeta(Meta);

	FTurnUI Turn;
	Turn.mRound = 1;
	Turn.mCurrentUnitId = PlayerUnitId;
	mUIModel->SetTurnUI(Turn);

	// TEMP(시각 검증용): 게임플레이가 아직 장비를 채우지 않아, 탑바 좌측 하단 장비 줄의 위치/모양 확인을 위해
	// 임시 3슬롯을 넣는다. 게임플레이가 실제 장비를 SetEquipmentUIs로 채우면 이 블록을 제거할 것.
	TArray<FEquipmentUI> Equipment;
	for (int32 SlotIndex = 0; SlotIndex < 3; ++SlotIndex)
	{
		FEquipmentUI Equip;
		Equip.mSlotIndex = SlotIndex;
		Equip.mName = FText::Format(NSLOCTEXT("CombatUIAdapter", "TempEquipSlot", "EQ{0}"), FText::AsNumber(SlotIndex + 1));
		Equip.mIsEquipped = (SlotIndex == 0);
		Equipment.Add(Equip);
	}
	mUIModel->SetEquipmentUIs(Equipment);

	// (시각 검증용 임시) 유닛 위치에 도형 마커를 깐다 — 플레이어/적이 어디 있는지 보이게.
	RefreshUnitMarkers();
}

/** @brief (시각 검증용 임시) 각 유닛 위치에 기본 도형(플레이어=원기둥/파랑, 적=정육면체/빨강)을 스폰한다.
    콜리전을 끈다 — 타일 선택 라인 트레이스가 도형을 통과해 타일에 닿게 하기 위함. */
void UCombatUIAdapter::RefreshUnitMarkers()
{
	UWorld* World = mCombat != nullptr ? mCombat->GetWorld() : nullptr;
	if (World == nullptr)
	{
		return;
	}

	// 이전 마커 제거(이동/사망 반영을 위해 매 push마다 재생성).
	for (const TObjectPtr<AActor>& Marker : mUnitMarkers)
	{
		if (Marker != nullptr)
		{
			Marker->Destroy();
		}
	}
	mUnitMarkers.Reset();

	UStaticMesh* PlayerMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* EnemyMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* ShapeMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	for (const FCombatUnitState& State : mUnitStates)
	{
		// 마커는 유닛의 "논리 타일(mTile)" 기준으로 둔다. 그래야 이동 시 액터 mobility와 무관하게 항상 따라온다.
		FVector Loc = TileToWorld(State.mTile);
		Loc.Z += 50.0f;   // 타일 위로 살짝 띄움

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Marker = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Loc, FRotator::ZeroRotator, Params);
		if (Marker == nullptr)
		{
			continue;
		}

		UStaticMeshComponent* MeshComp = Marker->GetStaticMeshComponent();
		MeshComp->SetMobility(EComponentMobility::Movable);
		MeshComp->SetStaticMesh(State.mIsPlayer ? PlayerMesh : EnemyMesh);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // 타일 트레이스 통과
		Marker->SetActorScale3D(State.mIsPlayer ? FVector(0.45f, 0.45f, 0.9f) : FVector(0.5f));

		if (ShapeMat != nullptr)
		{
			UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(ShapeMat, Marker);
			if (DynMat != nullptr)
			{
				DynMat->SetVectorParameterValue(TEXT("Color"), State.mIsPlayer ? FLinearColor(0.2f, 0.45f, 1.0f) : FLinearColor(1.0f, 0.2f, 0.2f));
				MeshComp->SetMaterial(0, DynMat);
			}
		}

		mUnitMarkers.Add(Marker);
	}
}

/** @brief 임시 상태 테이블에서 플레이어 행을 찾는다. */
const FCombatUnitState* UCombatUIAdapter::FindPlayerState() const
{
	return mUnitStates.FindByPredicate([](const FCombatUnitState& State) { return State.mIsPlayer; });
}

/** @brief UnitId payload를 임시 상태 테이블의 mutable 행으로 되돌린다. */
FCombatUnitState* UCombatUIAdapter::FindStateById(int32 UnitId)
{
	return mUnitStates.FindByPredicate([UnitId](const FCombatUnitState& State) { return State.mId == UnitId; });
}

/** @brief BASIC 임시 평타를 가상 적 HP에 적용하고 사망 시 상태 테이블에서 제거한다. */
void UCombatUIAdapter::ApplyBasicAttack(int32 TargetUnitId, int32 Amount)
{
	FCombatUnitState* Target = FindStateById(TargetUnitId);
	if (Target == nullptr || Target->mIsPlayer)
	{
		return;
	}

	Target->mHP = FMath::Max(0.0f, Target->mHP - static_cast<float>(Amount));
	if (Target->mHP <= 0.0f)
	{
		// 가상 적 사망: 상태에서 제거.
		const int32 RemoveId = Target->mId;
		mUnitStates.RemoveAll([RemoveId](const FCombatUnitState& State) { return State.mId == RemoveId; });
	}
	PushAll();
}

/** @brief STEP 임시 액션으로 플레이어 이동력 현재/최대값을 주사위 값만큼 채운다. */
void UCombatUIAdapter::ApplyStep(int32 Amount)
{
	FCombatUnitState* Player = mUnitStates.FindByPredicate([](const FCombatUnitState& State) { return State.mIsPlayer; });
	if (Player == nullptr)
	{
		return;
	}
	Player->mMovePoint += Amount;
	Player->mMaxMovePoint = Player->mMovePoint;   // STEP 직후 현재=최대(6/6).
	PushAll();
}

/** @brief MOVE 임시 액션으로 빈 타일 이동을 적용하고 실제 플레이어 액터 위치도 타일맵에 맞춘다. */
bool UCombatUIAdapter::TryMovePlayer(const FTileIndex& TargetTile)
{
	FCombatUnitState* Player = mUnitStates.FindByPredicate([](const FCombatUnitState& State) { return State.mIsPlayer; });
	if (Player == nullptr || Player->mMovePoint <= 0)
	{
		return false;
	}

	// 가상 적이 있는 타일로는 이동 불가.
	const int32 OccupantId = FindUnitIdAtTile(TargetTile);
	if (OccupantId != INDEX_NONE)
	{
		return false;
	}

	// 이동 비용 = 현재 칸→목표 칸 거리(Chebyshev 근사). 이동력이 모자라면 불가.
	const int32 Cost = FMath::Max(FMath::Abs(TargetTile.mX - Player->mTile.mX), FMath::Abs(TargetTile.mY - Player->mTile.mY));
	if (Cost <= 0 || Cost > Player->mMovePoint)
	{
		return false;
	}

	Player->mTile = TargetTile;
	Player->mMovePoint -= Cost;

	// 실제 플레이어 액터도 해당 타일 위로 옮긴다(StartActorMovement는 현재 스텁이라 위치를 직접 설정).
	if (Player->mActor.IsValid())
	{
		if (UTileMapModel* TileMap = mCombat != nullptr ? mCombat->GetModel<USRPGCombatModel>()->GetTileMap() : nullptr)
		{
			FVector NewLoc = TileMap->TileToWorldLocation(TargetTile);
			NewLoc.Z = Player->mActor->GetActorLocation().Z;   // 높이 유지
			// 폰 루트가 Static mobility면 SetActorLocation이 실패(moved=0)한다. 이동 전 Movable로 바꾼다.
			if (USceneComponent* Root = Player->mActor->GetRootComponent())
			{
				Root->SetMobility(EComponentMobility::Movable);
			}
			Player->mActor->SetActorLocation(NewLoc, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	PushAll();
	return true;
}

/** @brief 타일 점유 상태를 UnitId로 돌려준다; 빈 타일은 INDEX_NONE이다. */
int32 UCombatUIAdapter::FindUnitIdAtTile(const FTileIndex& Tile) const
{
	const FCombatUnitState* Found = mUnitStates.FindByPredicate([&Tile](const FCombatUnitState& State)
	{
		return State.mTile.mX == Tile.mX && State.mTile.mY == Tile.mY;
	});
	return Found != nullptr ? Found->mId : INDEX_NONE;
}

/** @brief 현재 플레이어 타일을 반환하고, 플레이어 상태가 없으면 Invalid를 돌려준다. */
FTileIndex UCombatUIAdapter::GetPlayerTile() const
{
	const FCombatUnitState* Player = FindPlayerState();
	return Player != nullptr ? Player->mTile : FTileIndex::Invalid;
}

/** @brief MOVE 버튼/이동 유지 여부 판단에 쓰는 플레이어 현재 이동력. */
int32 UCombatUIAdapter::GetPlayerMovePoint() const
{
	const FCombatUnitState* Player = FindPlayerState();
	return Player != nullptr ? Player->mMovePoint : 0;
}
