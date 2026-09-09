#include "UI/Reward/LevelUpSkillRewardFlow.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/Reward/RewardConcept03Widget.h"
#include "UI/RunOptionsRailWidget.h"
#include "Actor/Party/PartyModel.h"
#include "AttributeSet/LevelAttributeSet.h"
#include "Component/SkillComponent/UnitSkillComponentModel.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "DataAsset/GameplayAssetPolicy.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "UI/Combat/SkillDetailUIBuilder.h"
#include "UI/Shop/ShopUIModel.h"
#include "UI/Shop/ShopUIWidgetBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UStaticUnitSkillData* LoadSkill(const FPrimaryAssetId& Id)
	{
		UAssetManager* Manager = UAssetManager::GetIfInitialized();
		return Manager ? Cast<UStaticUnitSkillData>(Manager->GetPrimaryAssetPath(Id).TryLoad()) : nullptr;
	}

	bool IsOwned(const USkillComponentModel* Skills, const FPrimaryAssetId& Id)
	{
		return Skills && Skills->GetSkills().ContainsByPredicate([&Id](const FSkillEntry& Entry)
			{ return Entry.mData && Entry.mData->GetPrimaryAssetId() == Id; });
	}
}

TArray<FPrimaryAssetId> LevelUpSkillReward::ChooseCandidates(const TArray<UStaticUnitSkillData*>& Pool,
	const FRarityRate& Rate, const FRandomStream& Stream)
{
	TArray<UStaticUnitSkillData*> Remaining;
	for (UStaticUnitSkillData* Skill : Pool)
		if (Skill && GameplayAssetPolicy::IsPlayerFacing(Skill->GetPrimaryAssetId())) Remaining.AddUnique(Skill);
	Remaining.Sort([](const UStaticUnitSkillData& A, const UStaticUnitSkillData& B)
		{ return A.GetPrimaryAssetId().ToString() < B.GetPrimaryAssetId().ToString(); });
	TArray<FPrimaryAssetId> Result;
	while (!Remaining.IsEmpty() && Result.Num() < 3)
	{
		TArray<ERarityType> Rarities;
		for (const UStaticUnitSkillData* Skill : Remaining) Rarities.AddUnique(Skill->mRarityType);
		float TotalWeight = 0.f;
		for (ERarityType Rarity : Rarities) TotalWeight += FMath::Max(0.f, Rate.mWeights[static_cast<uint8>(Rarity)]);
		ERarityType Rarity = Rarities.Last();
		if (TotalWeight > 0.f)
		{
			float Roll = Stream.FRand() * TotalWeight;
			for (ERarityType Candidate : Rarities)
			{
				Roll -= FMath::Max(0.f, Rate.mWeights[static_cast<uint8>(Candidate)]);
				if (Roll < 0.f) { Rarity = Candidate; break; }
			}
		}
		else Rarity = Rarities[Stream.RandRange(0, Rarities.Num() - 1)];
		TArray<int32> Matching;
		for (int32 Index = 0; Index < Remaining.Num(); ++Index)
			if (Remaining[Index]->mRarityType == Rarity) Matching.Add(Index);
		const int32 Pick = Matching[Stream.RandRange(0, Matching.Num() - 1)];
		Result.Add(Remaining[Pick]->GetPrimaryAssetId());
		Remaining.RemoveAt(Pick);
	}
	return Result;
}

int32 LevelUpSkillReward::FindPendingReward(const TArray<FLevelUpSkillReward>& Rewards, int32 UnitIndex)
{
	return Rewards.IndexOfByPredicate([UnitIndex](const FLevelUpSkillReward& Reward)
		{ return !Reward.Completed && (UnitIndex == INDEX_NONE || Reward.UnitIndex == UnitIndex); });
}

bool LevelUpSkillReward::Skip(FLevelUpSkillReward& Reward)
{
	if (Reward.Completed || !Reward.Offered) return false;
	Reward.Completed = true;
	return true;
}

