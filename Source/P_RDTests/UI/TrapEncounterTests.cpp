#include "Misc/AutomationTest.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "DataAsset/ObstacleSpawnData/StaticGimmickSpawnData.h"
#include "UI/Tutorial/GuidedTutorialWidget.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Editor.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureCompiler.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrapHintSaveTest, "P_RD.Tutorial.Encounters.Save", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTrapHintSaveTest::RunTest(const FString&)
{
    auto* Profile = NewObject<UUserPersistData>();
    const FPrimaryAssetId Barrel(TEXT("CombatTargetObstacle"), TEXT("DA_Barrel_ExplosionGimmick"));
    const FPrimaryAssetId Stun(TEXT("CombatTargetObstacle"), TEXT("DA_StunTrapGimmick"));
    TestTrue(TEXT("Existing profiles default to no acknowledged trap types"), Profile->SeenTrapHints.IsEmpty());
    Profile->SeenTrapHints.Add(Barrel);
    Profile->SeenTrapHints.Add(Barrel);
    TArray<uint8> Bytes;
    { FMemoryWriter W(Bytes); FObjectAndNameAsStringProxyArchive A(W, false); A.ArIsSaveGame = true; A.ArNoDelta = true; Profile->Serialize(A); }
    auto* Reloaded = NewObject<UUserPersistData>();
    { FMemoryReader R(Bytes); FObjectAndNameAsStringProxyArchive A(R, false); A.ArIsSaveGame = true; A.ArNoDelta = true; Reloaded->Serialize(A); }
    TestTrue(TEXT("Acknowledged type survives a profile save/load"), Reloaded->SeenTrapHints.Contains(Barrel));
    TestFalse(TEXT("Different trap type remains unseen"), Reloaded->SeenTrapHints.Contains(Stun));
    TestEqual(TEXT("Duplicate trap instances share a single record"), Reloaded->SeenTrapHints.Num(), 1);
    TestFalse(TEXT("Encounter history does not enroll an existing player in first-room tutorial"), Reloaded->GuidedTutorial.Enrolled);
    Reloaded->ClearUser();
    TestTrue(TEXT("Explicit profile reset permits learning traps again"), Reloaded->SeenTrapHints.IsEmpty());
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrapHintRenderTest, "P_RD.Tutorial.Encounters.RenderAndIdle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTrapHintRenderTest::RunTest(const FString&)
{
    if (GUsingNullRHI) { AddError(TEXT("Rendering required")); return false; }
    auto* World = GEditor->GetEditorWorldContext().World();
    auto* HUDClass = LoadClass<UCombatLayoutHUDWidget>(nullptr, TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
    auto* HUD = CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass);
    HUD->mUsePreviewData = false;
    HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    const auto HUDSlate = HUD->TakeWidget();
    auto* Model = NewObject<UCombatUIModel>(HUD);
    HUD->BindUIModel(Model);
    HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible); // World widget manager opens the constructed HUD in game.
    TestTrue(TEXT("Idle board can show a hint"), HUD->CanShowEncounterHint());
    Model->SetBuildPhase(ECombatBuildPhaseUI::AimSelection);
    TestFalse(TEXT("Do not interrupt skill target selection"), HUD->CanShowEncounterHint());
    Model->SetBuildPhase(ECombatBuildPhaseUI::None);
    FCombatPendingActionUI Pending; Pending.mType = ECombatPendingActionType::Move;
    Model->SetPendingAction(Pending);
    TestFalse(TEXT("Do not interrupt a planned movement"), HUD->CanShowEncounterHint());
    Model->SetPendingAction(FCombatPendingActionUI());
    FSkillDetailUI Detail; Detail.mSkillIndex = 0; Detail.mName = FText::FromString(TEXT("Skill"));
    Model->SetSkillDetail(Detail);
    TestFalse(TEXT("Do not overlap a skill explanation"), HUD->CanShowEncounterHint());

    const FString Folder = FPaths::ProjectSavedDir() / TEXT("UI/TrapEncounters");
    IFileManager::Get().MakeDirectory(*Folder, true);
    for (const TCHAR* Name : {TEXT("DA_Barrel_ExplosionGimmick"), TEXT("DA_BlackHoleTotemGimmick"), TEXT("DA_BoobyTrapGimmick"),
        TEXT("DA_FirePuddleGimmick"), TEXT("DA_HealTotemGimmick"), TEXT("DA_PoisonPuddleGimmick"), TEXT("DA_PushPlateGimmick"), TEXT("DA_StunTrapGimmick")})
    {
        auto* Data = LoadObject<UStaticGimmickSpawnData>(nullptr, *(FString(TEXT("/Game/BP/DataAsset/CombatTargetObstacle/")) + Name + TEXT(".") + Name));
        if (!TestNotNull(Name, Data)) continue;
        TestFalse(TEXT("Every shipped trap has an authored explanation"), Data->mDescription.IsEmpty());
        auto* Widget = CreateWidget<UGuidedTutorialWidget>(World);
        Widget->PresentEncounter(Data->mDisplayName, Data->mDescription); // Deliberately before RebuildWidget.
        Widget->SetWorldFocus({{690, 220}, {790, 220}, {790, 350}, {690, 350}});
        const auto Slate = Widget->TakeWidget();
        bool FoundBody = false;
        Widget->WidgetTree->ForEachWidget([&](UWidget* Child) {
            if (auto* Text = Cast<UTextBlock>(Child)) FoundBody |= Text->GetText().EqualTo(Data->mDescription);
        });
        TestTrue(TEXT("Authored text survives widget construction"), FoundBody);
        TestTrue(TEXT("Reading a trap has an explicit confirm button"), Widget->GetContinueButton()->IsVisible());
        FTextureCompilingManager::Get().FinishAllCompilation();
        FWidgetRenderer Renderer(true, true); Renderer.SetIsPrepassNeeded(true);
        for (int32 I = 0; I < 4; ++I) { Renderer.DrawWidget(Slate, FVector2D(1100, 700)); Widget->UpdateLayoutForTest(); }
        auto* Target = Renderer.DrawWidget(Slate, FVector2D(1100, 700));
        FlushRenderingCommands();
        TArray<FColor> Pixels; Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
        const FGeometry ButtonGeometry = Widget->GetContinueButton()->GetCachedGeometry();
        const FVector2D ButtonCenter = ButtonGeometry.GetAbsolutePosition() + ButtonGeometry.GetAbsoluteSize() * .5;
        TestTrue(TEXT("Confirmation remains on screen for every description"), ButtonCenter.X > 0 && ButtonCenter.X < 1100 && ButtonCenter.Y > 0 && ButtonCenter.Y < 700);
        TestTrue(TEXT("Actual trap stays clear in spotlight"), Pixels[280 * 1100 + 740].A < 30);
        TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(1100, 700, Pixels, PNG);
        TestTrue(TEXT("Save rendered notice"), FFileHelper::SaveArrayToFile(PNG, *(Folder / (FString(Name) + TEXT(".png")))));
    }
    return !HasAnyErrors();
}
