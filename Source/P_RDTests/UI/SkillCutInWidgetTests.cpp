#include "Misc/AutomationTest.h"

#include "Engine/Texture2D.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkillCutInGeneratedTextureLoadTest,
	"P_RD.UI.SkillCutIn.GeneratedTexturesLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkillCutInGeneratedTextureLoadTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* ObjectPath : {
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Mercenary_BrushBG_v5.T_SkillCutIn_Mercenary_BrushBG_v5"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelSingle_SpeedFX_v3.T_SkillCutIn_MasterDuelSingle_SpeedFX_v3"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelSingle_ImpactFX_v3.T_SkillCutIn_MasterDuelSingle_ImpactFX_v3"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelSingle_Knight_1672x941_v1.T_SkillCutIn_MasterDuelSingle_Knight_1672x941_v1"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Monster_BrushBG_v5.T_SkillCutIn_Monster_BrushBG_v5"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelMonster_SpeedFX_v1.T_SkillCutIn_MasterDuelMonster_SpeedFX_v1"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelMonster_ImpactFX_v1.T_SkillCutIn_MasterDuelMonster_ImpactFX_v1"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelMonster_Character_v2.T_SkillCutIn_MasterDuelMonster_Character_v2"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Mercenary_Barbarian_v2.T_SkillCutIn_Roster_Mercenary_Barbarian_v2"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Mercenary_Druid_v2.T_SkillCutIn_Roster_Mercenary_Druid_v2"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Mercenary_Mage_v2.T_SkillCutIn_Roster_Mercenary_Mage_v2"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Mercenary_Ranger_v2.T_SkillCutIn_Roster_Mercenary_Ranger_v2"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Mercenary_Rogue_v2.T_SkillCutIn_Roster_Mercenary_Rogue_v2"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Monster_Eagle_v1.T_SkillCutIn_Roster_Monster_Eagle_v1"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Monster_Golem_v1.T_SkillCutIn_Roster_Monster_Golem_v1"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Monster_Leshy_v1.T_SkillCutIn_Roster_Monster_Leshy_v1"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Monster_Mushroom_v1.T_SkillCutIn_Roster_Monster_Mushroom_v1"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Roster_Monster_Spider_v1.T_SkillCutIn_Roster_Monster_Spider_v1") })
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, ObjectPath);
		if (TestNotNull(ObjectPath, Texture))
		{
			const FIntPoint ImportedSize = Texture->GetImportedSize();
			TestEqual(*FString::Printf(TEXT("%s width"), ObjectPath), ImportedSize.X, 1672);
			TestEqual(*FString::Printf(TEXT("%s height"), ObjectPath), ImportedSize.Y, 941);
			TestEqual(*FString::Printf(TEXT("%s UI texture group"), ObjectPath),
				Texture->LODGroup, TEXTUREGROUP_UI);
			TestEqual(*FString::Printf(TEXT("%s clamp X"), ObjectPath),
				Texture->AddressX, TA_Clamp);
			TestEqual(*FString::Printf(TEXT("%s clamp Y"), ObjectPath),
				Texture->AddressY, TA_Clamp);
			TestTrue(*FString::Printf(TEXT("%s never streams"), ObjectPath),
				Texture->NeverStream);
		}
	}
	return true;
}

#endif
