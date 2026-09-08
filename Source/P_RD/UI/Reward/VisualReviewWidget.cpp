#include "UI/Reward/VisualReviewWidget.h"
#include "UI/Combat/SkillCutInWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/IConsoleManager.h"

bool UVisualReviewWidget::Initialize()
{
	if (!Super::Initialize()) return false;
	if (WidgetTree->RootWidget) return true;
	auto* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;
	auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	auto* RowSlot = Root->AddChildToCanvas(Row);
	RowSlot->SetPosition(FVector2D(24,24)); RowSlot->SetAutoSize(true);
	const TCHAR* Labels[] = {TEXT("상자 다시 재생"), TEXT("아군 컷씬"), TEXT("몬스터 컷씬")};
	for (int32 i=0; i<3; ++i)
	{
		auto* Button = WidgetTree->ConstructWidget<UButton>();
		auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Labels[i]));
		FSlateFontInfo Font = Text->GetFont(); Font.Size = 18; Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Button->SetBackgroundColor(FLinearColor(.08f,.12f,.2f,1));
		Button->AddChild(Text); Row->AddChild(Button);
		if (i==0) Button->OnClicked.AddDynamic(this, &UVisualReviewWidget::ReplayChest);
		if (i==1) Button->OnClicked.AddDynamic(this, &UVisualReviewWidget::PlayAlly);
		if (i==2) Button->OnClicked.AddDynamic(this, &UVisualReviewWidget::PlayMonster);
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	return true;
}
void UVisualReviewWidget::ReplayChest()
{
	if (CutIn) CutIn->StopCutIn();
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("RD.RewardConcept03Preview.Auto"));
}
void UVisualReviewWidget::PlayAlly() { PlayCutIn(true); }
void UVisualReviewWidget::PlayMonster() { PlayCutIn(false); }
void UVisualReviewWidget::PlayCutIn(bool Ally)
{
	if (!CutIn) { CutIn = CreateWidget<USkillCutInWidget>(GetWorld()); CutIn->AddToViewport(20000); }
	FSkillCutInPresentationData P;
	P.LayerRig = ESkillCutInLayerRig::MasterDuelSingle;
	P.bMirror = Ally; P.DurationSeconds = .82f; P.FailSafeSeconds = 1.12f;
	const FString Base(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/SkillCutIn/Generated/"));
	auto Tex = [&Base](const TCHAR* Name) { return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Base + Name + TEXT(".") + Name)); };
	P.BodyTexture = Tex(Ally ? TEXT("T_SkillCutIn_MasterDuelSingle_Knight_1672x941_v1") : TEXT("T_SkillCutIn_MasterDuelMonster_Character_v2"));
	P.BackgroundTexture = Tex(Ally ? TEXT("T_SkillCutIn_Mercenary_BrushBG_v5") : TEXT("T_SkillCutIn_Monster_BrushBG_v5"));
	P.SpeedLinesTexture = Tex(Ally ? TEXT("T_SkillCutIn_MasterDuelSingle_SpeedFX_v3") : TEXT("T_SkillCutIn_MasterDuelMonster_SpeedFX_v1"));
	P.ForegroundTexture = Tex(Ally ? TEXT("T_SkillCutIn_MasterDuelSingle_ImpactFX_v3") : TEXT("T_SkillCutIn_MasterDuelMonster_ImpactFX_v1"));
	CutIn->PlayCutIn(P);
}
void UVisualReviewWidget::NativeDestruct()
{
	if (CutIn) { CutIn->StopCutIn(); CutIn->RemoveFromParent(); CutIn = nullptr; }
	Super::NativeDestruct();
}
#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorld ReviewCommand(TEXT("RD.VisualReview"), TEXT("Open chest and cut-in review controls."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (!World || !World->IsGameWorld()) return;
		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("RD.RewardConcept03Preview"));
		auto* Review = CreateWidget<UVisualReviewWidget>(World); Review->AddToViewport(30000);
	}));
#endif
