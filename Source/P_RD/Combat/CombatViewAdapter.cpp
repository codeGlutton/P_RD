#include "Combat/CombatViewAdapter.h"

#include "Pawn/Unit.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "UI/Combat/CombatViewModel.h"
#include "UI/Combat/CombatViewTypes.h"

void UCombatViewAdapter::Build(USRPGCombatSubsystem* InCombat, const URunPersistData* InRun)
{
	mCombat = InCombat;
	mPlayerLevel = InRun != nullptr ? InRun->GetPlayerLevel() : 1;
	PushAll();
}

void UCombatViewAdapter::BindViewModel(UCombatViewModel* InViewModel)
{
	mViewModel = InViewModel;
	PushAll();
}

void UCombatViewAdapter::PushAll()
{
	if (mViewModel == nullptr)
	{
		return;
	}

	// ── 유닛 뷰: 스폰된 AUnit들에서(비GAS). HP는 데이터계층 전이라 플레이스홀더. ──
	TArray<FUnitView> UnitViews;
	int32 PlayerUnitId = INDEX_NONE;
	if (mCombat != nullptr)
	{
		const TArray<TObjectPtr<AUnit>>& Units = mCombat->GetUnits();
		UnitViews.Reserve(Units.Num());
		for (int32 UnitIndex = 0; UnitIndex < Units.Num(); ++UnitIndex)
		{
			const AUnit* Unit = Units[UnitIndex];
			if (Unit == nullptr)
			{
				continue;
			}

			FUnitView View;
			View.mUnitId = UnitIndex;                 // 배열 index를 안정적 식별자로 사용(비GAS).
			View.mIsPlayer = Unit->IsPlayerUnit();
			// TODO(GAS 제거): 실제 HP/스탯은 UUnitData에서. 지금은 표시 검증용 플레이스홀더.
			View.mMaxHP = View.mIsPlayer ? 100.f : 30.f;
			View.mHP = View.mMaxHP;
			View.mTile = Unit->GetTileTransform().mIndex;

			if (View.mIsPlayer)
			{
				PlayerUnitId = UnitIndex;
			}
			UnitViews.Add(View);
		}
	}
	mViewModel->SetUnitViews(UnitViews);

	// ── 플레이어 메타: 레벨만 실데이터, 골드/경험치는 플레이스홀더(맞추면 됨). ──
	FPlayerMetaView Meta;
	Meta.mLevel = mPlayerLevel;
	Meta.mGold = 100;   // TODO(GAS 제거): 실제 골드 소스 연결.
	Meta.mExp = 0.f;
	Meta.mMaxExp = 0.f;
	mViewModel->SetPlayerMeta(Meta);

	// ── 턴: 라운드/현재유닛(현재는 플레이어 시작 가정). 실제 턴 시스템 연결은 후속. ──
	FTurnView Turn;
	Turn.mRound = 1;
	Turn.mCurrentUnitId = PlayerUnitId;
	mViewModel->SetTurnView(Turn);
}