bool LevelUpSkillReward::TryEquip(FLevelUpSkillReward& Reward, UPlayerUnitModel* Unit,
	const FPrimaryAssetId& SkillId, int32 SkillSlot)
{
	if (Reward.Completed || !Reward.Offered || !GameplayAssetPolicy::IsPlayerFacing(SkillId) || !Reward.Candidates.Contains(SkillId)
		|| !Unit || SkillSlot < 1 || SkillSlot > 4) return false;
	USkillComponentModel* Skills = Unit->GetSkillComponentModel();
	UStaticUnitSkillData* Skill = LoadSkill(SkillId);
	if (!Skills || !Skill || !Skills->GetSkills().IsValidIndex(SkillSlot)
		|| IsOwned(Skills, SkillId) || !Skills->SetSkill(SkillSlot, Skill)) return false;
	Reward.Completed = true;
	return true;
}

ULevelUpSkillRewardFlow::ULevelUpSkillRewardFlow()
{
	static ConstructorHelpers::FClassFinder<UShopUIWidgetBase> ShopClass(TEXT("/Game/UI/Shop/WBP_Shop_FullGenerated"));
	if (ShopClass.Succeeded()) mWidgetClass = ShopClass.Class;
}

void ULevelUpSkillRewardFlow::Open(URunPersistData* Run, UPartyModel* Party, APlayerController* Controller)
{
	if (!Run || !Party || !Controller) return;
	mRun = Run;
	mParty = Party;
	mController = Controller;
	if (!mUIModel)
	{
		mUIModel = NewObject<UShopUIModel>(this);
		mUIModel->OnRewardUnitRequested.AddDynamic(this, &ULevelUpSkillRewardFlow::SelectRewardUnit);
		mUIModel->OnBuySkillRequested.AddDynamic(this, &ULevelUpSkillRewardFlow::Select);
		mUIModel->OnLeaveRequested.AddDynamic(this, &ULevelUpSkillRewardFlow::ContinueWithoutCandidate);
		mUIModel->OnItemDetailRequested.AddDynamic(this, &ULevelUpSkillRewardFlow::ShowDetail);
		mUIModel->OnOwnedSkillDetailRequested.AddDynamic(this, &ULevelUpSkillRewardFlow::ShowOwnedDetail);
	}
	ShowNext();
}

void ULevelUpSkillRewardFlow::Save()
{
	if (mController && mController->GetGameInstance())
		mController->GetGameInstance()->GetSubsystem<USaveGameSubsystem>()->RequestRunAutosave();
}

void ULevelUpSkillRewardFlow::Close()
{
	if (mWidget) mWidget->CloseUI();
	for (const auto& Entry : mSuspendedRewardWidgets)
		if (IsValid(Entry.Key) && Entry.Key->IsInViewport()) Entry.Key->SetVisibility(Entry.Value);
	mSuspendedRewardWidgets.Reset();
	mActiveReward = INDEX_NONE;
}

void ULevelUpSkillRewardFlow::SuspendRewardScreens()
{
	// The selection is transparent all the way to the battlefield. Keep the reward
	// flow alive underneath so its current step can resume after choosing/skipping.
	for (UClass* Class : { URewardConcept03Widget::StaticClass(), URunOptionsRailWidget::StaticClass() })
	{
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(mController, Widgets, Class, true);
		for (UUserWidget* Widget : Widgets)
			if (Widget->IsVisible() && !mSuspendedRewardWidgets.Contains(Widget))
			{
				mSuspendedRewardWidgets.Add(Widget, Widget->GetVisibility());
				Widget->SetVisibility(ESlateVisibility::Collapsed);
			}
	}
}

