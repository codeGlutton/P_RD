#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"
#include "Engine/Texture2D.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSVNUiDynamicAssetContractTest,
	"P_RD.UI.SVNAssets.DynamicPathContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSVNUiDynamicAssetContractTest::RunTest(const FString& Parameters)
{
	const TCHAR* RequiredTexturePaths[] = {
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Equipment/T_equip_weapon_common.T_equip_weapon_common"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Icons/T_Reward_GoldIcon_V1.T_Reward_GoldIcon_V1"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Agility.T_Status_Agility"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Fortification.T_Status_Fortification"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Vulnerability.T_Status_Vulnerability"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Weakness.T_Status_Weakness"),
	};
	for (const TCHAR* Path : RequiredTexturePaths)
	{
		TestNotNull(FString::Printf(TEXT("동적 UI 텍스처가 존재해야 함: %s"), Path),
			LoadObject<UTexture2D>(nullptr, Path));
	}

	const FString TurnFrameRoot = TEXT(
		"/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/TurnChange/FramesBiRefNet/");
	for (int32 FrameNumber = 1; FrameNumber <= 33; FrameNumber += 2)
	{
		const FString AssetName = FString::Printf(TEXT("T_TurnChange_%03d"), FrameNumber);
		const FString Path = TurnFrameRoot + AssetName + TEXT(".") + AssetName;
		TestNotNull(FString::Printf(TEXT("턴 전환 프레임이 존재해야 함: %s"), *Path),
			LoadObject<UTexture2D>(nullptr, *Path));
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
