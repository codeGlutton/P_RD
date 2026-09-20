/*****************************************************************//**
 * @file   ArtifactConsoleCommand.cpp
 * @brief  아티팩트 테스트용 콘솔 명령
 * @details
 * RD.AddArtifact <에셋이름>  현재 방 파티에 아티팩트 추가 (이름 일부 허용)
 * RD.ListArtifacts           파티 보유 아티팩트 출력
 * @author 이문환
 * @date   2026-09-10
 *********************************************************************/

#if !UE_BUILD_SHIPPING

#include "RDMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/AssetManager.h"

#include "GameMode/RoomGameModeBase.h"
#include "Actor/Party/PartyModel.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"

namespace
{
	// 현재 방의 파티 아티팩트 컴포넌트 (없으면 nullptr)
	UPartyArtifactComponentModel* FindPartyArtifacts(UWorld* World)
	{
		ARoomGameModeBase* GameMode = World != nullptr ? World->GetAuthGameMode<ARoomGameModeBase>() : nullptr;
		UPartyModel* PartyModel = GameMode != nullptr ? GameMode->GetPartyModel() : nullptr;
		return PartyModel != nullptr ? PartyModel->GetPartyArtifactComponentModel() : nullptr;
	}

	// 이름으로 아티팩트 ID 검색 (정확 일치 우선, 부분 일치는 하나일 때만)
	bool FindArtifactId(const FString& Query, FPrimaryAssetId& OutId)
	{
		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (AssetManager == nullptr)
		{
			return false;
		}

		TArray<FPrimaryAssetId> Ids;
		AssetManager->GetPrimaryAssetIdList(ArtifactPrimaryAssetTypes::GetArtifactType(), Ids);

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
		UE_LOG(LogRD, Warning, TEXT("아티팩트 검색 실패: '%s' (일치 %d개)"), *Query, Matched.Num());
		return false;
	}

	// 파티에 아티팩트 추가
	void AddArtifact(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogRD, Warning, TEXT("사용법: RD.AddArtifact <에셋이름>"));
			return;
		}

		UPartyArtifactComponentModel* PartyArtifacts = FindPartyArtifacts(World);
		if (PartyArtifacts == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("파티 없음: 방 안에서만 사용 가능"));
			return;
		}

		FPrimaryAssetId ArtifactId;
		if (FindArtifactId(Args[0], ArtifactId) == false)
		{
			return;
		}
		if (PartyArtifacts->AddArtifact(ArtifactId) == false)
		{
			UE_LOG(LogRD, Warning, TEXT("아티팩트 추가 실패: %s"), *ArtifactId.ToString());
			return;
		}
		UE_LOG(LogRD, Log, TEXT("아티팩트 추가: %s (보유 %d개)"), *ArtifactId.PrimaryAssetName.ToString(), PartyArtifacts->GetPartyArtifacts().Num());
	}

	FAutoConsoleCommandWithWorldAndArgs AddArtifactCommand(
		TEXT("RD.AddArtifact"),
		TEXT("현재 방 파티에 아티팩트를 추가한다. 사용법: RD.AddArtifact <에셋이름(일부 가능)>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AddArtifact));

	// 파티 보유 아티팩트 출력
	void ListArtifacts(const TArray<FString>& Args, UWorld* World)
	{
		UPartyArtifactComponentModel* PartyArtifacts = FindPartyArtifacts(World);
		if (PartyArtifacts == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("파티 없음: 방 안에서만 사용 가능"));
			return;
		}

		const TArray<TObjectPtr<UStaticArtifactData>>& Artifacts = PartyArtifacts->GetPartyArtifacts();
		UE_LOG(LogRD, Log, TEXT("파티 아티팩트 %d개"), Artifacts.Num());
		for (int32 Index = 0; Index < Artifacts.Num(); ++Index)
		{
			UE_LOG(LogRD, Log, TEXT("  [%d] %s"), Index, *GetNameSafe(Artifacts[Index]));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ListArtifactsCommand(
		TEXT("RD.ListArtifacts"),
		TEXT("파티 보유 아티팩트를 출력한다."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListArtifacts));
}

#endif