void ULevelUpSkillRewardFlow::ShowNext(int32 PreferredUnit)
{
	TArray<FLevelUpSkillReward>& Rewards = mRun->GetRoomTransactionsMutable().LevelUpSkills;
	mActiveReward = LevelUpSkillReward::FindPendingReward(Rewards, PreferredUnit);
	if (mActiveReward == INDEX_NONE) mActiveReward = LevelUpSkillReward::FindPendingReward(Rewards);
	if (mActiveReward == INDEX_NONE) { Close(); return; }
	FLevelUpSkillReward& Reward = Rewards[mActiveReward];
	const auto& Units = mParty->GetPlayerUnitModels();
	UPlayerUnitModel* Unit = Units.IsValidIndex(Reward.UnitIndex) ? Units[Reward.UnitIndex].Get() : nullptr;
	if (!Unit) { UE_LOG(LogTemp, Error, TEXT("Level-up reward target is missing")); return; }
	USkillComponentModel* Skills = Unit->GetSkillComponentModel();
	if (!Reward.Offered)
	{
		UAssetManager* Manager = UAssetManager::GetIfInitialized();
		if (!Manager || !Skills) return;
		TArray<FPrimaryAssetId> Ids;
		Manager->GetPrimaryAssetIdList(SkillPrimaryAssetTypes::GetActiveType(), Ids);
		TArray<UStaticUnitSkillData*> Pool;
		for (const FPrimaryAssetId& Id : Ids)
		{
			UStaticUnitSkillData* Skill = LoadSkill(Id);
			if (Skill && !Skill->mSkillPhaseLayers.IsEmpty() && Skills->IsAcquirableSkill(Skill)
				&& !IsOwned(Skills, Id)) Pool.Add(Skill);
		}
		Reward.Candidates = LevelUpSkillReward::ChooseCandidates(Pool,
			ULevelAttributeSet::GetRarityRate(mController, Reward.Level), mRun->GetEventStream());
		Reward.Offered = true;
		Save();
	}
	// Old checkpoint offers must follow the same production-only policy.
	Reward.Candidates.RemoveAll([](const FPrimaryAssetId& Id) { return !GameplayAssetPolicy::IsPlayerFacing(Id); });

	FShopUI View;
	View.mIsLevelUpReward = true;
	View.mRewardTitle = FText::Format(NSLOCTEXT("LevelUpReward", "Target", "{0} · Lv.{1}"),
		Unit->GetBoardActorDisplayName(), FText::AsNumber(Reward.Level));
	View.mRewardOfferId = mActiveReward;
	View.mRewardUnitIndex = Reward.UnitIndex;
	for (int32 UnitIndex = 0; UnitIndex < Units.Num(); ++UnitIndex)
	{
		const int32 Pending = LevelUpSkillReward::FindPendingReward(Rewards, UnitIndex);
		if (Pending == INDEX_NONE || !Units[UnitIndex]) continue;
		FShopOwnedUnitUI Target;
		Target.mUnitIndex = UnitIndex;
		Target.mJobType = Units[UnitIndex]->GetUnitJobType();
		Target.mLevel = Rewards[Pending].Level;
		for (const FSkillEntry& Entry : Units[UnitIndex]->GetSkillComponentModel()->GetSkills())
		{
			FShopOwnedSkillSlotUI& Slot = Target.mSkillSlots.AddDefaulted_GetRef();
			Slot.mIsEmpty = !Entry.mData;
			if (Entry.mData)
			{
				Slot.mName = Entry.mData->mName;
				Slot.mIcon = Entry.mData->mIcon.LoadSynchronous();
				Slot.mDescription = Entry.mData->mDescription;
			}
		}
		View.mOwnedUnits.Add(Target);
		View.mSkillTargetUnits.Add(Target);
	}
	for (int32 Index = 0; Index < Reward.Candidates.Num(); ++Index)
	{
		const UStaticUnitSkillData* Skill = LoadSkill(Reward.Candidates[Index]);
		if (!Skill) { UE_LOG(LogTemp, Error, TEXT("Level-up candidate failed to load")); return; }
		FShopItemUI& Item = View.mItems.AddDefaulted_GetRef();
		// Encode the offer as well, so a double tap from the previous offer cannot consume the next one.
		Item.mSlotIndex = mActiveReward * 3 + Index;
		Item.mRequiredJobType = Skill->mJobType;
		Item.mName = Skill->mName;
		Item.mIcon = Skill->mIcon.LoadSynchronous();
		Item.mDescription = Skill->mDescription;
		Item.mRarityColor = Skill->mRarityType == ERarityType::Epic ? FLinearColor(.72f, .46f, .92f)
			: Skill->mRarityType == ERarityType::Rare ? FLinearColor(.42f, .66f, .95f) : FLinearColor(.72f, .78f, .75f);
	}
	if (!mWidget && mWidgetClass) mWidget = CreateWidget<UShopUIWidgetBase>(mController, mWidgetClass);
	if (!mWidget) { UE_LOG(LogTemp, Error, TEXT("Level-up shop screen could not be created")); return; }
	SuspendRewardScreens();
	mUIModel->SetShop(View);
	mWidget->BindUIModel(mUIModel);
	mWidget->OpenUI();
	mWidget->SetVisibility(ESlateVisibility::Visible);
}

