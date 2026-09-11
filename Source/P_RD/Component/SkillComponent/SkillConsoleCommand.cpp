/*****************************************************************//**
 * @file   SkillConsoleCommand.cpp
 * @brief  스킬 테스트용 콘솔 명령
 * @details
 * RD.AddSkill <유닛번호> <스킬슬롯> <에셋이름>  파티 유닛의 슬롯에 스킬 장착 (이름 일부 허용)
 * RD.RemoveSkill <유닛번호> <스킬슬롯>           파티 유닛의 슬롯 비우기
 * RD.ListSkills                                 파티 유닛별 스킬 슬롯 출력
 * @author 이문환
 * @date   2026-09-11
 *********************************************************************/

#if !UE_BUILD_SHIPPING

#include "RDMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/AssetManager.h"

#include "GameMode/RoomGameModeBase.h"
#include "Actor/Party/PartyModel.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/PrimaryAssetType.h"

namespace
{
	// 현재 방의 파티 모델 (없으면 nullptr)
	UPartyModel* FindParty(UWorld* World)
	{
		ARoomGameModeBase* GameMode = World != nullptr ? World->GetAuthGameMode<ARoomGameModeBase>() : nullptr;
		return GameMode != nullptr ? GameMode->GetPartyModel() : nullptr;
	}

	// 유닛번호로 파티 유닛의 스킬 컴포넌트 획득 (범위 밖이면 nullptr)
	USkillComponentModel* FindUnitSkills(UWorld* World, int32 UnitIndex)
	{
		UPartyModel* Party = FindParty(World);
		if (Party == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("파티 없음: 방 안에서만 사용 가능"));
			return nullptr;
		}

