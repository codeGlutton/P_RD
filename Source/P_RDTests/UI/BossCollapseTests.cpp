#include "Misc/AutomationTest.h"
#include "UI/StageVictory/BossCollapseWidget.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "RHI.h"
#include "Widgets/Layout/SDPIScaler.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossCollapseFlowTest,"P_RD.UI.BossCollapse.Flow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBossCollapseFlowTest::RunTest(const FString&)
{
    TestFalse(TEXT("Ordinary victory bypasses boss comic"),UBossCollapseWidget::ShouldPlay(true,false,1));
    TestFalse(TEXT("Boss defeat bypasses victory comic"),UBossCollapseWidget::ShouldPlay(false,true,3));
    TestFalse(TEXT("Missing stage has no fabricated boss"),UBossCollapseWidget::ShouldPlay(true,true,0));
    TestFalse(TEXT("Unsupported stage rejected"),UBossCollapseWidget::ShouldPlay(true,true,4));
    for (int32 Stage=1;Stage<=3;++Stage)
    {
        auto* Widget=CreateWidget<UBossCollapseWidget>(GEditor->GetEditorWorldContext().World());
        if (!TestTrue(TEXT("Stage atlas and sound load"),Widget->LoadStage(Stage))) return false;
        TestEqual(TEXT("Four separate source cuts"),Widget->Panels.Num(),4);
        const FBox2f FinalUV=Widget->Panels[3].GetUVRegion();
        TestTrue(TEXT("Cold load selects the whole final cell, not a placeholder-sized closeup"),FinalUV.GetSize().X>.45f && FinalUV.GetSize().Y>.45f);
        int32 Calls=0;
        Widget->Finished=FSimpleDelegate::CreateLambda([&Calls](){++Calls;});
        Widget->NativeOnTouchStarted(FGeometry(),FPointerEvent());
        TestEqual(TEXT("Touch cannot skip the comic or claim rewards"),Calls,0);
        Widget->Advance(Widget->GetDuration()-.1f);
        TestEqual(TEXT("Rewards wait for the final frame"),Calls,0);
        Widget->Advance(.2f);Widget->Advance(10.f);
        TestEqual(TEXT("Reward continuation fires exactly once"),Calls,1);
        auto* Cancelled=CreateWidget<UBossCollapseWidget>(GEditor->GetEditorWorldContext().World());
        Cancelled->LoadStage(Stage);
        Cancelled->Finished=FSimpleDelegate::CreateLambda([&Calls](){++Calls;});
        Cancelled->Cancel();Cancelled->Advance(10.f);
        TestEqual(TEXT("Closing world does not open stale rewards"),Calls,1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossCollapseRenderTest,"P_RD.UI.BossCollapse.Render",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBossCollapseRenderTest::RunTest(const FString&)
{
    if (GUsingNullRHI) { AddWarning(TEXT("Render check requires RHI")); return true; }
    const FString Dir=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("UI/BossCollapse"));
    IFileManager::Get().MakeDirectory(*Dir,true);
    for(int32 Stage=1;Stage<=3;++Stage)
    {
        auto* Widget=CreateWidget<UBossCollapseWidget>(GEditor->GetEditorWorldContext().World());
        if(!TestTrue(TEXT("Source art loads"),Widget->LoadStage(Stage)))return false;
        Widget->TakeWidget();
        FAssetCompilingManager::Get().FinishAllCompilation();
        if(GShaderCompilingManager)GShaderCompilingManager->FinishAllCompilation();
        for(int32 Beat=0;Beat<3;++Beat)
        {
            Widget->Elapsed=Beat==0?.85f:Beat==1?(Stage==1?1.4f:2.f):Widget->GetDuration()-.3f;
            Widget->bFinished=true; // Freeze render time; offscreen Slate ticks cannot run the callback.
            for(const FIntPoint Size:{FIntPoint(1280,720),FIntPoint(1560,720)})
            {
                Widget->ForceLayoutPrepass();
                FWidgetRenderer Renderer(true,true);
                auto* Target=Renderer.DrawWidget(Widget->TakeWidget(),FVector2D(Size.X,Size.Y));
                FlushRenderingCommands();TArray<FColor> Pixels;
                if(!TestTrue(TEXT("Rendered pixels available"),Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels)))return false;
                int32 Visible=0;for(auto C:Pixels)if(FMath::Max3(C.R,C.G,C.B)>30)++Visible;
                TestTrue(TEXT("Comic renders rather than a black viewport"),Visible>Pixels.Num()/3);
                TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,Png);
                TestTrue(TEXT("Capture saved"),FFileHelper::SaveArrayToFile(Png,*FPaths::Combine(Dir,FString::Printf(TEXT("stage%d_beat%d_%dx%d.png"),Stage,Beat,Size.X,Size.Y))));
                if(Beat==2 && Size.X==1280)
                {
                    auto Scaled=SNew(SDPIScaler).DPIScale(2.f/3.f)[Widget->TakeWidget()];
                    auto* DpiTarget=Renderer.DrawWidget(Scaled,FVector2D(1280,720));
                    FlushRenderingCommands();TArray<FColor> DpiPixels;
                    DpiTarget->GameThread_GetRenderTargetResource()->ReadPixels(DpiPixels);
                    FImageUtils::PNGCompressImageArray(1280,720,DpiPixels,Png);
                    FFileHelper::SaveArrayToFile(Png,*FPaths::Combine(Dir,FString::Printf(TEXT("stage%d_dpi.png"),Stage)));
                }
            }
        }
    }
    return true;
}
#endif
