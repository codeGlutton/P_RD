#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Setting/GamePlaySettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSVNUiDynamicAssetContractTest,
	"P_RD.UI.SVNAssets.DynamicPathContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSVNUiDynamicAssetContractTest::RunTest(const FString& Parameters)
{
	const FString CanonicalUiRoot = TEXT(
		"/Game/SVN/OutSideAsset/AICreation/UI/P_RD/");
	const FString SvnUiMountRoot = FPaths::GetPath(CanonicalUiRoot.LeftChop(1)) + TEXT("/");
	const TCHAR* RequiredTexturePaths[] = {
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Items/Equipment/T_equip_weapon_common.T_equip_weapon_common"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Common/Icons/T_Reward_GoldIcon_V1.T_Reward_GoldIcon_V1"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/Result/Rewards/T_reward_v4_gold_icon.T_reward_v4_gold_icon"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/Status/T_Status_Agility.T_Status_Agility"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/Status/T_Status_Fortification.T_Status_Fortification"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/Status/T_Status_Vulnerability.T_Status_Vulnerability"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/Status/T_Status_Weakness.T_Status_Weakness"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Characters/Mercenaries/T_MB_HireHero_Knight.T_MB_HireHero_Knight"),
	};
	for (const TCHAR* Path : RequiredTexturePaths)
	{
		TestTrue(FString::Printf(TEXT("동적 UI 경로는 캐노니컬 루트를 써야 함: %s"), Path),
			FString(Path).StartsWith(CanonicalUiRoot));
		TestNotNull(FString::Printf(TEXT("동적 UI 텍스처가 존재해야 함: %s"), Path),
			LoadObject<UTexture2D>(nullptr, Path));
	}

	const FString TurnFrameRoot = CanonicalUiRoot + TEXT("Combat/TurnChange/Frames/");
	TestTrue(TEXT("턴 전환 동적 루트는 캐노니컬 UI 루트 하위여야 함"),
		TurnFrameRoot.StartsWith(CanonicalUiRoot));
	for (int32 FrameNumber = 1; FrameNumber <= 33; FrameNumber += 2)
	{
		const FString AssetName = FString::Printf(TEXT("T_TurnChange_%03d"), FrameNumber);
		const FString Path = TurnFrameRoot + AssetName + TEXT(".") + AssetName;
		TestNotNull(FString::Printf(TEXT("턴 전환 프레임이 존재해야 함: %s"), *Path),
			LoadObject<UTexture2D>(nullptr, *Path));
	}

	const FString ExpectedAlwaysCookRoots[] = {
		CanonicalUiRoot + TEXT("Combat/TurnChange/Frames"),
		CanonicalUiRoot + TEXT("Combat/Result/Rewards"),
		CanonicalUiRoot + TEXT("Characters/Mercenaries"),
	};
	TArray<FString> AlwaysCookEntries;
	TestTrue(TEXT("ProjectPackagingSettings의 DirectoriesToAlwaysCook를 읽어야 함"),
		GConfig != nullptr && GConfig->GetArray(
			TEXT("/Script/UnrealEd.ProjectPackagingSettings"),
			TEXT("DirectoriesToAlwaysCook"), AlwaysCookEntries, GGameIni));
	for (const FString& ExpectedRoot : ExpectedAlwaysCookRoots)
	{
		const bool bHasRoot = AlwaysCookEntries.ContainsByPredicate(
			[&ExpectedRoot](const FString& Entry)
			{
				return Entry.Contains(ExpectedRoot);
			});
		TestTrue(FString::Printf(TEXT("동적 UI 루트를 강제 쿡해야 함: %s"), *ExpectedRoot),
			bHasRoot);
	}
	int32 SvnUiAlwaysCookCount = 0;
	for (const FString& Entry : AlwaysCookEntries)
	{
		if (Entry.Contains(SvnUiMountRoot))
		{
			++SvnUiAlwaysCookCount;
			TestTrue(FString::Printf(TEXT("SVN UI 강제 쿡 경로는 캐노니컬 루트를 써야 함: %s"), *Entry),
				Entry.Contains(CanonicalUiRoot));
		}
	}
	TestEqual(TEXT("SVN UI 강제 쿡은 동적 로드 루트 3개만 유지해야 함"),
		SvnUiAlwaysCookCount, static_cast<int32>(UE_ARRAY_COUNT(ExpectedAlwaysCookRoots)));

	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	if (TestNotNull(TEXT("게임플레이 설정을 로드해야 함"), GamePlaySettings))
	{
		const FString CanonicalMediaRoot = TEXT(
			"SVN/OutSideAsset/AICreation/UI/P_RD/Combat/Result/Media/");
		const FString MediaPaths[] = {
			GamePlaySettings->mCombatVictoryVideoPath,
			GamePlaySettings->mCombatDefeatVideoPath,
		};
		for (const FString& RelativePath : MediaPaths)
		{
			TestTrue(FString::Printf(TEXT("전투 결과 MP4는 캐노니컬 미디어 루트를 써야 함: %s"), *RelativePath),
				RelativePath.StartsWith(CanonicalMediaRoot));
			const FString FullPath = FPaths::ConvertRelativePathToFull(
				FPaths::Combine(FPaths::ProjectContentDir(), RelativePath));
			TestTrue(FString::Printf(TEXT("전투 결과 MP4가 존재해야 함: %s"), *FullPath),
				IFileManager::Get().FileExists(*FullPath));
		}
	}

	const TCHAR* UnitDataPaths[] = {
		TEXT("/Game/BP/DataAsset/Unit/PlayerUnit/DA_TestPlayerUnit.DA_TestPlayerUnit"),
		TEXT("/Game/BP/DataAsset/Unit/PlayerUnit/DA_TestKnightPlayerUnit.DA_TestKnightPlayerUnit"),
		TEXT("/Game/BP/DataAsset/Unit/PlayerUnit/DA_TestMagePlayerUnit.DA_TestMagePlayerUnit"),
		TEXT("/Game/BP/DataAsset/Unit/PlayerUnit/DA_TestRangerPlayerUnit.DA_TestRangerPlayerUnit"),
		TEXT("/Game/BP/DataAsset/Unit/PlayerUnit/DA_TestRoguePlayerUnit.DA_TestRoguePlayerUnit"),
		TEXT("/Game/BP/DataAsset/Unit/PlayerUnit/DA_TestBarbarianPlayerUnit.DA_TestBarbarianPlayerUnit"),
		TEXT("/Game/BP/DataAsset/Unit/PlayerUnit/DA_TestDruidPlayerUnit.DA_TestDruidPlayerUnit"),
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/DA_TestEnemyUnit.DA_TestEnemyUnit"),
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_EagleUnit.DA_EagleUnit"),
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_GolemUnit.DA_GolemUnit"),
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_SpiderUnit.DA_SpiderUnit"),
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_WerewolfUnit.DA_WerewolfUnit"),
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_LeshyUnit.DA_LeshyUnit"),
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_MushroomUnit.DA_MushroomUnit"),
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_SlimeUnit.DA_SlimeUnit"),
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage3/DA_SkeletonGolem.DA_SkeletonGolem"),
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage3/DA_SkeletonMinionMeleeUnit.DA_SkeletonMinionMeleeUnit"),
		TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage3/DA_SkeletonMinionRangedUnit.DA_SkeletonMinionRangedUnit"),
	};
	for (const TCHAR* Path : UnitDataPaths)
	{
		UStaticUnitSpawnData* UnitData = LoadObject<UStaticUnitSpawnData>(nullptr, Path);
		if (TestNotNull(FString::Printf(TEXT("유닛 데이터가 존재해야 함: %s"), Path), UnitData))
		{
			TestFalse(FString::Printf(TEXT("초상화 경로가 비어 있으면 안 됨: %s"), Path),
				UnitData->mPortrait.IsNull());
			TestFalse(FString::Printf(TEXT("아이콘 경로가 비어 있으면 안 됨: %s"), Path),
				UnitData->mIcon.IsNull());
			TestNotNull(FString::Printf(TEXT("초상화가 로드되어야 함: %s"), Path),
				UnitData->mPortrait.LoadSynchronous());
			TestNotNull(FString::Printf(TEXT("아이콘이 로드되어야 함: %s"), Path),
				UnitData->mIcon.LoadSynchronous());
		}
	}

	return true;
}

#endif