void ULevelUpSkillRewardFlow::Select(int32 Choice, int32 UnitIndex, int32 SkillSlot)
{
	if (!mRun || mActiveReward == INDEX_NONE || Choice < 0 || Choice / 3 != mActiveReward) return;
	FLevelUpSkillReward& Reward = mRun->GetRoomTransactionsMutable().LevelUpSkills[mActiveReward];
	if (Reward.UnitIndex != UnitIndex || !Reward.Candidates.IsValidIndex(Choice % 3)) return;
	const auto& Units = mParty->GetPlayerUnitModels();
	if (!Units.IsValidIndex(UnitIndex)) return;
	if (LevelUpSkillReward::TryEquip(Reward, Units[UnitIndex], Reward.Candidates[Choice % 3], SkillSlot))
	{
		FinishActiveReward();
	}
}

void ULevelUpSkillRewardFlow::FinishActiveReward()
{
	const int32 UnitIndex = mRun->GetRoomTransactions().LevelUpSkills[mActiveReward].UnitIndex;
	Save();
	// Retire this offer immediately, including skip, before accepting another tap.
	mActiveReward = INDEX_NONE;
	mWidget->SetIsEnabled(false);
	mController->GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, UnitIndex]()
		{ if (mWidget) mWidget->SetIsEnabled(true); ShowNext(UnitIndex); }));
}

void ULevelUpSkillRewardFlow::SelectRewardUnit(int32 UnitIndex)
{
	if (!mRun || mActiveReward == INDEX_NONE
		|| LevelUpSkillReward::FindPendingReward(mRun->GetRoomTransactions().LevelUpSkills, UnitIndex) == INDEX_NONE) return;
	ShowNext(UnitIndex);
}

void ULevelUpSkillRewardFlow::ContinueWithoutCandidate()
{
	if (!mRun || mActiveReward == INDEX_NONE) return;
	if (LevelUpSkillReward::Skip(mRun->GetRoomTransactionsMutable().LevelUpSkills[mActiveReward]))
		FinishActiveReward();
}

void ULevelUpSkillRewardFlow::ShowDetail(int32 Choice)
{
	if (!mRun || mActiveReward == INDEX_NONE || Choice < 0 || Choice / 3 != mActiveReward) return;
	const auto& Reward = mRun->GetRoomTransactions().LevelUpSkills[mActiveReward];
	if (!Reward.Candidates.IsValidIndex(Choice % 3)) return;
	FSkillDetailUI Detail;
	SkillDetailUIBuilder::FillFromSkillData(LoadSkill(Reward.Candidates[Choice % 3]), Detail);
	mUIModel->SetSkillDetail(Detail);
}

void ULevelUpSkillRewardFlow::ShowOwnedDetail(int32 UnitIndex, int32 SkillSlot)
{
	if (!mParty || mActiveReward == INDEX_NONE || UnitIndex != mRun->GetRoomTransactions().LevelUpSkills[mActiveReward].UnitIndex) return;
	const auto& Units = mParty->GetPlayerUnitModels();
	if (!Units.IsValidIndex(UnitIndex) || !Units[UnitIndex]) return;
	const FSkillEntry* Entry = Units[UnitIndex]->GetSkillComponentModel()->GetSkill(SkillSlot);
	FSkillDetailUI Detail;
	if (Entry) SkillDetailUIBuilder::FillFromSkillData(Entry->mData, Detail);
	mUIModel->SetSkillDetail(Detail);
}