		const TArray<TObjectPtr<UPlayerUnitModel>>& Units = Party->GetPlayerUnitModels();
		if (Units.IsValidIndex(UnitIndex) == false || Units[UnitIndex] == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("유닛번호 범위 밖: %d (파티 %d명)"), UnitIndex, Units.Num());
			return nullptr;
		}
		return Units[UnitIndex]->GetSkillComponentModel();
	}

	// 문자열 인자를 정수로 변환
	bool GetIntArg(const TArray<FString>& Args, int32 ArgIndex, int32& OutValue)
	{
		if (Args.IsValidIndex(ArgIndex) == false || Args[ArgIndex].IsNumeric() == false)
		{
			return false;
		}
		OutValue = FCString::Atoi(*Args[ArgIndex]);
		return true;
	}

	// 이름으로 스킬 ID 검색 (정확 일치 우선, 부분 일치는 하나일 때만)
	bool FindSkillId(const FString& Query, FPrimaryAssetId& OutId)
	{
		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (AssetManager == nullptr)
		{
			return false;
		}

		TArray<FPrimaryAssetId> Ids;
		AssetManager->GetPrimaryAssetIdList(SkillPrimaryAssetTypes::GetActiveType(), Ids);

		TArray<FPrimaryAssetId> Matched;
		for (const FPrimaryAssetId& Id : Ids)
		{
			const FString Name = Id.PrimaryAssetName.ToString();
			if (Name.Equals(Query, ESearchCase::IgnoreCase))
			{
				OutId = Id;
				return true;
			}
			if (Name.Contains(Query, ESearchCase::IgnoreCase))
			{
				Matched.Add(Id);
			}
		}

		if (Matched.Num() == 1)
		{
			OutId = Matched[0];
			return true;
		}
		for (const FPrimaryAssetId& Id : Matched)
		{
			UE_LOG(LogRD, Warning, TEXT("후보: %s"), *Id.PrimaryAssetName.ToString());
		}
		UE_LOG(LogRD, Warning, TEXT("스킬 검색 실패: '%s' (일치 %d개)"), *Query, Matched.Num());
		return false;
	}

	// 스킬 DA 로드 (이미 로드됐으면 그대로, 아니면 동기 로드)
	UStaticSkillData* LoadSkillData(const FPrimaryAssetId& SkillId)
	{
		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (AssetManager == nullptr)
		{
			return nullptr;
		}
		if (UStaticSkillData* Loaded = AssetManager->GetPrimaryAssetObject<UStaticSkillData>(SkillId))
		{
			return Loaded;
		}
		return Cast<UStaticSkillData>(AssetManager->GetPrimaryAssetPath(SkillId).TryLoad());
	}

	// 파티 유닛 슬롯에 스킬 장착
	void AddSkill(const TArray<FString>& Args, UWorld* World)
	{
		int32 UnitIndex = 0;
		int32 SlotIndex = 0;
		if (GetIntArg(Args, 0, UnitIndex) == false || GetIntArg(Args, 1, SlotIndex) == false || Args.Num() < 3)
		{
			UE_LOG(LogRD, Warning, TEXT("사용법: RD.AddSkill <유닛번호> <스킬슬롯> <에셋이름>"));
			return;
		}

		USkillComponentModel* Skills = FindUnitSkills(World, UnitIndex);
		if (Skills == nullptr)
		{
			return;
		}
		if (Skills->GetSkills().IsValidIndex(SlotIndex) == false)
		{
			UE_LOG(LogRD, Warning, TEXT("스킬슬롯 범위 밖: %d (슬롯 %d개)"), SlotIndex, Skills->GetSkills().Num());
			return;
		}

		FPrimaryAssetId SkillId;
		if (FindSkillId(Args[2], SkillId) == false)
		{
			return;
		}
		UStaticSkillData* SkillData = LoadSkillData(SkillId);
		if (SkillData == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("스킬 로드 실패: %s"), *SkillId.ToString());
			return;
		}

		// 직업 불일치 등은 SetSkill이 거부
		if (Skills->SetSkill(SlotIndex, SkillData) == false)
		{
			UE_LOG(LogRD, Warning, TEXT("스킬 장착 거부: %s (유닛 %d, 직업 불일치 등)"), *SkillId.PrimaryAssetName.ToString(), UnitIndex);
			return;
		}
		UE_LOG(LogRD, Log, TEXT("스킬 장착: 유닛 %d 슬롯 %d <- %s"), UnitIndex, SlotIndex, *SkillId.PrimaryAssetName.ToString());
	}

	FAutoConsoleCommandWithWorldAndArgs AddSkillCommand(
		TEXT("RD.AddSkill"),
		TEXT("파티 유닛의 스킬 슬롯에 스킬을 장착한다. 사용법: RD.AddSkill <유닛번호> <스킬슬롯> <에셋이름(일부 가능)>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AddSkill));

	// 파티 유닛 슬롯 비우기
	void RemoveSkill(const TArray<FString>& Args, UWorld* World)
	{
		int32 UnitIndex = 0;
		int32 SlotIndex = 0;
		if (GetIntArg(Args, 0, UnitIndex) == false || GetIntArg(Args, 1, SlotIndex) == false)
		{
			UE_LOG(LogRD, Warning, TEXT("사용법: RD.RemoveSkill <유닛번호> <스킬슬롯>"));
			return;
		}

		USkillComponentModel* Skills = FindUnitSkills(World, UnitIndex);
		if (Skills == nullptr)
		{
			return;
		}
		if (Skills->GetSkills().IsValidIndex(SlotIndex) == false)
		{
			UE_LOG(LogRD, Warning, TEXT("스킬슬롯 범위 밖: %d (슬롯 %d개)"), SlotIndex, Skills->GetSkills().Num());
			return;
		}

		const FString PreName = GetNameSafe(Skills->GetSkills()[SlotIndex].mData);
		Skills->RemoveSkill(SlotIndex);
		UE_LOG(LogRD, Log, TEXT("스킬 해제: 유닛 %d 슬롯 %d (%s)"), UnitIndex, SlotIndex, *PreName);
	}

	FAutoConsoleCommandWithWorldAndArgs RemoveSkillCommand(
		TEXT("RD.RemoveSkill"),
		TEXT("파티 유닛의 스킬 슬롯을 비운다. 사용법: RD.RemoveSkill <유닛번호> <스킬슬롯>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RemoveSkill));

	// 파티 유닛별 스킬 슬롯 출력
	void ListSkills(const TArray<FString>& Args, UWorld* World)
	{
		UPartyModel* Party = FindParty(World);
		if (Party == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("파티 없음: 방 안에서만 사용 가능"));
			return;
		}

		const TArray<TObjectPtr<UPlayerUnitModel>>& Units = Party->GetPlayerUnitModels();
		UE_LOG(LogRD, Log, TEXT("파티 유닛 %d명"), Units.Num());
		for (int32 UnitIndex = 0; UnitIndex < Units.Num(); ++UnitIndex)
		{
			USkillComponentModel* Skills = Units[UnitIndex] != nullptr ? Units[UnitIndex]->GetSkillComponentModel() : nullptr;
			if (Skills == nullptr)
			{
				UE_LOG(LogRD, Log, TEXT("  [%d] %s: 스킬 컴포넌트 없음"), UnitIndex, *GetNameSafe(Units[UnitIndex]));
				continue;
			}

			UE_LOG(LogRD, Log, TEXT("  [%d] %s"), UnitIndex, *GetNameSafe(Units[UnitIndex]));
			const TArray<FSkillEntry>& Entries = Skills->GetSkills();
			for (int32 SlotIndex = 0; SlotIndex < Entries.Num(); ++SlotIndex)
			{
				UE_LOG(LogRD, Log, TEXT("      슬롯 %d: %s"), SlotIndex, Entries[SlotIndex].mData != nullptr ? *GetNameSafe(Entries[SlotIndex].mData) : TEXT("(빈 슬롯)"));
			}
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ListSkillsCommand(
		TEXT("RD.ListSkills"),
		TEXT("파티 유닛별 스킬 슬롯을 출력한다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListSkills));
}

#endif
