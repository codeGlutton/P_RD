#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UI/Reward/RewardSettlementWidgetBase.h"
#include "UI/Reward/RewardUIModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	struct FExpectedResultBoardTexture
	{
		const TCHAR* Name;
		int32 Width;
		int32 Height;
	};

	constexpr TCHAR ResultBoardArtRoot[] = TEXT("/Game/UI/ResultBoards/Art");
	constexpr TCHAR ResultBoardFontPath[] =
		TEXT("/Game/SVN/OutSideAsset/Fonts/F_HUD_Oswald.F_HUD_Oswald");
	constexpr FExpectedResultBoardTexture ExpectedTextures[] =
	{
		{ TEXT("T_VR_HeaderBlank_0809"), 1200, 280 },
		{ TEXT("T_VR_PanelBlank_0809"), 1600, 900 },
		{ TEXT("T_VR_TabBlank_0809"), 256, 256 },
		{ TEXT("T_VR_PortraitFrame_0809"), 512, 512 },
		{ TEXT("T_VR_ProgressTrack_0809"), 1024, 128 },
		{ TEXT("T_VR_ProgressFill_0809"), 1024, 96 },
		{ TEXT("T_VR_XPTicketBlank_0809"), 512, 192 },
		{ TEXT("T_VR_RewardCardBlank_0809"), 480, 672 },
		{ TEXT("T_UI_ButtonPrimaryBlank_0809"), 768, 224 },
		{ TEXT("T_UI_ButtonSecondaryBlank_0809"), 640, 224 },
		{ TEXT("T_DF_BoardBlank_0809"), 1600, 1280 },
		{ TEXT("T_DF_RibbonBlank_0809"), 896, 256 },
		{ TEXT("T_DF_PortraitCardBlank_0809"), 480, 560 },
	};

	struct FExpectedResultBoardPlacement
	{
		const TCHAR* WidgetClassPath;
		const TCHAR* WidgetName;
	};

	constexpr FExpectedResultBoardPlacement ExpectedPlacements[] =
	{
		{ TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C"), TEXT("HeaderBlankArt") },
		{ TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C"), TEXT("MainPanelArt") },
		{ TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C"), TEXT("StepCoinArt_1") },
		{ TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C"), TEXT("StepCoinArt_2") },
		{ TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C"), TEXT("NextButtonArt") },
		{ TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C"), TEXT("DefeatOuterFrame") },
		{ TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C"), TEXT("DefeatTitleBanner") },
		{ TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C"), TEXT("DefeatCardFrame_0") },
		{ TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C"), TEXT("DefeatCardFrame_1") },
		{ TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C"), TEXT("DefeatCardFrame_2") },
		{ TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C"), TEXT("mTitleButtonArt") },
	};

	UWidgetTree* LoadWidgetTreeArchetype(const TCHAR* ClassPath)
	{
		UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, ClassPath);
		UWidgetBlueprintGeneratedClass* GeneratedClass =
			Cast<UWidgetBlueprintGeneratedClass>(WidgetClass);
		return GeneratedClass != nullptr ? GeneratedClass->GetWidgetTreeArchetype() : nullptr;
	}

	UWorld* FindAutomationWorld()
	{
		if (GEngine == nullptr)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() != nullptr)
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	UImage* FindImageByPrefix(UWidgetTree* Tree, const TCHAR* Prefix)
	{
		UImage* Found = nullptr;
		if (Tree != nullptr)
		{
			Tree->ForEachWidget([&Found, Prefix](UWidget* Widget)
			{
				if (Found == nullptr && Widget != nullptr
					&& Widget->GetName().StartsWith(Prefix))
				{
					Found = Cast<UImage>(Widget);
				}
			});
		}
		return Found;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResultBoardArtContractTest,
	"P_RD.UI.ResultBoards.ArtContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResultBoardArtContractTest::RunTest(const FString& Parameters)
{
	bool bAllValid = true;
	for (const FExpectedResultBoardTexture& Expected : ExpectedTextures)
	{
		const FString ObjectPath = FString::Printf(
			TEXT("%s/%s.%s"), ResultBoardArtRoot, Expected.Name, Expected.Name);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!TestNotNull(*ObjectPath, Texture))
		{
			bAllValid = false;
			continue;
		}

		const FIntPoint ImportedSize = Texture->GetImportedSize();
		bAllValid &= TestEqual(
			*FString::Printf(TEXT("%s 폭"), Expected.Name),
			ImportedSize.X, Expected.Width);
		bAllValid &= TestEqual(
			*FString::Printf(TEXT("%s 높이"), Expected.Name),
			ImportedSize.Y, Expected.Height);
		bAllValid &= TestEqual(
			*FString::Printf(TEXT("%s UI 텍스처 그룹"), Expected.Name),
			Texture->LODGroup, TEnumAsByte<TextureGroup>(TEXTUREGROUP_UI));
		bAllValid &= TestEqual(
			*FString::Printf(TEXT("%s 밉 없음"), Expected.Name),
			Texture->MipGenSettings,
			TEnumAsByte<TextureMipGenSettings>(TMGS_NoMipmaps));
		bAllValid &= TestEqual(
			*FString::Printf(TEXT("%s UI 압축"), Expected.Name),
			Texture->CompressionSettings,
			TEnumAsByte<TextureCompressionSettings>(TC_EditorIcon));
		bAllValid &= TestTrue(
			*FString::Printf(TEXT("%s sRGB"), Expected.Name), Texture->SRGB);
		bAllValid &= TestTrue(
			*FString::Printf(TEXT("%s 스트리밍 비활성"), Expected.Name),
			Texture->NeverStream);
	}
	return bAllValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResultBoardVisualContractTest,
	"P_RD.UI.ResultBoards.VisualContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResultBoardVisualContractTest::RunTest(const FString& Parameters)
{
	bool bAllValid = true;
	for (const FExpectedResultBoardPlacement& Expected : ExpectedPlacements)
	{
		UWidgetTree* Tree = LoadWidgetTreeArchetype(Expected.WidgetClassPath);
		if (!TestNotNull(Expected.WidgetClassPath, Tree))
		{
			bAllValid = false;
			continue;
		}
		UImage* Image = Cast<UImage>(Tree->FindWidget(FName(Expected.WidgetName)));
		if (!TestNotNull(Expected.WidgetName, Image))
		{
			bAllValid = false;
			continue;
		}
		UTexture2D* Texture = Cast<UTexture2D>(Image->GetBrush().GetResourceObject());
		UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Image->Slot);
		if (!TestNotNull(*FString::Printf(TEXT("%s 텍스처"), Expected.WidgetName), Texture)
			|| !TestNotNull(*FString::Printf(TEXT("%s 캔버스 슬롯"), Expected.WidgetName), Slot))
		{
			bAllValid = false;
			continue;
		}
		const FVector2D Size = Slot->GetSize();
		const FIntPoint ImportedSize = Texture->GetImportedSize();
		const bool bHasImportedSize = ImportedSize.X > 0 && ImportedSize.Y > 0;
		bAllValid &= TestTrue(
			*FString::Printf(TEXT("%s 원본 크기 유효"), Expected.WidgetName),
			bHasImportedSize);
		if (bHasImportedSize == false)
		{
			continue;
		}
		const double NativeRatio =
			static_cast<double>(ImportedSize.X) / ImportedSize.Y;
		const double PlacedRatio = Size.Y > 0.0 ? Size.X / Size.Y : 0.0;
		bAllValid &= TestTrue(*FString::Printf(TEXT("%s 원본 비율 유지"), Expected.WidgetName),
			FMath::IsNearlyEqual(PlacedRatio, NativeRatio, 0.001));
	}

	for (const TCHAR* ClassPath : {
		TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C"),
		TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C") })
	{
		const bool bRewardSettlement = FCString::Strcmp(ClassPath,
			TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C")) == 0;
		UWidgetTree* Tree = LoadWidgetTreeArchetype(ClassPath);
		if (Tree == nullptr)
		{
			bAllValid = false;
			continue;
		}
		Tree->ForEachWidget([this, &bAllValid, bRewardSettlement](UWidget* Widget)
		{
			UTextBlock* Text = Cast<UTextBlock>(Widget);
			if (Text == nullptr)
			{
				return;
			}
			const UObject* FontObject = Text->GetFont().FontObject;
			bAllValid &= TestTrue(*FString::Printf(TEXT("%s 인게임 결과판 폰트"), *Text->GetName()),
				FontObject != nullptr && FontObject->GetPathName() == ResultBoardFontPath);
			bAllValid &= TestEqual(*FString::Printf(TEXT("%s Bold"), *Text->GetName()),
				Text->GetFont().TypefaceFontName, FName(TEXT("Bold")));
			if (bRewardSettlement)
			{
				bAllValid &= TestTrue(
					*FString::Printf(TEXT("%s 승리판 순백색"), *Text->GetName()),
					Text->GetColorAndOpacity().GetSpecifiedColor().Equals(
						FLinearColor::White, 0.001f));
				bAllValid &= TestTrue(
					*FString::Printf(TEXT("%s 승리판 검은 외곽선"), *Text->GetName()),
					Text->GetFont().OutlineSettings.OutlineColor.Equals(
						FLinearColor::Black, 0.001f));
				const FLinearColor Shadow = Text->GetShadowColorAndOpacity();
				bAllValid &= TestTrue(
					*FString::Printf(TEXT("%s 승리판 어두운 그림자"), *Text->GetName()),
					Shadow.R <= 0.001f && Shadow.G <= 0.001f
						&& Shadow.B <= 0.001f && Shadow.A > 0.0f);
			}
		});
	}
	return bAllValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRewardSettlementDesignerPartsContractTest,
	"P_RD.UI.ResultBoards.RewardDesignerParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardSettlementDesignerPartsContractTest::RunTest(const FString& Parameters)
{
	UWidgetTree* Tree = LoadWidgetTreeArchetype(
		TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C"));
	if (!TestNotNull(TEXT("승리 결과판 위젯 트리"), Tree))
	{
		return false;
	}

	bool bAllValid = true;
	UWidgetSwitcher* Switcher = Cast<UWidgetSwitcher>(
		Tree->FindWidget(TEXT("SettlementStepSwitcher")));
	bAllValid &= TestNotNull(TEXT("디자이너 단계 전환기"), Switcher);
	UCanvasPanel* ResultStep = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("SettlementResultStep")));
	UCanvasPanel* ChoiceStep = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("SettlementChoiceStep")));
	bAllValid &= TestNotNull(TEXT("EXP/골드 단계"), ResultStep);
	bAllValid &= TestNotNull(TEXT("3중 선택 단계"), ChoiceStep);
	if (Switcher != nullptr)
	{
		bAllValid &= TestTrue(TEXT("EXP 단계가 switcher 자식"),
			ResultStep != nullptr && ResultStep->GetParent() == Switcher);
		bAllValid &= TestTrue(TEXT("선택 단계가 switcher 자식"),
			ChoiceStep != nullptr && ChoiceStep->GetParent() == Switcher);
	}

	bAllValid &= TestNotNull(TEXT("고정 골드 아이콘"),
		Cast<UImage>(Tree->FindWidget(TEXT("SettlementGoldCoin"))));
	bAllValid &= TestNotNull(TEXT("고정 골드 텍스트"),
		Cast<UTextBlock>(Tree->FindWidget(TEXT("SettlementGoldGain"))));

	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto Name = [Index](const TCHAR* Prefix)
		{
			return FName(*FString::Printf(TEXT("%s_%d"), Prefix, Index));
		};
		UCanvasPanel* Row = Cast<UCanvasPanel>(Tree->FindWidget(Name(TEXT("SettlementExpRow"))));
		bAllValid &= TestNotNull(*FString::Printf(TEXT("EXP 행 %d"), Index), Row);
		bAllValid &= TestNotNull(*FString::Printf(TEXT("초상 프레임 %d"), Index),
			Cast<UImage>(Tree->FindWidget(Name(TEXT("SettlementPortraitPlate")))));
		UScaleBox* PortraitFit = Cast<UScaleBox>(
			Tree->FindWidget(Name(TEXT("SettlementPortraitFit"))));
		UImage* Portrait = Cast<UImage>(Tree->FindWidget(Name(TEXT("SettlementPortrait"))));
		bAllValid &= TestNotNull(*FString::Printf(TEXT("초상 비율 박스 %d"), Index), PortraitFit);
		bAllValid &= TestNotNull(*FString::Printf(TEXT("초상 이미지 %d"), Index), Portrait);
		if (PortraitFit != nullptr && Portrait != nullptr)
		{
			bAllValid &= TestTrue(*FString::Printf(TEXT("초상 부모 계약 %d"), Index),
				Portrait->GetParent() == PortraitFit);
			bAllValid &= TestEqual(*FString::Printf(TEXT("초상 ScaleToFit %d"), Index),
				PortraitFit->GetStretch(), EStretch::ScaleToFit);
		}
		bAllValid &= TestNotNull(*FString::Printf(TEXT("레벨 텍스트 %d"), Index),
			Cast<UTextBlock>(Tree->FindWidget(Name(TEXT("SettlementMercenaryLevel")))));
		bAllValid &= TestNotNull(*FString::Printf(TEXT("EXP 트랙 %d"), Index),
			Cast<UImage>(Tree->FindWidget(Name(TEXT("SettlementMercenaryTrack")))));
		UCanvasPanel* FillClip = Cast<UCanvasPanel>(
			Tree->FindWidget(Name(TEXT("SettlementMercenaryBarClip"))));
		UImage* Fill = Cast<UImage>(Tree->FindWidget(Name(TEXT("SettlementMercenaryBar"))));
		bAllValid &= TestNotNull(*FString::Printf(TEXT("EXP clip %d"), Index), FillClip);
		bAllValid &= TestNotNull(*FString::Printf(TEXT("EXP fill %d"), Index), Fill);
		if (FillClip != nullptr && Fill != nullptr)
		{
			bAllValid &= TestTrue(*FString::Printf(TEXT("EXP fill 부모 계약 %d"), Index),
				Fill->GetParent() == FillClip);
		}
		bAllValid &= TestNotNull(*FString::Printf(TEXT("EXP 수치 %d"), Index),
			Cast<UTextBlock>(Tree->FindWidget(Name(TEXT("SettlementMercenaryBarText")))));
		bAllValid &= TestNotNull(*FString::Printf(TEXT("XP 티켓 %d"), Index),
			Cast<UImage>(Tree->FindWidget(Name(TEXT("SettlementXPRibbon")))));
		bAllValid &= TestNotNull(*FString::Printf(TEXT("XP 텍스트 %d"), Index),
			Cast<UTextBlock>(Tree->FindWidget(Name(TEXT("SettlementXPText")))));

		UCanvasPanel* ChoiceMount = Cast<UCanvasPanel>(
			Tree->FindWidget(Name(TEXT("SettlementChoiceMount"))));
		bAllValid &= TestNotNull(*FString::Printf(TEXT("선택 카드 mount %d"), Index), ChoiceMount);
		bAllValid &= TestNotNull(*FString::Printf(TEXT("선택 카드 판 %d"), Index),
			Cast<UImage>(Tree->FindWidget(Name(TEXT("SettlementChoiceCard")))));
		UScaleBox* IconFit = Cast<UScaleBox>(
			Tree->FindWidget(Name(TEXT("SettlementChoiceIconFit"))));
		UImage* Icon = Cast<UImage>(Tree->FindWidget(Name(TEXT("SettlementChoiceIcon"))));
		bAllValid &= TestNotNull(*FString::Printf(TEXT("선택 아이콘 비율 박스 %d"), Index), IconFit);
		bAllValid &= TestNotNull(*FString::Printf(TEXT("선택 아이콘 %d"), Index), Icon);
		if (IconFit != nullptr && Icon != nullptr)
		{
			bAllValid &= TestTrue(*FString::Printf(TEXT("선택 아이콘 부모 계약 %d"), Index),
				Icon->GetParent() == IconFit);
			bAllValid &= TestEqual(*FString::Printf(TEXT("선택 아이콘 ScaleToFit %d"), Index),
				IconFit->GetStretch(), EStretch::ScaleToFit);
		}
		bAllValid &= TestNotNull(*FString::Printf(TEXT("선택 이름 %d"), Index),
			Cast<UTextBlock>(Tree->FindWidget(Name(TEXT("SettlementChoiceName")))));
		bAllValid &= TestNotNull(*FString::Printf(TEXT("선택 버튼 %d"), Index),
			Cast<UButton>(Tree->FindWidget(Name(TEXT("SettlementChoiceButton")))));
	}

	return bAllValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRewardSettlementDynamicAspectTest,
	"P_RD.UI.ResultBoards.DynamicAspect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardSettlementDynamicAspectTest::RunTest(const FString& Parameters)
{
	UWorld* World = FindAutomationWorld();
	if (World == nullptr)
	{
		AddInfo(TEXT("위젯 생성 월드가 없어 동적 결과판 비율 검사를 건너뜀"));
		return true;
	}

	UClass* WidgetClass = LoadClass<URewardSettlementWidgetBase>(nullptr,
		TEXT("/Game/UI/RewardSettlement/WBP_RewardSettlement_Runtime.WBP_RewardSettlement_Runtime_C"));
	URewardSettlementWidgetBase* Widget = WidgetClass != nullptr
		? CreateWidget<URewardSettlementWidgetBase>(World, WidgetClass) : nullptr;
	if (!TestNotNull(TEXT("승리 결과판 인스턴스"), Widget))
	{
		return false;
	}

	UTexture2D* WideTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/UI/ResultBoards/Art/T_VR_HeaderBlank_0809.T_VR_HeaderBlank_0809"));
	if (!TestNotNull(TEXT("비정방형 동적 이미지 검사 텍스처"), WideTexture))
	{
		return false;
	}

	URewardUIModel* Model = NewObject<URewardUIModel>(Widget);
	FRewardUI Reward;
	Reward.mGoldGained = 1;
	Reward.mExpGained = 1;
	FRewardMercenaryExpUI Mercenary;
	Mercenary.mPortrait = WideTexture;
	Mercenary.mLevel = 1;
	Mercenary.mExpAfter = 1.0f;
	Mercenary.mMaxExp = 10.0f;
	Reward.mMercenaryExp.Add(Mercenary);
	Model->SetReward(Reward);

	FRewardChoiceUI Choice;
	Choice.mChoiceIndex = 0;
	Choice.mName = FText::FromString(TEXT("비율 검사"));
	Choice.mIcon = WideTexture;
	FRewardChoiceUI Choice1 = Choice;
	Choice1.mChoiceIndex = 1;
	Choice1.mName = FText::FromString(TEXT("두 번째"));
	FRewardChoiceUI Choice2 = Choice;
	Choice2.mChoiceIndex = 2;
	Choice2.mName = FText::FromString(TEXT("세 번째"));
	Model->SetRewardChoices({ Choice, Choice1, Choice2 });

	Widget->OpenUI();
	Widget->TakeWidget();
	UWidget* PortraitBeforeBind = Widget->GetWidgetFromName(TEXT("SettlementPortrait_0"));
	UWidget* ChoiceBeforeBind = Widget->GetWidgetFromName(TEXT("SettlementChoiceIcon_0"));
	Widget->BindUIModel(Model);
	TestTrue(TEXT("모델 갱신이 초상 위젯을 재생성하지 않음"),
		PortraitBeforeBind != nullptr
			&& PortraitBeforeBind == Widget->GetWidgetFromName(TEXT("SettlementPortrait_0")));
	TestTrue(TEXT("모델 갱신이 선택 아이콘을 재생성하지 않음"),
		ChoiceBeforeBind != nullptr
			&& ChoiceBeforeBind == Widget->GetWidgetFromName(TEXT("SettlementChoiceIcon_0")));
	UWidgetSwitcher* StepSwitcher = Cast<UWidgetSwitcher>(
		Widget->GetWidgetFromName(TEXT("SettlementStepSwitcher")));
	if (TestNotNull(TEXT("런타임 단계 switcher"), StepSwitcher))
	{
		TestEqual(TEXT("처음에는 EXP 단계"), StepSwitcher->GetActiveWidgetIndex(), 0);
	}
	for (int32 RowIndex = 0; RowIndex < 3; ++RowIndex)
	{
		UWidget* Row = Widget->GetWidgetFromName(
			FName(*FString::Printf(TEXT("SettlementExpRow_%d"), RowIndex)));
		if (TestNotNull(*FString::Printf(TEXT("고정 EXP 행 %d"), RowIndex), Row))
		{
			TestEqual(*FString::Printf(TEXT("EXP 행 %d 데이터 표시"), RowIndex),
				Row->GetVisibility(), RowIndex == 0
					? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	UCanvasPanel* FillClip = Cast<UCanvasPanel>(
		Widget->GetWidgetFromName(TEXT("SettlementMercenaryBarClip_0")));
	UImage* FullFill = Cast<UImage>(
		Widget->GetWidgetFromName(TEXT("SettlementMercenaryBar_0")));
	UCanvasPanelSlot* FillClipSlot = FillClip != nullptr
		? Cast<UCanvasPanelSlot>(FillClip->Slot) : nullptr;
	UCanvasPanelSlot* FullFillSlot = FullFill != nullptr
		? Cast<UCanvasPanelSlot>(FullFill->Slot) : nullptr;
	if (TestNotNull(TEXT("EXP clip 슬롯"), FillClipSlot)
		&& TestNotNull(TEXT("EXP full fill 슬롯"), FullFillSlot))
	{
		TestTrue(TEXT("EXP clip은 full fill의 10% 폭"), FMath::IsNearlyEqual(
			FillClipSlot->GetSize().X, FullFillSlot->GetSize().X * .1f, .01f));
	}
	auto TestDynamicWhiteText = [this, Widget](const TCHAR* Phase)
	{
		int32 TextCount = 0;
		Widget->WidgetTree->ForEachWidget([this, Phase, &TextCount](UWidget* Child)
		{
			UTextBlock* Text = Cast<UTextBlock>(Child);
			if (Text == nullptr || Text->GetName().StartsWith(TEXT("Settlement")) == false)
			{
				return;
			}
			++TextCount;
			TestTrue(*FString::Printf(TEXT("%s %s 순백색"), Phase, *Text->GetName()),
				Text->GetColorAndOpacity().GetSpecifiedColor().Equals(
					FLinearColor::White, 0.001f));
			TestTrue(*FString::Printf(TEXT("%s %s 검은 외곽선"), Phase, *Text->GetName()),
				Text->GetFont().OutlineSettings.OutlineColor.Equals(
					FLinearColor::Black, 0.001f));
			const FLinearColor Shadow = Text->GetShadowColorAndOpacity();
			TestTrue(*FString::Printf(TEXT("%s %s 어두운 그림자"), Phase, *Text->GetName()),
				Shadow.R <= 0.001f && Shadow.G <= 0.001f
					&& Shadow.B <= 0.001f && Shadow.A > 0.0f);
		});
		TestTrue(*FString::Printf(TEXT("%s 동적 텍스트 생성"), Phase), TextCount > 0);
	};

	auto TestScaleToFit = [this, WideTexture](const TCHAR* Label, UImage* Image)
	{
		UScaleBox* Fit = Image != nullptr ? Cast<UScaleBox>(Image->GetParent()) : nullptr;
		if (!TestNotNull(Label, Fit))
		{
			return;
		}
		const FIntPoint ImportedSize = WideTexture->GetImportedSize();
		const FVector2D BrushSize = Image->GetBrush().ImageSize;
		const double NativeRatio = static_cast<double>(ImportedSize.X) / ImportedSize.Y;
		const double BrushRatio = BrushSize.Y > 0.0 ? BrushSize.X / BrushSize.Y : 0.0;
		TestEqual(*FString::Printf(TEXT("%s ScaleToFit"), Label),
			Fit->GetStretch(), EStretch::ScaleToFit);
		TestTrue(*FString::Printf(TEXT("%s 브러시 원본 비율 유지"), Label),
			FMath::IsNearlyEqual(BrushRatio, NativeRatio, 0.001));
	};

	TestScaleToFit(TEXT("고정 용병 초상"),
		FindImageByPrefix(Widget->WidgetTree, TEXT("SettlementPortrait_")));
	TestDynamicWhiteText(TEXT("정산 단계"));

	UButton* NextButton = Cast<UButton>(Widget->GetWidgetFromName(TEXT("mNextButton")));
	if (TestNotNull(TEXT("다음/받기 버튼"), NextButton))
	{
		TestTrue(TEXT("EXP 단계 다음 활성"), NextButton->GetIsEnabled());
		NextButton->OnClicked.Broadcast();
	}
	if (StepSwitcher != nullptr)
	{
		TestEqual(TEXT("다음 후 선택 단계"), StepSwitcher->GetActiveWidgetIndex(), 1);
	}
	TestTrue(TEXT("단계 전환이 초상 위젯을 재생성하지 않음"),
		PortraitBeforeBind == Widget->GetWidgetFromName(TEXT("SettlementPortrait_0")));
	TestTrue(TEXT("단계 전환이 선택 아이콘을 재생성하지 않음"),
		ChoiceBeforeBind == Widget->GetWidgetFromName(TEXT("SettlementChoiceIcon_0")));
	TestScaleToFit(TEXT("고정 보상 아이콘"),
		FindImageByPrefix(Widget->WidgetTree, TEXT("SettlementChoiceIcon_")));
	TestDynamicWhiteText(TEXT("선택 단계"));
	UButton* ChoiceButton1 = Cast<UButton>(
		Widget->GetWidgetFromName(TEXT("SettlementChoiceButton_1")));
	if (TestNotNull(TEXT("두 번째 선택 버튼"), ChoiceButton1)
		&& TestNotNull(TEXT("받기 버튼"), NextButton))
	{
		TestFalse(TEXT("선택 전 받기 비활성"), NextButton->GetIsEnabled());
		ChoiceButton1->OnClicked.Broadcast();
		TestTrue(TEXT("선택 후 받기 활성"), NextButton->GetIsEnabled());
		for (int32 CardIndex = 0; CardIndex < 3; ++CardIndex)
		{
			UWidget* Mount = Widget->GetWidgetFromName(
				FName(*FString::Printf(TEXT("SettlementChoiceMount_%d"), CardIndex)));
			if (TestNotNull(*FString::Printf(TEXT("선택 카드 mount %d"), CardIndex), Mount))
			{
				TestTrue(*FString::Printf(TEXT("선택 카드 opacity %d"), CardIndex),
					FMath::IsNearlyEqual(Mount->GetRenderOpacity(),
						CardIndex == 1 ? 1.f : .55f, .001f));
			}
		}
	}
	Widget->CloseUI();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsPanelLayoutContractTest,
	"P_RD.UI.Settings.LayoutContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSettingsPanelLayoutContractTest::RunTest(const FString& Parameters)
{
	UWidgetTree* Tree = LoadWidgetTreeArchetype(
		TEXT("/Game/UI/WBP_SettingsPanel.WBP_SettingsPanel_C"));
	if (!TestNotNull(TEXT("설정판 위젯 트리"), Tree))
	{
		return false;
	}

	struct FExpectedFont
	{
		const TCHAR* WidgetName;
		int32 Size;
	};
	const FExpectedFont ExpectedFonts[] = {
		{ TEXT("SettingsTitleText"), 50 },
		{ TEXT("MasterVolumeRow_Label"), 27 },
		{ TEXT("LowQualityButtonText"), 25 },
		{ TEXT("LanguageKoreanButtonText"), 25 },
		{ TEXT("BackButtonText"), 30 },
	};
	for (const FExpectedFont& Expected : ExpectedFonts)
	{
		UTextBlock* Text = Cast<UTextBlock>(Tree->FindWidget(FName(Expected.WidgetName)));
		if (!TestNotNull(Expected.WidgetName, Text))
		{
			continue;
		}
		const UObject* FontObject = Text->GetFont().FontObject;
		TestTrue(*FString::Printf(TEXT("%s 프로젝트 복합 폰트"), Expected.WidgetName),
			FontObject != nullptr && FontObject->GetPathName() == ResultBoardFontPath);
		TestEqual(*FString::Printf(TEXT("%s 정확한 크기"), Expected.WidgetName),
			Text->GetFont().Size, static_cast<float>(Expected.Size), 0.01f);
		if (UOverlaySlot* TextSlot = Cast<UOverlaySlot>(Text->Slot))
		{
			const FMargin Padding = TextSlot->GetPadding();
			TestTrue(*FString::Printf(TEXT("%s 음수 패딩 제거"), Expected.WidgetName),
				FMath::IsNearlyZero(Padding.Left)
				&& FMath::IsNearlyZero(Padding.Top)
				&& FMath::IsNearlyZero(Padding.Right)
				&& FMath::IsNearlyZero(Padding.Bottom));
			TestEqual(*FString::Printf(TEXT("%s 가로 Fill"), Expected.WidgetName),
				TextSlot->GetHorizontalAlignment(), HAlign_Fill);
			TestEqual(*FString::Printf(TEXT("%s 세로 중앙"), Expected.WidgetName),
				TextSlot->GetVerticalAlignment(), VAlign_Center);
		}
	}

	const FLinearColor ParchmentInk(0.19f, 0.065f, 0.014f, 1.f);
	for (const TCHAR* TextName : { TEXT("SettingsTitleText"),
		TEXT("MasterVolumeRow_Label"), TEXT("QualityRow_Label"),
		TEXT("StatusText") })
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Tree->FindWidget(FName(TextName))))
		{
			TestTrue(*FString::Printf(TEXT("%s 양피지 잉크색"), TextName),
				Text->GetColorAndOpacity().GetSpecifiedColor().Equals(
					ParchmentInk, 0.001f));
		}
	}

	const FLinearColor IvoryInk(1.f, .90f, .68f, 1.f);
	for (const TCHAR* TextName : { TEXT("AudioSectionHeader"),
		TEXT("BackButtonText"), TEXT("LowQualityButtonText"),
		TEXT("MediumQualityButtonText"), TEXT("HighQualityButtonText") })
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Tree->FindWidget(FName(TextName))))
		{
			TestTrue(*FString::Printf(TEXT("%s 아이보리 잉크색"), TextName),
				Text->GetColorAndOpacity().GetSpecifiedColor().Equals(
					IvoryInk, 0.001f));
		}
	}

	for (const TCHAR* DuplicateName : { TEXT("Set_fps30_text"), TEXT("Set_fps60_text") })
	{
		UTextBlock* Duplicate = Cast<UTextBlock>(Tree->FindWidget(FName(DuplicateName)));
		if (TestNotNull(DuplicateName, Duplicate))
		{
			TestEqual(*FString::Printf(TEXT("%s 중복 라벨 숨김"), DuplicateName),
				Duplicate->GetVisibility(), ESlateVisibility::Collapsed);
		}
	}

	UScaleBox* SettingsScale = Cast<UScaleBox>(Tree->FindWidget(TEXT("SettingsScaleBox")));
	UWidget* SettingsSize = Tree->FindWidget(TEXT("SettingsSizeBox"));
	UCanvasPanel* ModalCanvas = Cast<UCanvasPanel>(
		Tree->FindWidget(TEXT("SettingsModalCanvas")));
	UCanvasPanel* ContentCanvas = Cast<UCanvasPanel>(
		Tree->FindWidget(TEXT("SettingsContentCanvas")));
	if (TestNotNull(TEXT("설정 반응형 ScaleBox"), SettingsScale))
	{
		TestEqual(TEXT("설정 화면 ScaleToFit"), SettingsScale->GetStretch(),
			EStretch::ScaleToFit);
		TestEqual(TEXT("설정 화면 양방향 스케일"), SettingsScale->GetStretchDirection(),
			EStretchDirection::Both);
	}
	if (TestNotNull(TEXT("설정 1920x1080 SizeBox"), SettingsSize)
		&& TestNotNull(TEXT("설정 디자인 캔버스"), ModalCanvas))
	{
		TestTrue(TEXT("디자인 캔버스는 SizeBox 자식"),
			ModalCanvas->GetParent() == SettingsSize);
	}
	UWidget* BodyMount = Tree->FindWidget(TEXT("Set_panel_bodyMount"));
	UCanvasPanelSlot* BodyMountSlot = BodyMount != nullptr
		? Cast<UCanvasPanelSlot>(BodyMount->Slot) : nullptr;
	if (TestNotNull(TEXT("장부 본체 마운트"), BodyMountSlot))
	{
		TestTrue(TEXT("장부 본체 위치"),
			BodyMountSlot->GetPosition().Equals(FVector2D(175.f, 28.f), 0.01));
		TestTrue(TEXT("장부 본체 1570x1001"),
			BodyMountSlot->GetSize().Equals(FVector2D(1570.f, 1001.f), 0.01));
	}
	if (UImage* BookImage = Cast<UImage>(Tree->FindWidget(TEXT("Set_panel_body"))))
	{
		const UObject* BookTexture = BookImage->GetBrush().GetResourceObject();
		TestTrue(TEXT("장부 전용 BookBase 텍스처"),
			BookTexture != nullptr && BookTexture->GetPathName().Contains(
				TEXT("SettingsLedger/T_MB_SettingsLedger_BookBase")));
	}
	if (TestNotNull(TEXT("설정 기능 컨텐츠 캔버스"), ContentCanvas))
	{
		for (const TCHAR* WidgetName : {
			TEXT("SettingsTitleText_Center"), TEXT("AudioSectionHeader_Center"),
			TEXT("DisplaySectionHeader_Center"), TEXT("MasterVolumeSlider"),
			TEXT("UIVolumeSlider"), TEXT("LowQualityButtonPlateMount"),
			TEXT("FpsThirtyButtonPlateMount"), TEXT("LanguageKoreanButtonPlateMount"),
			TEXT("BackButtonPlateMount"), TEXT("RunActionsPanel") })
		{
			UWidget* Widget = Tree->FindWidget(FName(WidgetName));
			TestTrue(*FString::Printf(TEXT("%s 기능 캔버스 자식"), WidgetName),
				Widget != nullptr && Widget->GetParent() == ContentCanvas);
		}
	}

	struct FExpectedSurface
	{
		const TCHAR* Name;
		FVector2D Size;
	};
	const FExpectedSurface ExpectedSurfaces[] = {
		{ TEXT("Set_row_master_plate_Surface"), FVector2D(595.f, 78.f) },
		{ TEXT("Set_row_bgm_plate_Surface"), FVector2D(595.f, 78.f) },
		{ TEXT("Set_row_sfx_plate_Surface"), FVector2D(595.f, 78.f) },
		{ TEXT("Set_row_ui_plate_Surface"), FVector2D(595.f, 78.f) },
		{ TEXT("Set_row_shake_plate_Surface"), FVector2D(575.f, 74.f) },
		{ TEXT("Set_row_effects_plate_Surface"), FVector2D(575.f, 74.f) },
		{ TEXT("SettingsQualityRowSurface"), FVector2D(575.f, 74.f) },
		{ TEXT("SettingsFpsRowSurface"), FVector2D(575.f, 74.f) },
		{ TEXT("SettingsLanguageRowSurface"), FVector2D(575.f, 74.f) },
	};
	for (const FExpectedSurface& Expected : ExpectedSurfaces)
	{
		UBorder* Surface = Cast<UBorder>(Tree->FindWidget(FName(Expected.Name)));
		UCanvasPanelSlot* Slot = Surface != nullptr
			? Cast<UCanvasPanelSlot>(Surface->Slot) : nullptr;
		if (TestNotNull(*FString::Printf(TEXT("%s 기능 행 표면"), Expected.Name), Surface)
			&& TestNotNull(*FString::Printf(TEXT("%s 캔버스 슬롯"), Expected.Name), Slot))
		{
			TestTrue(*FString::Printf(TEXT("%s 장부 행 크기"), Expected.Name),
				Slot->GetSize().Equals(Expected.Size, 0.01));
		}
	}

	for (const TCHAR* LedgerRowName : {
		TEXT("Set_row_master_plate"), TEXT("Set_row_bgm_plate"),
		TEXT("Set_row_sfx_plate"), TEXT("Set_row_ui_plate"),
		TEXT("Set_row_shake_plate"),
		TEXT("Set_row_effects_plate"), TEXT("Set_row_quality_plate"),
		TEXT("Set_row_fps_plate"), TEXT("Set_row_language_plate") })
	{
		UImage* Plate = Cast<UImage>(Tree->FindWidget(FName(LedgerRowName)));
		if (TestNotNull(*FString::Printf(TEXT("%s 장부 라벨 파츠"), LedgerRowName), Plate))
		{
			TestEqual(*FString::Printf(TEXT("%s 표시"), LedgerRowName),
				Plate->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
			const UObject* Resource = Plate->GetBrush().GetResourceObject();
			TestTrue(*FString::Printf(TEXT("%s 전용 RowLabel"), LedgerRowName),
				Resource != nullptr && Resource->GetPathName().Contains(
					TEXT("SettingsLedger/T_MB_SettingsLedger_RowLabel")));
		}
	}

	for (const TCHAR* FillName : { TEXT("Set_slider_fill_master"),
		TEXT("Set_slider_fill_bgm"), TEXT("Set_slider_fill_sfx"),
		TEXT("Set_slider_fill_ui") })
	{
		UProgressBar* Fill = Cast<UProgressBar>(Tree->FindWidget(FName(FillName)));
		UCanvasPanelSlot* Slot = Fill != nullptr
			? Cast<UCanvasPanelSlot>(Fill->Slot) : nullptr;
		if (TestNotNull(*FString::Printf(TEXT("%s 실제 채움"), FillName), Fill)
			&& TestNotNull(*FString::Printf(TEXT("%s 캔버스 슬롯"), FillName), Slot))
		{
			TestTrue(*FString::Printf(TEXT("%s 장부 채움 영역"), FillName),
				Slot->GetSize().Equals(FVector2D(332.f, 29.f), 0.02));
		}
	}
	if (UWidget* UiSlider = Tree->FindWidget(TEXT("UIVolumeSlider")))
	{
		TestEqual(TEXT("UI 볼륨 입력 노출"), UiSlider->GetVisibility(),
			ESlateVisibility::Visible);
	}
	for (const TCHAR* VibrationName : { TEXT("VibrationRow_Label_Center"),
		TEXT("VibrationCheckBox") })
	{
		if (UWidget* VibrationWidget = Tree->FindWidget(FName(VibrationName)))
		{
			TestEqual(*FString::Printf(TEXT("%s 미구현 기능 숨김"), VibrationName),
				VibrationWidget->GetVisibility(), ESlateVisibility::Collapsed);
		}
	}

	UWidget* RunActions = Tree->FindWidget(TEXT("RunActionsPanel"));
	UCanvasPanelSlot* RunActionsSlot = RunActions != nullptr
		? Cast<UCanvasPanelSlot>(RunActions->Slot) : nullptr;
	if (TestNotNull(TEXT("런 액션 캔버스 슬롯"), RunActionsSlot))
	{
		TestTrue(TEXT("런 액션 620x118"),
			RunActionsSlot->GetSize().Equals(FVector2D(620.0, 118.0), 0.01));
	}
	for (const TCHAR* ContainerName : {
		TEXT("Set_run_SaveAndExitButton"), TEXT("Set_run_AbandonRunButton") })
	{
		UWidget* Container = Tree->FindWidget(FName(ContainerName));
		UHorizontalBoxSlot* Slot = Container != nullptr
			? Cast<UHorizontalBoxSlot>(Container->Slot) : nullptr;
		if (TestNotNull(*FString::Printf(TEXT("%s 가로 슬롯"), ContainerName), Slot))
		{
			TestEqual(*FString::Printf(TEXT("%s Fill"), ContainerName),
				Slot->GetSize().SizeRule, ESlateSizeRule::Fill);
		}
	}
	for (const TCHAR* ButtonName : { TEXT("SaveAndExitButton"), TEXT("AbandonRunButton") })
	{
		UButton* Button = Cast<UButton>(Tree->FindWidget(FName(ButtonName)));
		UOverlaySlot* Slot = Button != nullptr ? Cast<UOverlaySlot>(Button->Slot) : nullptr;
		if (TestNotNull(*FString::Printf(TEXT("%s 오버레이 슬롯"), ButtonName), Slot))
		{
			TestEqual(*FString::Printf(TEXT("%s 가로 Fill"), ButtonName),
				Slot->GetHorizontalAlignment(), HAlign_Fill);
			TestEqual(*FString::Printf(TEXT("%s 세로 Fill"), ButtonName),
				Slot->GetVerticalAlignment(), VAlign_Fill);
		}
	}

	double PreviousRight = -1.0;
	for (const TCHAR* MountName : {
		TEXT("LowQualityButtonPlateMount"), TEXT("MediumQualityButtonPlateMount"),
		TEXT("HighQualityButtonPlateMount") })
	{
		UWidget* Mount = Tree->FindWidget(FName(MountName));
		UCanvasPanelSlot* Slot = Mount != nullptr ? Cast<UCanvasPanelSlot>(Mount->Slot) : nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("%s 캔버스 슬롯"), MountName), Slot))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s 112x56"), MountName),
			Slot->GetSize().Equals(FVector2D(112.0, 56.0), 0.01));
		TestTrue(*FString::Printf(TEXT("%s 이전 버튼과 비중첩"), MountName),
			Slot->GetPosition().X >= PreviousRight - 0.01);
		PreviousRight = Slot->GetPosition().X + Slot->GetSize().X;
	}

	struct FExpectedTextFit
	{
		const TCHAR* TextName;
		const TCHAR* ContainerName;
		FMargin Padding;
	};
	const FMargin SegmentTextPadding(8.f, 2.f);
	const FMargin ActionTextPadding(14.f, 2.f, 14.f, 32.f);
	const FMargin CompactActionTextPadding(12.f, 2.f, 12.f, 22.f);
	const FExpectedTextFit ExpectedTextFits[] = {
		{ TEXT("BackButtonText"), TEXT("BackButtonText_Center"), ActionTextPadding },
		{ TEXT("ResetButtonText"), TEXT("ResetButtonText_Center"), ActionTextPadding },
		{ TEXT("LowQualityButtonText"), TEXT("LowQualityButtonText_Center"), SegmentTextPadding },
		{ TEXT("MediumQualityButtonText"), TEXT("MediumQualityButtonText_Center"), SegmentTextPadding },
		{ TEXT("HighQualityButtonText"), TEXT("HighQualityButtonText_Center"), SegmentTextPadding },
		{ TEXT("FpsThirtyButtonText"), TEXT("FpsThirtyButtonText_Center"), SegmentTextPadding },
		{ TEXT("FpsSixtyButtonText"), TEXT("FpsSixtyButtonText_Center"), SegmentTextPadding },
		{ TEXT("LanguageKoreanButtonText"), TEXT("LanguageKoreanButtonText_Center"), SegmentTextPadding },
		{ TEXT("LanguageEnglishButtonText"), TEXT("LanguageEnglishButtonText_Center"), SegmentTextPadding },
		{ TEXT("SaveAndExitButtonText"), TEXT("Set_run_SaveAndExitButton"), ActionTextPadding },
		{ TEXT("AbandonRunButtonText"), TEXT("Set_run_AbandonRunButton"), ActionTextPadding },
		{ TEXT("ConfirmAbandonButtonText"), TEXT("ConfirmAbandonButtonText_Center"), CompactActionTextPadding },
		{ TEXT("CancelAbandonButtonText"), TEXT("CancelAbandonButtonText_Center"), CompactActionTextPadding },
	};
	for (const FExpectedTextFit& Expected : ExpectedTextFits)
	{
		UTextBlock* Text = Cast<UTextBlock>(Tree->FindWidget(FName(Expected.TextName)));
		UScaleBox* Scale = Cast<UScaleBox>(Tree->FindWidget(
			FName(*(FString(Expected.TextName) + TEXT("_FitScale")))));
		UWidget* Container = Tree->FindWidget(FName(Expected.ContainerName));
		if (TestNotNull(*FString::Printf(TEXT("%s 안전 텍스트"), Expected.TextName), Text)
			&& TestNotNull(*FString::Printf(TEXT("%s 축소 래퍼"), Expected.TextName), Scale)
			&& TestNotNull(*FString::Printf(TEXT("%s 컨테이너"), Expected.TextName), Container))
		{
			TestTrue(*FString::Printf(TEXT("%s ScaleBox 자식"), Expected.TextName),
				Text->GetParent() == Scale);
			TestTrue(*FString::Printf(TEXT("%s 지정 컨테이너 안"), Expected.TextName),
				Scale->GetParent() == Container);
			TestEqual(*FString::Printf(TEXT("%s 넘침 시에만 축소"), Expected.TextName),
				Scale->GetStretchDirection(), EStretchDirection::DownOnly);
			TestEqual(*FString::Printf(TEXT("%s 경계 클립"), Expected.TextName),
				Scale->GetClipping(), EWidgetClipping::ClipToBoundsAlways);
			UOverlaySlot* ScaleSlot = Cast<UOverlaySlot>(Scale->Slot);
			if (TestNotNull(*FString::Printf(TEXT("%s 중앙 슬롯"), Expected.TextName),
				ScaleSlot))
			{
				const FMargin ActualPadding = ScaleSlot->GetPadding();
				TestTrue(*FString::Printf(TEXT("%s 버튼 면 중앙 패딩"), Expected.TextName),
					FMath::IsNearlyEqual(ActualPadding.Left, Expected.Padding.Left)
					&& FMath::IsNearlyEqual(ActualPadding.Top, Expected.Padding.Top)
					&& FMath::IsNearlyEqual(ActualPadding.Right, Expected.Padding.Right)
					&& FMath::IsNearlyEqual(ActualPadding.Bottom, Expected.Padding.Bottom));
				TestEqual(*FString::Printf(TEXT("%s 가로 Fill"), Expected.TextName),
					ScaleSlot->GetHorizontalAlignment(), HAlign_Fill);
				TestEqual(*FString::Printf(TEXT("%s 세로 Fill"), Expected.TextName),
					ScaleSlot->GetVerticalAlignment(), VAlign_Fill);
			}
			if (UScaleBoxSlot* TextSlot = Cast<UScaleBoxSlot>(Text->Slot))
			{
				TestEqual(*FString::Printf(TEXT("%s 글자 가로 중앙"), Expected.TextName),
					TextSlot->GetHorizontalAlignment(), HAlign_Center);
				TestEqual(*FString::Printf(TEXT("%s 글자 세로 중앙"), Expected.TextName),
					TextSlot->GetVerticalAlignment(), VAlign_Center);
			}
			TestTrue(*FString::Printf(TEXT("%s 레거시 렌더 오프셋 제거"), Expected.TextName),
				Text->GetRenderTransform() == FWidgetTransform());
		}
	}

	// A button's functional mount may be wider than its source art. The transparent
	// UButton fills that mount, while the plate must be centered at one uniform scale.
	auto TestAspectFitPlate = [this, Tree](const TCHAR* PlateName,
		const FVector2D ExplicitBounds = FVector2D::ZeroVector)
	{
		UImage* Plate = Cast<UImage>(Tree->FindWidget(FName(PlateName)));
		if (!TestNotNull(*FString::Printf(TEXT("%s 이미지"), PlateName), Plate))
		{
			return;
		}

		const FSlateBrush& Brush = Plate->GetBrush();
		UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject());
		UOverlaySlot* PlateSlot = Cast<UOverlaySlot>(Plate->Slot);
		if (!TestNotNull(*FString::Printf(TEXT("%s 텍스처"), PlateName), Texture)
			|| !TestNotNull(*FString::Printf(TEXT("%s 오버레이 슬롯"), PlateName), PlateSlot))
		{
			return;
		}

		FVector2D Bounds = ExplicitBounds;
		if (Bounds.IsNearlyZero())
		{
			const UWidget* Mount = Plate->GetParent();
			const UCanvasPanelSlot* MountSlot = Mount != nullptr
				? Cast<UCanvasPanelSlot>(Mount->Slot) : nullptr;
			if (!TestNotNull(*FString::Printf(TEXT("%s 마운트 슬롯"), PlateName),
				MountSlot))
			{
				return;
			}
			Bounds = MountSlot->GetSize();
		}

		const FIntPoint NativeSize = Texture->GetImportedSize();
		const FVector2D ArtSize = Brush.GetImageSize();
		const double NativeRatio = NativeSize.Y > 0
			? static_cast<double>(NativeSize.X) / NativeSize.Y : 0.0;
		const double ArtRatio = ArtSize.Y > 0.0 ? ArtSize.X / ArtSize.Y : 0.0;
		TestTrue(*FString::Printf(TEXT("%s 원본 비율"), PlateName),
			FMath::IsNearlyEqual(ArtRatio, NativeRatio, 0.001));
		TestTrue(*FString::Printf(TEXT("%s 마운트 안에 fit"), PlateName),
			ArtSize.X <= Bounds.X + 0.01 && ArtSize.Y <= Bounds.Y + 0.01);
		TestTrue(*FString::Printf(TEXT("%s 비변형 Image 브러시"), PlateName),
			Brush.DrawAs == ESlateBrushDrawType::Image);
		TestEqual(*FString::Printf(TEXT("%s 가로 중앙"), PlateName),
			PlateSlot->GetHorizontalAlignment(), HAlign_Center);
		TestEqual(*FString::Printf(TEXT("%s 세로 중앙"), PlateName),
			PlateSlot->GetVerticalAlignment(), VAlign_Center);
	};

	for (const TCHAR* PlateName : {
		TEXT("BackButtonPlate"), TEXT("ResetButtonPlate"),
		TEXT("FpsThirtyButtonPlate"), TEXT("FpsSixtyButtonPlate"),
		TEXT("LowQualityButtonPlate"), TEXT("MediumQualityButtonPlate"),
		TEXT("HighQualityButtonPlate"), TEXT("LanguageKoreanButtonPlate"),
		TEXT("LanguageEnglishButtonPlate"), TEXT("ConfirmAbandonButtonPlate"),
		TEXT("CancelAbandonButtonPlate") })
	{
		TestAspectFitPlate(PlateName);
	}
	TestAspectFitPlate(TEXT("SaveAndExitButtonPlate"), FVector2D(310.0, 118.0));
	TestAspectFitPlate(TEXT("AbandonRunButtonPlate"), FVector2D(310.0, 118.0));

	// The generated ledger is fixed illustration art: the center spine, rivets and
	// parchment labels must never be 9-sliced. Only the confirmation panel remains a box.
	for (const TCHAR* ImageName : {
		TEXT("Set_panel_body"),
		TEXT("Set_row_master_plate"), TEXT("Set_row_bgm_plate"),
		TEXT("Set_row_sfx_plate"), TEXT("Set_row_ui_plate"),
		TEXT("Set_row_shake_plate"),
		TEXT("Set_row_effects_plate"), TEXT("Set_row_quality_plate"),
		TEXT("Set_row_fps_plate"), TEXT("Set_row_language_plate") })
	{
		UImage* Image = Cast<UImage>(Tree->FindWidget(FName(ImageName)));
		if (!TestNotNull(*FString::Printf(TEXT("%s 고정 이미지"), ImageName), Image))
		{
			continue;
		}
		const FSlateBrush& Brush = Image->GetBrush();
		UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject());
		if (!TestNotNull(*FString::Printf(TEXT("%s 텍스처"), ImageName), Texture))
		{
			continue;
		}
		const FIntPoint NativeSize = Texture->GetImportedSize();
		const FVector2D BasisSize = Brush.GetImageSize();
		TestTrue(*FString::Printf(TEXT("%s Image 브러시"), ImageName),
			Brush.DrawAs == ESlateBrushDrawType::Image);
		TestTrue(*FString::Printf(TEXT("%s 네이티브 이미지 기준"), ImageName),
			BasisSize.Equals(FVector2D(NativeSize.X, NativeSize.Y), 0.01));
		TestTrue(*FString::Printf(TEXT("%s 9-slice 없음"), ImageName),
			Brush.GetMargin() == FMargin(0.f));
	}

	if (UImage* ConfirmPlate = Cast<UImage>(Tree->FindWidget(TEXT("Set_confirm_plate"))))
	{
		TestEqual(TEXT("확인 모달만 9-slice"), ConfirmPlate->GetBrush().DrawAs,
			ESlateBrushDrawType::Box);
	}

	for (const TCHAR* RibbonName : { TEXT("SettingsAudioRibbonArt"),
		TEXT("SettingsDisplayRibbonArt"), TEXT("SettingsGameplayRibbonArt") })
	{
		UImage* Ribbon = Cast<UImage>(Tree->FindWidget(FName(RibbonName)));
		if (TestNotNull(*FString::Printf(TEXT("%s 장부 리본"), RibbonName), Ribbon))
		{
			const UObject* Resource = Ribbon->GetBrush().GetResourceObject();
			TestTrue(*FString::Printf(TEXT("%s 전용 텍스처"), RibbonName),
				Resource != nullptr && Resource->GetPathName().Contains(
					TEXT("SettingsLedger/T_MB_SettingsLedger_SectionRibbon")));
		}
	}
	UImage* SelectedReference = Cast<UImage>(
		Tree->FindWidget(TEXT("SettingsChoiceSelectedCookReference")));
	if (TestNotNull(TEXT("선택 파츠 cook 참조"), SelectedReference))
	{
		const UObject* Resource = SelectedReference->GetBrush().GetResourceObject();
		TestEqual(TEXT("선택 파츠 cook 참조 숨김"),
			SelectedReference->GetVisibility(), ESlateVisibility::Collapsed);
		TestTrue(TEXT("선택 파츠 전용 텍스처"),
			Resource != nullptr && Resource->GetPathName().Contains(
				TEXT("SettingsLedger/T_MB_SettingsLedger_ChoiceButton_Selected")));
	}

	for (const TCHAR* TrackName : { TEXT("Set_slider_track_master"),
		TEXT("Set_slider_track_bgm"), TEXT("Set_slider_track_sfx"),
		TEXT("Set_slider_track_ui") })
	{
		UImage* Track = Cast<UImage>(Tree->FindWidget(FName(TrackName)));
		UCanvasPanelSlot* Slot = Track != nullptr
			? Cast<UCanvasPanelSlot>(Track->Slot) : nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("%s 트랙"), TrackName), Track)
			|| !TestNotNull(*FString::Printf(TEXT("%s 트랙 슬롯"), TrackName), Slot))
		{
			continue;
		}
		const FSlateBrush& Brush = Track->GetBrush();
		UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject());
		if (!TestNotNull(*FString::Printf(TEXT("%s 트랙 텍스처"), TrackName), Texture))
		{
			continue;
		}
		const FIntPoint NativeSize = Texture->GetImportedSize();
		const FVector2D DrawSize = Slot->GetSize();
		const double NativeRatio = static_cast<double>(NativeSize.X) / NativeSize.Y;
		const double DrawRatio = DrawSize.X / DrawSize.Y;
		TestEqual(*FString::Printf(TEXT("%s 비변형 Image"), TrackName),
			Brush.DrawAs, ESlateBrushDrawType::Image);
		TestTrue(*FString::Printf(TEXT("%s 원본 비율"), TrackName),
			FMath::IsNearlyEqual(NativeRatio, DrawRatio, 0.01));
	}
	return true;
}

#endif
