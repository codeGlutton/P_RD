#include "Commandlet/RefreshPullDescriptionsCommandlet.h"
#include "Commandlet/DataAssetImportUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Pull.h"
#include "Internationalization/Internationalization.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"

URefreshPullDescriptionsCommandlet::URefreshPullDescriptionsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URefreshPullDescriptionsCommandlet::Main(const FString& Params)
{
	FInternationalization::Get().SetCurrentCulture(TEXT("ko"));
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.ScanPathsSynchronous({TEXT("/Game/BP/DataAsset/Skill")}, true);
	TArray<FAssetData> Assets;
	Registry.GetAssetsByPath(TEXT("/Game/BP/DataAsset/Skill"), Assets, true);
	int32 Changed = 0;
	for (const FAssetData& Asset : Assets)
	{
		auto* Skill = Cast<UStaticSkillData>(Asset.GetAsset());
		if (!Skill || !Skill->mDescription.ToString().Contains(TEXT("대상을 시전자 옆까지 끌어옵니다."))) continue;
		bool bHasPull = false;
		for (const auto& Phase : Skill->mSkillPhaseLayers)
			for (const auto& Layer : Phase.mSkillEffectLayers)
				bHasPull |= Layer.IsValid() && Layer.GetScriptStruct()->IsChildOf(FSkillEffectLayer_Pull::StaticStruct());
		if (!bHasPull) continue;
		const FText Description = Skill->MakeDescription();
		if (Description.IsEmpty() || Description.ToString().Contains(TEXT("대상을 시전자 옆까지 끌어옵니다."))) return 1;
		UE_LOG(LogTemp, Display, TEXT("RefreshPullDescriptions %s: %s -> %s"),
			*Skill->GetPathName(), *Skill->mDescription.ToString(), *Description.ToString());
		if (bApply)
		{
			Skill->Modify();
			Skill->mDescription = Description;
			if (!DataAssetImportUtils::SaveAsset(Skill)) return 1;
		}
		++Changed;
	}
	UE_LOG(LogTemp, Display, TEXT("RefreshPullDescriptions: %d %s"), Changed, bApply ? TEXT("saved") : TEXT("would change"));
	return 0;
}
