#include "Misc/AutomationTest.h"

#include "Engine/Texture2D.h"
#include "UI/Combat/SkillCutInWidget.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RHI.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillCutInColdFrameTest,
	"P_RD.UI.SkillCutIn.ColdFrameDoesNotSkipPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkillCutInColdFrameTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("Editor world"), World)) return false;
	USkillCutInWidget* Widget = CreateWidget<USkillCutInWidget>(World);
	if (!TestNotNull(TEXT("Cut-in widget"), Widget)) return false;
	Widget->TakeWidget();
	FSkillCutInPresentationData Presentation;
	Presentation.LayerRig = ESkillCutInLayerRig::MasterDuelSingle;
	Presentation.DurationSeconds = .82f;
	Presentation.FailSafeSeconds = 1.12f;
	int32 Completed = 0;
	Widget->PlayCutIn(Presentation, FOnSkillCutInFinished::CreateLambda([&Completed]() { ++Completed; }));
	TestEqual(TEXT("No background flash before first frame"), Widget->FixedBackgroundCanvas->GetRenderOpacity(), 0.f);
	Widget->NativeTick(FGeometry(), 2.f);
	TestTrue(TEXT("A two-second loading frame must not finish the cut-in"), Widget->IsCutInPlaying());
	TestEqual(TEXT("Cold-frame time excluded"), Widget->ElapsedSeconds, 0.f);
	TestEqual(TEXT("Gameplay barrier remains held"), Completed, 0);
	Widget->NativeTick(FGeometry(), .2f);
	TestTrue(TEXT("Animation advances after setup"), Widget->ElapsedSeconds > .19f);
	Widget->NativeTick(FGeometry(), .7f);
	TestFalse(TEXT("Normal duration finishes"), Widget->IsCutInPlaying());
	TestEqual(TEXT("Finished root cannot paint an opaque last frame"), Widget->GetRenderOpacity(), 0.f);
	TestEqual(TEXT("Finished panel retains its faded pose"), Widget->PanelCanvas->GetRenderOpacity(), 0.f);
	TestEqual(TEXT("Finished background is not restored"), Widget->FixedBackgroundCanvas->GetRenderOpacity(), 0.f);
	TestEqual(TEXT("Finished foreground is not restored"), Widget->FixedFrontFXCanvas->GetRenderOpacity(), 0.f);
	Widget->NativeTick(FGeometry(), 1.f);
	TestEqual(TEXT("Completion fires exactly once"), Completed, 1);
	Widget->PlayCutIn(Presentation);
	TestEqual(TEXT("Explicit replay restores root opacity"), Widget->GetRenderOpacity(), 1.f);
	Widget->NativeTick(FGeometry(), 2.f);
	TestTrue(TEXT("Reused widget also ignores its setup delta"), Widget->IsCutInPlaying());
	Widget->StopCutIn();
	TestEqual(TEXT("Interrupted playback also remains transparent"), Widget->GetRenderOpacity(), 0.f);
	if (!GUsingNullRHI)
	{
		const FString Output = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UI/SkillCutInDrift"));
		IFileManager::Get().MakeDirectory(*Output, true);
		FWidgetRenderer Renderer(true, true);
		Renderer.SetIsPrepassNeeded(true);
		for (const bool bPlayer : { true, false })
		{
			const FString Prefix(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/"));
			auto Texture = [&Prefix](const TCHAR* Name)
			{
				return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Prefix + Name + TEXT(".") + Name));
			};
			Presentation.bMirror = bPlayer;
			Presentation.BodyTexture = Texture(bPlayer
				? TEXT("T_SkillCutIn_MasterDuelSingle_Knight_1672x941_v1")
				: TEXT("T_SkillCutIn_MasterDuelMonster_Character_v2"));
			Presentation.BackgroundTexture = Texture(bPlayer
				? TEXT("T_SkillCutIn_Mercenary_BrushBG_v5") : TEXT("T_SkillCutIn_Monster_BrushBG_v5"));
			Widget->PlayCutIn(Presentation);
			for (UTexture2D* TextureAsset : { Presentation.BodyTexture.Get(), Presentation.BackgroundTexture.Get() })
			{
				if (TextureAsset)
				{
					TextureAsset->UpdateResource();
					TextureAsset->SetForceMipLevelsToBeResident(30.f);
					TextureAsset->WaitForStreaming();
				}
			}
			// Sample exact presentation times independently of renderer wall time.
			Widget->bCutInPlaying = false;
			const TSharedRef<SWidget> Slate = Widget->TakeWidget();
			for (float Time : { .0f, .12f, .34f, .65f, .90f })
			{
				Widget->ApplyMotion(Time);
				Widget->UpdatePanelLayoutForViewport(FVector2D(1280.f, 720.f));
				Widget->ForceLayoutPrepass();
				for (int32 Warmup = 0; Warmup < 3; ++Warmup)
				{
					Renderer.DrawWidget(Slate, FVector2D(1280.f, 720.f));
					FlushRenderingCommands();
				}
				UTextureRenderTarget2D* Target = Renderer.DrawWidget(Slate, FVector2D(1280.f, 720.f));
				FlushRenderingCommands();
				if (!TestNotNull(TEXT("Cut-in frame render target"), Target)) return false;
				if (Time == .34f)
				{
					AddInfo(FString::Printf(TEXT("Cut-in capture: body=%s size=%s panel=%s opacity=%f visibility=%d"),
						*GetNameSafe(Widget->BodyLayer->GetBrush().GetResourceObject()),
						*Widget->BodyLayer->GetCachedGeometry().GetLocalSize().ToString(),
						*Widget->PanelCanvas->GetCachedGeometry().GetLocalSize().ToString(),
						Widget->BodyLayer->GetRenderOpacity(), int32(Widget->PanelCanvas->GetVisibility())));
					AddInfo(FString::Printf(TEXT("Cut-in geometry body pos=%s root=%s bg=%s"),
						*Widget->BodyLayer->GetCachedGeometry().GetAbsolutePosition().ToString(),
						*Widget->GetCachedGeometry().GetLocalSize().ToString(),
						*Widget->FixedBackgroundCanvas->GetCachedGeometry().GetAbsolutePosition().ToString()));
				}
				TArray<FColor> Pixels;
				if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels)) return false;
				if (Time == .34f)
				{
					int32 VisibleArtPixels = 0;
					for (const FColor& Pixel : Pixels)
					{
						if (FMath::Abs(int32(Pixel.R) - int32(Pixels[0].R))
							+ FMath::Abs(int32(Pixel.G) - int32(Pixels[0].G))
							+ FMath::Abs(int32(Pixel.B) - int32(Pixels[0].B)) > 30) ++VisibleArtPixels;
					}
					TestTrue(TEXT("Hold frame must render character art, not only the dim layer"), VisibleArtPixels > 1000);
				}
				TArray64<uint8> Png;
				FImageUtils::PNGCompressImageArray(1280, 720, Pixels, Png);
				const FString Name = FString::Printf(TEXT("%s_%03d.png"), bPlayer ? TEXT("Player") : TEXT("Monster"), FMath::RoundToInt(Time * 100));
				TestTrue(TEXT("Cut-in frame saved"), FFileHelper::SaveArrayToFile(Png, *FPaths::Combine(Output, Name)));
			}
			Widget->StopCutIn();
		}
	}

	// Check every new monster plate with the actual layered widget at a wide mobile viewport.
	if (!GUsingNullRHI)
	{
		const FVector2D MobileSize(844.f, 390.f);
		const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UI/MonsterCutIns"));
		IFileManager::Get().MakeDirectory(*Folder, true);
		FWidgetRenderer Renderer(true, true);
		Renderer.SetIsPrepassNeeded(true);
		for (const TCHAR* Path : {
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Stump_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Werewolf_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Bat_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_HatchetBird_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Mimic_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Pumpkin_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_SkeletonGolem_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_SkeletonMelee_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_SkeletonRanged_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Slime_Explosion_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Demon_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Ghoul_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Necromancer_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Reaper_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Skeleton_Mage_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Skeleton_Rogue_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Skeleton_Warrior_v1"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MonsterCutIns/T_CutIn_Zombie_v1") })
		{
			UTexture2D* Plate = LoadObject<UTexture2D>(nullptr, Path);
			if (!TestNotNull(Path, Plate)) return false;
			Presentation.bMirror = false;
			Presentation.BodyTexture = Plate;
			Presentation.BackgroundTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_Monster_BrushBG_v5.T_SkillCutIn_Monster_BrushBG_v5")));
			Presentation.SpeedLinesTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelMonster_SpeedFX_v1.T_SkillCutIn_MasterDuelMonster_SpeedFX_v1")));
			Presentation.ForegroundTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/T_SkillCutIn_MasterDuelMonster_ImpactFX_v1.T_SkillCutIn_MasterDuelMonster_ImpactFX_v1")));
			Widget->PlayCutIn(Presentation);
			TestEqual(TEXT("Mapped body is used without fallback"), Widget->BodyLayer->GetBrush().GetResourceObject(), static_cast<UObject*>(Plate));
			Plate->UpdateResource(); Plate->SetForceMipLevelsToBeResident(30.f); Plate->WaitForStreaming();
			Widget->bCutInPlaying = false;
			Widget->ApplyMotion(.34f);
			Widget->UpdatePanelLayoutForViewport(MobileSize);
			Widget->ForceLayoutPrepass();
			const TSharedRef<SWidget> Slate = Widget->TakeWidget();
			for (int32 Warmup = 0; Warmup < 3; ++Warmup)
			{
				Renderer.DrawWidget(Slate, MobileSize); FlushRenderingCommands();
			}
			UTextureRenderTarget2D* Target = Renderer.DrawWidget(Slate, MobileSize);
			FlushRenderingCommands();
			if (!TestNotNull(TEXT("Mobile render target"), Target)) return false;
			TArray<FColor> Pixels;
			if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels)) return false;
			int32 Visible = 0;
			for (const FColor& Pixel : Pixels)
				if (FMath::Abs(int32(Pixel.R)-int32(Pixels[0].R))+FMath::Abs(int32(Pixel.G)-int32(Pixels[0].G))+FMath::Abs(int32(Pixel.B)-int32(Pixels[0].B)) > 30) ++Visible;
			TestTrue(TEXT("Mobile hold frame contains visible art"), Visible > 1000);
			TArray64<uint8> Png;
			FImageUtils::PNGCompressImageArray(844,390,Pixels,Png);
			TestTrue(TEXT("Mobile plate capture saved"), FFileHelper::SaveArrayToFile(Png,*FPaths::Combine(Folder,Plate->GetName()+TEXT("_mobile.png"))));
			Widget->StopCutIn();
			TestEqual(TEXT("Roster playback stops transparent"),Widget->GetRenderOpacity(),0.f);
		}
	}
	return true;
}


#endif
