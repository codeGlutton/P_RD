/*****************************************************************//**
 * @file   ShopFullGeneratedCaptureTests.cpp
 * @brief  전용 생성 아트 상점 WBP의 실제 렌더와 기본 상호작용을 검증한다.
 * @details
 * 기존 WBP 캡처 테스트와 상태를 공유하지 않고, 새 WBP에 합성 상점 모델을
 * 연결해 아티팩트/스킬/휴식 화면을 Fold 실기기와 같은 2176x1812 PNG로 남긴다.
 * @date   2026-08-12
 *********************************************************************/

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "TextureCompiler.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/SlateTypes.h"
#include "UI/Shop/ShopFullGeneratedWidgetBase.h"
#include "UI/Shop/ShopUIModel.h"
#include "UI/Hire/MercenaryHireWidget.h"
#include "UI/ShopFullGeneratedWidgetBuilder.h"
#include "UI/RunOptionsRailWidget.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/SOverlay.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace ShopFullGeneratedCaptureTests
{
	constexpr int32 CaptureWidth = 2176;
	constexpr int32 CaptureHeight = 1812;
	constexpr TCHAR WidgetClassPath[] =
		TEXT("/Game/UI/Shop/WBP_Shop_FullGenerated.WBP_Shop_FullGenerated_C");

	FString OutputDirectory()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UI"),
			TEXT("ShopFullGenerated"));
	}

	UTexture2D* LoadTexture(const TCHAR* ObjectPath)
	{
		return LoadObject<UTexture2D>(nullptr, ObjectPath);
	}

	/** 실제 DTO 아이콘을 사용하는, 화면 네 종류를 모두 채울 수 있는 모델. */
	FShopUI MakeSyntheticShop()
	{
		const TCHAR* ArtifactPaths[] = {
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_LuckyCoin.T_Artifact_LuckyCoin"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_FangAmulet.T_Artifact_FangAmulet"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_BloodChalice.T_Artifact_BloodChalice"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_TravelersMap.T_Artifact_TravelersMap"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_ThornCrest.T_Artifact_ThornCrest")
		};
		const TCHAR* SkillPaths[] = {
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Barrier.T_SkillIcon_Barrier"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Slash.T_SkillIcon_Slash"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Whirlwind.T_SkillIcon_Whirlwind"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_HeavySmash.T_SkillIcon_HeavySmash"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Charge.T_SkillIcon_Charge")
		};
		const TCHAR* ArtifactNames[] = {
			TEXT("행운의 동전"), TEXT("야수의 부적"), TEXT("피의 성배"),
			TEXT("여행자의 지도"), TEXT("가시 문장")
		};
		const TCHAR* SkillNames[] = {
			TEXT("수호 방벽"), TEXT("회전 베기"), TEXT("칼날 폭풍"),
			TEXT("강타"), TEXT("돌진")
		};

		FShopUI Shop;
		Shop.mGold = 840;
		for (int32 Index = 0; Index < 5; ++Index)
		{
			FShopItemUI& Artifact = Shop.mItems.AddDefaulted_GetRef();
			Artifact.mSlotIndex = Index;
			Artifact.mKind = EShopItemKind::Artifact;
			Artifact.mName = FText::FromString(ArtifactNames[Index]);
			Artifact.mIcon = LoadTexture(ArtifactPaths[Index]);
			Artifact.mDescription = FText::FromString(FString::Printf(
				TEXT("파티 전원에게 적용되는 아티팩트 효과 %d"), Index + 1));
			Artifact.mPrice = 60 + Index * 15;
			Artifact.mRarityColor = Index == 2
				? FLinearColor(0.28f, 0.82f, 1.f, 1.f)
				: FLinearColor(1.f, 0.88f, 0.62f, 1.f);
			Artifact.mIsAffordable = true;
			Artifact.mIsSoldOut = Index == 4;

			FShopItemUI& Skill = Shop.mItems.AddDefaulted_GetRef();
			Skill.mSlotIndex = 100 + Index;
			Skill.mKind = EShopItemKind::Skill;
			Skill.mRequiredJobType = EUnitJobType::Common;
			Skill.mName = FText::FromString(SkillNames[Index]);
			Skill.mIcon = LoadTexture(SkillPaths[Index]);
			Skill.mDescription = FText::FromString(FString::Printf(
				TEXT("선택한 용병의 %d번 슬롯에 장착할 전투 기술"), Index + 1));
			Skill.mPrice = 100 + Index * 20;
			Skill.mRarityColor = Index == 2
				? FLinearColor(1.f, 0.62f, 0.20f, 1.f)
				: FLinearColor(0.70f, 0.88f, 1.f, 1.f);
			Skill.mIsAffordable = true;
			Skill.mIsSoldOut = false;
		}

		const EUnitJobType CandidateJobs[] = {
			EUnitJobType::Knight, EUnitJobType::Mage, EUnitJobType::Rogue
		};
		const TCHAR* CandidateNames[] = { TEXT("기사"), TEXT("마법사"), TEXT("도적") };
		const TCHAR* CandidateRoles[] = {
			TEXT("방패 탱커 · 근접"), TEXT("주문 술사 · 원거리"), TEXT("기습 암살자 · 근접")
		};
		const TCHAR* CandidateIcons[] = {
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue")
		};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			FShopMercenaryUI& Candidate =
				Shop.mMercenaries.AddDefaulted_GetRef();
			Candidate.mSlotIndex = 200 + Index;
			Candidate.mLevel = 3 + Index;
			Candidate.mPrice = 180 + Index * 40;
			Candidate.mIsAffordable = true;
			Candidate.mCharacter.mIndex = Index;
			Candidate.mCharacter.mDisplayName =
				FText::FromString(CandidateNames[Index]);
			Candidate.mCharacter.mRoleText =
				FText::FromString(CandidateRoles[Index]);
			Candidate.mCharacter.mRoleShort = FText::Format(
				FText::FromString(TEXT("Lv.{0}")),
				FText::AsNumber(Candidate.mLevel));
			Candidate.mCharacter.mDescription = FText::FromString(
				TEXT("상점에서 현재 파티원과 교체해 고용할 수 있는 용병입니다."));
			Candidate.mCharacter.mJobType = CandidateJobs[Index];
			Candidate.mCharacter.mMaxHP = 100 - Index * 12;
			Candidate.mCharacter.mMaxAP = 7 + Index;
			Candidate.mCharacter.mSpeed = 3 + Index;
			Candidate.mCharacter.mIcon = LoadTexture(CandidateIcons[Index]);
			Candidate.mCharacter.mSelectable = true;
		}

		for (int32 Index = 0; Index < 3; ++Index)
		{
			FShopOwnedArtifactUI& Owned =
				Shop.mOwnedArtifacts.AddDefaulted_GetRef();
			Owned.mArtifactIndex = Index;
			Owned.mName = FText::FromString(ArtifactNames[Index]);
			Owned.mIcon = LoadTexture(ArtifactPaths[Index]);
			Owned.mRarityColor = FLinearColor(0.75f, 0.90f, 1.f, 1.f);
		}

		const EUnitJobType Jobs[] = {
			EUnitJobType::Knight, EUnitJobType::Mage, EUnitJobType::Rogue
		};
		for (int32 UnitIndex = 0; UnitIndex < 3; ++UnitIndex)
		{
			FShopOwnedUnitUI& Unit = Shop.mOwnedUnits.AddDefaulted_GetRef();
			Unit.mUnitIndex = UnitIndex;
			Unit.mJobType = Jobs[UnitIndex];
			Unit.mLevel = 4 + UnitIndex;
			for (int32 SlotIndex = 0; SlotIndex < 6; ++SlotIndex)
			{
				const int32 SkillIndex = (UnitIndex + SlotIndex) % 5;
				FShopOwnedSkillSlotUI& Slot =
					Unit.mSkillSlots.AddDefaulted_GetRef();
				Slot.mIsEmpty = false;
				Slot.mName = FText::FromString(SkillNames[SkillIndex]);
				Slot.mDescription = FText::FromString(FString::Printf(
					TEXT("보유 스킬 설명 %d"), SkillIndex + 1));
				Slot.mIcon = LoadTexture(SkillPaths[SkillIndex]);
			}
			FShopMercenaryPartySlotUI& HireSlot =
				Shop.mMercenaryPartySlots.AddDefaulted_GetRef();
			HireSlot.mUnitIndex = UnitIndex;
			HireSlot.mIsOccupied = true;
			HireSlot.mJobType = Jobs[UnitIndex];
			HireSlot.mLevel = Unit.mLevel;
		}

		Shop.mRest.mPrice = 100;
		Shop.mRest.mIsAffordable = true;
		Shop.mRest.mIsUsed = false;
		const float CurrentHP[] = { 42.f, 31.f, 58.f };
		const float CurrentAP[] = { 6.f, 5.f, 8.f };
		for (int32 UnitIndex = 0; UnitIndex < 3; ++UnitIndex)
		{
			FShopRestUnitUI& RestUnit = Shop.mRest.mUnits.AddDefaulted_GetRef();
			RestUnit.mUnitIndex = UnitIndex;
			RestUnit.mJobType = Jobs[UnitIndex];
			RestUnit.mHP = CurrentHP[UnitIndex];
			RestUnit.mMaxHP = 100.f;
			RestUnit.mAP = CurrentAP[UnitIndex];
			RestUnit.mMaxAP = 12.f;
		}
		return Shop;
	}

	/** 첫 draw 전에 정적 생성 아트와 DTO 아이콘을 모두 resident로 만든다. */
	int32 MakeBrushTexturesResident(UShopUIWidgetBase& ShopWidget)
	{
		if (ShopWidget.WidgetTree == nullptr)
		{
			return 0;
		}

		TSet<UTexture2D*> Textures;
		auto CollectBrush = [&Textures](const FSlateBrush& Brush)
		{
			if (UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject()))
			{
				Textures.Add(Texture);
			}
		};
		auto CollectStyle = [&CollectBrush](const FSlateWidgetStyle& Style)
		{
			TArray<const FSlateBrush*> Brushes;
			Style.GetResources(Brushes);
			for (const FSlateBrush* Brush : Brushes)
			{
				if (Brush != nullptr)
				{
					CollectBrush(*Brush);
				}
			}
		};

		ShopWidget.WidgetTree->ForEachWidget(
			[&CollectBrush, &CollectStyle](UWidget* Widget)
			{
				if (const UImage* Image = Cast<UImage>(Widget))
				{
					CollectBrush(Image->GetBrush());
				}
				else if (const UBorder* Border = Cast<UBorder>(Widget))
				{
					CollectBrush(Border->Background);
				}
				else if (const UButton* Button = Cast<UButton>(Widget))
				{
					CollectStyle(Button->GetStyle());
				}
			});

		for (UTexture2D* Texture : Textures)
		{
			FTextureCompilingManager::Get().FinishCompilation({ Texture });
			if (Texture->GetResource() == nullptr)
			{
				Texture->UpdateResource();
			}
			Texture->SetForceMipLevelsToBeResident(30.f);
			Texture->WaitForStreaming();
		}
		FlushRenderingCommands();
		return Textures.Num();
	}

	bool Capture(UShopUIWidgetBase& ShopWidget,
		const TSharedRef<SWidget>& ShopSlate, const TCHAR* FileName,
		TArray<FColor>& OutPixels, FString& OutError)
	{
		ShopWidget.SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ShopWidget.ForceLayoutPrepass();
		const int32 TextureCount = MakeBrushTexturesResident(ShopWidget);
		if (TextureCount == 0)
		{
			OutError = TEXT("새 상점 WBP 브러시에 Texture2D가 하나도 없음");
			return false;
		}

		const TSharedRef<SOverlay> CaptureRoot =
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SColorBlock).Color(FLinearColor(0.008f, 0.012f, 0.018f, 1.f))
			]
			+ SOverlay::Slot()
			[
				ShopSlate
			];
		FWidgetRenderer Renderer(true, true);
		Renderer.SetIsPrepassNeeded(true);
		// 처음 로드한 Slate brush와 Texture2D resource가 각각 한 프레임씩
		// 늦게 준비될 수 있으므로 캡처 전 두 번 그려 첫 화면도 안정화한다.
		for (int32 WarmupFrame = 0; WarmupFrame < 6; ++WarmupFrame)
		{
			Renderer.DrawWidget(
				CaptureRoot, FVector2D(CaptureWidth, CaptureHeight));
			FlushRenderingCommands();
		}
		UTextureRenderTarget2D* RenderTarget = Renderer.DrawWidget(
			CaptureRoot, FVector2D(CaptureWidth, CaptureHeight));
		if (RenderTarget == nullptr)
		{
			OutError = TEXT("FWidgetRenderer가 새 상점 렌더 타깃을 만들지 못함");
			return false;
		}

		FlushRenderingCommands();
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(false);
		if (!RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(
				OutPixels, ReadFlags)
			|| OutPixels.Num() != CaptureWidth * CaptureHeight)
		{
			OutError = TEXT("새 상점 2176x1812 렌더 결과를 읽지 못함");
			return false;
		}

		int32 PixelsDifferentFromFirst = 0;
		uint8 MinLuma = 255;
		uint8 MaxLuma = 0;
		const FColor FirstPixel = OutPixels[0];
		for (FColor& Pixel : OutPixels)
		{
			// RGBA8 WidgetRenderer 경로의 중복 감마를 한 번 되돌린다.
			Pixel.R = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
			Pixel.G = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
			Pixel.B = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));

			const uint8 Luma = uint8((54 * int32(Pixel.R)
				+ 183 * int32(Pixel.G) + 19 * int32(Pixel.B)) >> 8);
			MinLuma = FMath::Min(MinLuma, Luma);
			MaxLuma = FMath::Max(MaxLuma, Luma);
			if (FMath::Abs(int32(Pixel.R) - int32(FirstPixel.R))
				+ FMath::Abs(int32(Pixel.G) - int32(FirstPixel.G))
				+ FMath::Abs(int32(Pixel.B) - int32(FirstPixel.B)) > 12)
			{
				++PixelsDifferentFromFirst;
			}
		}
		if (int32(MaxLuma) - int32(MinLuma) < 16
			|| PixelsDifferentFromFirst < CaptureWidth * CaptureHeight / 100)
		{
			OutError = TEXT("새 상점 캡처가 단색이다. WBP가 그려지지 않음");
			return false;
		}

		TArray64<uint8> PngData;
		FImageUtils::PNGCompressImageArray(
			CaptureWidth, CaptureHeight, OutPixels, PngData);
		IFileManager::Get().MakeDirectory(*OutputDirectory(), true);
		const FString OutputPath = FPaths::Combine(OutputDirectory(), FileName);
		if (!FFileHelper::SaveArrayToFile(PngData, *OutputPath))
		{
			OutError = FString::Printf(TEXT("새 상점 캡처 저장 실패: %s"),
				*OutputPath);
			return false;
		}

		UE_LOG(LogTemp, Display,
			TEXT("[ShopFullGeneratedUI] %s (%d textures) -> %s"),
			FileName, TextureCount, *OutputPath);
		return true;
	}

	int32 CountChangedPixels(const TArray<FColor>& A, const TArray<FColor>& B)
	{
		if (A.Num() != B.Num())
		{
			return 0;
		}

		int32 ChangedPixels = 0;
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (FMath::Abs(int32(A[Index].R) - int32(B[Index].R))
				+ FMath::Abs(int32(A[Index].G) - int32(B[Index].G))
				+ FMath::Abs(int32(A[Index].B) - int32(B[Index].B)) > 12)
			{
				++ChangedPixels;
			}
		}
		return ChangedPixels;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopFullGeneratedRenderedCaptureTest,
	"P_RD.UI.ShopFullGenerated.RenderedCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopFullGeneratedRenderedCaptureTest::RunTest(const FString& Parameters)
{
	using namespace ShopFullGeneratedCaptureTests;

	if (GUsingNullRHI == true)
	{
		AddInfo(TEXT("NullRHI 환경이라 전용 생성 아트 상점 캡처 생략"));
		return true;
	}

	// 명령행 ExecCmds는 에디터 모듈 등록보다 먼저 실행될 수 있으므로,
	// 선택 원화 임포트와 WBP 재생성을 캡처 직전에 직접 보장한다.
	ShopFullGeneratedWidgetBuilder::Build();

	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("새 상점 캡처 에디터 월드"), World))
	{
		return false;
	}

	UClass* ShopClass = LoadClass<UShopUIWidgetBase>(nullptr, WidgetClassPath);
	if (!TestNotNull(TEXT("WBP_Shop_FullGenerated 클래스"), ShopClass))
	{
		return false;
	}
	TestTrue(TEXT("새 WBP는 UShopFullGeneratedWidgetBase 기반"),
		ShopClass->IsChildOf(UShopFullGeneratedWidgetBase::StaticClass()));

	UShopUIWidgetBase* Widget = CreateWidget<UShopUIWidgetBase>(World, ShopClass);
	if (!TestNotNull(TEXT("WBP_Shop_FullGenerated 인스턴스"), Widget)
		|| !TestNotNull(TEXT("WBP_Shop_FullGenerated WidgetTree"),
			Widget != nullptr ? Widget->WidgetTree.Get() : nullptr))
	{
		return false;
	}

	const TSharedRef<SWidget> ShopSlate = Widget->TakeWidget();
	UShopUIModel* Model = NewObject<UShopUIModel>(Widget,
		TEXT("ShopFullGeneratedCaptureModel"));
	if (!TestNotNull(TEXT("새 상점 합성 UIModel"), Model))
	{
		return false;
	}
	const FShopUI SyntheticShop = MakeSyntheticShop();
	TestEqual(TEXT("합성 판매 항목 10개"), SyntheticShop.mItems.Num(), 10);
	TestEqual(TEXT("합성 보유 아티팩트 3개"),
		SyntheticShop.mOwnedArtifacts.Num(), 3);
	TestEqual(TEXT("합성 소지 유닛 3명"), SyntheticShop.mOwnedUnits.Num(), 3);
	TestEqual(TEXT("합성 휴식 대상 3명"), SyntheticShop.mRest.mUnits.Num(), 3);
	TestEqual(TEXT("합성 용병 후보 3명"), SyntheticShop.mMercenaries.Num(), 3);
	TestEqual(TEXT("합성 고용 대상 파티 슬롯 3칸"),
		SyntheticShop.mMercenaryPartySlots.Num(), 3);
	Model->SetShop(SyntheticShop);
	Widget->BindUIModel(Model);
	Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UWidgetTree* Tree = Widget->WidgetTree;
	UWidget* ArtifactPanel = Tree->FindWidget(TEXT("mArtifactShopPanel"));
	UWidget* SkillPanel = Tree->FindWidget(TEXT("mSkillShopPanel"));
	UWidget* RestPanel = Tree->FindWidget(TEXT("mRestShopPanel"));
	UScaleBox* BackgroundScale = Cast<UScaleBox>(
		Tree->FindWidget(TEXT("ShopBackgroundScale")));
	UScaleBox* MasterScale = Cast<UScaleBox>(
		Tree->FindWidget(TEXT("ShopMasterScale")));
	USizeBox* DesignSize = Cast<USizeBox>(
		Tree->FindWidget(TEXT("ShopDesignSize")));
	UCanvasPanel* DesignCanvas = Cast<UCanvasPanel>(
		Tree->FindWidget(TEXT("ShopDesignCanvas")));
	UCanvasPanel* TopZone = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("TopZone")));
	UCanvasPanel* CenterZone = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("CenterZone")));
	UCanvasPanel* BottomZone = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("BottomZone")));
	UWidget* ItemCarouselPanel = Tree->FindWidget(TEXT("ItemCarouselPanel"));
	UWidget* SkillTopContextPanel = Tree->FindWidget(TEXT("SkillTopContextPanel"));
	UWidget* SkillBottomContextPanel = Tree->FindWidget(TEXT("SkillBottomContextPanel"));
	UWidget* RestBottomContextPanel = Tree->FindWidget(TEXT("RestBottomContextPanel"));
	URunOptionsRailWidget* OptionsRail = Cast<URunOptionsRailWidget>(
		Tree->FindWidget(TEXT("mRunOptionsRailWidget")));
	UImage* BackgroundArt = Cast<UImage>(
		Tree->FindWidget(TEXT("ShopBackgroundArt")));
	UTextBlock* SelectedName = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("mSelectedItemNameText")));
	UButton* PreviousButton = Cast<UButton>(
		Tree->FindWidget(TEXT("mPreviousButton")));
	UButton* NextButton = Cast<UButton>(Tree->FindWidget(TEXT("mNextButton")));
	UButton* SkillTab = Cast<UButton>(Tree->FindWidget(TEXT("mSkillTabButton")));
	UButton* RestTab = Cast<UButton>(Tree->FindWidget(TEXT("mRestTabButton")));
	UButton* MercenaryTab = Cast<UButton>(
		Tree->FindWidget(TEXT("mMercenaryTabButton")));
	UMercenaryHireWidget* MercenaryHire = Cast<UMercenaryHireWidget>(
		Tree->FindWidget(TEXT("mMercenaryHireWidget")));
	UWidget* SelectedSkillPlate = Tree->FindWidget(TEXT("SkillSlotPlate_2"));
	UButton* HeldSkillButton = Cast<UButton>(
		Tree->FindWidget(TEXT("mSkillSlotButton_0")));
	UTextBlock* SelectedDescription = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("mSelectedItemDescriptionText")));
	UTextBlock* SelectedPrice = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("mSelectedItemPriceText")));
	if (!TestNotNull(TEXT("아티팩트 패널"), ArtifactPanel)
		|| !TestNotNull(TEXT("스킬 패널"), SkillPanel)
		|| !TestNotNull(TEXT("휴식 패널"), RestPanel)
		|| !TestNotNull(TEXT("전체 화면 배경 ScaleBox"), BackgroundScale)
		|| !TestNotNull(TEXT("기능 UI ScaleBox"), MasterScale)
		|| !TestNotNull(TEXT("1600x1000 기능 UI SizeBox"), DesignSize)
		|| !TestNotNull(TEXT("기능 UI 디자인 Canvas"), DesignCanvas)
		|| !TestNotNull(TEXT("상단 210 영역"), TopZone)
		|| !TestNotNull(TEXT("중앙 580 영역"), CenterZone)
		|| !TestNotNull(TEXT("하단 210 영역"), BottomZone)
		|| !TestNotNull(TEXT("중앙 상품 캐러셀"), ItemCarouselPanel)
		|| !TestNotNull(TEXT("상단 스킬 대상 영역"), SkillTopContextPanel)
		|| !TestNotNull(TEXT("하단 스킬 슬롯 영역"), SkillBottomContextPanel)
		|| !TestNotNull(TEXT("하단 휴식 비용 영역"), RestBottomContextPanel)
		|| !TestNotNull(TEXT("TopZone 내장 공용 설정바"), OptionsRail)
		|| !TestNotNull(TEXT("공용 KayKit 배경 이미지"), BackgroundArt)
		|| !TestNotNull(TEXT("선택 상품 이름"), SelectedName)
		|| !TestNotNull(TEXT("이전 상품 버튼"), PreviousButton)
		|| !TestNotNull(TEXT("다음 상품 버튼"), NextButton)
		|| !TestNotNull(TEXT("스킬 탭"), SkillTab)
		|| !TestNotNull(TEXT("휴식 탭"), RestTab)
		|| !TestNotNull(TEXT("용병 고용 탭"), MercenaryTab)
		|| !TestNotNull(TEXT("재사용 용병 선택 WBP"), MercenaryHire)
		|| !TestNotNull(TEXT("선택 발광 스킬 프레임"), SelectedSkillPlate)
		|| !TestNotNull(TEXT("길게 누르기 스킬 버튼"), HeldSkillButton)
		|| !TestNotNull(TEXT("선택 상품 설명"), SelectedDescription)
		|| !TestNotNull(TEXT("선택 상품 가격"), SelectedPrice))
	{
		return false;
	}
	TestEqual(TEXT("정사각형 배경은 모든 해상도를 채움"),
		BackgroundScale->GetStretch(), EStretch::ScaleToFill);
	TestEqual(TEXT("기능 UI는 전투 HUD처럼 비율을 보존해 맞춤"),
		MasterScale->GetStretch(), EStretch::ScaleToFit);
	TestEqual(TEXT("기능 UI 디자인 폭"), DesignSize->GetWidthOverride(), 1600.f);
	TestEqual(TEXT("기능 UI 디자인 높이"), DesignSize->GetHeightOverride(), 1000.f);
	TestTrue(TEXT("디자인 SizeBox가 로컬 Canvas를 소유"),
		DesignSize->GetContent() == DesignCanvas);
	TestTrue(TEXT("설정바는 별도 Viewport가 아닌 TopZone 자식"),
		OptionsRail->GetParent() == TopZone);
	auto TestZoneGeometry = [this](const TCHAR* Label, UCanvasPanel* Zone,
		const FVector2D& ExpectedPosition, const FVector2D& ExpectedSize)
	{
		UCanvasPanelSlot* Slot = Zone != nullptr
			? Cast<UCanvasPanelSlot>(Zone->Slot) : nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("%s CanvasSlot"), Label), Slot))
		{
			return;
		}
		TestTrue(*FString::Printf(TEXT("%s 위치"), Label),
			Slot->GetPosition().Equals(ExpectedPosition));
		TestTrue(*FString::Printf(TEXT("%s 크기"), Label),
			Slot->GetSize().Equals(ExpectedSize));
	};
	TestZoneGeometry(TEXT("TopZone"), TopZone,
		FVector2D(0.f, 0.f), FVector2D(1600.f, 210.f));
	TestZoneGeometry(TEXT("CenterZone"), CenterZone,
		FVector2D(0.f, 210.f), FVector2D(1600.f, 580.f));
	TestZoneGeometry(TEXT("BottomZone"), BottomZone,
		FVector2D(0.f, 790.f), FVector2D(1600.f, 210.f));
	TestTrue(TEXT("상품 캐러셀은 CenterZone 자식"),
		ItemCarouselPanel->GetParent() == CenterZone);
	TestTrue(TEXT("스킬 대상은 TopZone 자식"),
		SkillTopContextPanel->GetParent() == TopZone);
	TestTrue(TEXT("장착 스킬은 BottomZone 자식"),
		SkillBottomContextPanel->GetParent() == BottomZone);
	TestTrue(TEXT("휴식 비용은 BottomZone 자식"),
		RestBottomContextPanel->GetParent() == BottomZone);
	TestNotNull(TEXT("내장 설정바 지도 버튼"),
		OptionsRail->GetWidgetFromName(TEXT("MenuButton_0")));
	TestNotNull(TEXT("내장 설정바 용병/인벤토리 버튼"),
		OptionsRail->GetWidgetFromName(TEXT("MenuButton_1")));
	TestNotNull(TEXT("내장 설정바 설정 버튼"),
		OptionsRail->GetWidgetFromName(TEXT("MenuButton_3")));
	TestNull(TEXT("아티팩트 검은 레일 배경 제거"),
		Tree->FindWidget(TEXT("ArtifactRailScrim")));
	TestNull(TEXT("스킬 검은 레일 배경 제거"),
		Tree->FindWidget(TEXT("SkillRailScrim")));
	TestNull(TEXT("휴식 검은 배경 제거"),
		Tree->FindWidget(TEXT("RestContentScrim")));
	TestEqual(TEXT("배경 크롭은 뷰포트 밖을 자름"),
		BackgroundScale->GetClipping(), EWidgetClipping::ClipToBoundsAlways);
	TestNotNull(TEXT("공용 배경 브러시에 텍스처 연결"),
		Cast<UTexture2D>(BackgroundArt->GetBrush().GetResourceObject()));

	TestEqual(TEXT("첫 화면 아티팩트 패널 표시"),
		ArtifactPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("첫 화면 스킬 패널 숨김"),
		SkillPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("첫 화면 휴식 패널 숨김"),
		RestPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestNull(TEXT("스킬 선택 화살표 제거"),
		Tree->FindWidget(TEXT("ShopSelectionPointer")));
	TestEqual(TEXT("첫 상품 선택"), SelectedName->GetText().ToString(),
		FString(TEXT("행운의 동전")));
	TestNull(TEXT("상점 전용 인벤토리 버튼 제거"),
		Tree->FindWidget(TEXT("mInventoryButton")));
	TestNull(TEXT("상점 전용 보유 아티팩트 모달 제거"),
		Tree->FindWidget(TEXT("mArtifactInventoryPanel")));

	TArray<FColor> ArtifactPixels;
	FString CaptureError;
	if (!Capture(*Widget, ShopSlate,
		TEXT("WBP_Shop_FullGenerated_Artifact.png"), ArtifactPixels, CaptureError))
	{
		AddError(CaptureError);
		return false;
	}
	TestEqual(TEXT("아티팩트 캡처 정확한 픽셀 수"), ArtifactPixels.Num(),
		CaptureWidth * CaptureHeight);

	NextButton->OnClicked.Broadcast();
	TestEqual(TEXT("다음 버튼으로 두 번째 상품 선택"),
		SelectedName->GetText().ToString(), FString(TEXT("야수의 부적")));
	PreviousButton->OnClicked.Broadcast();
	TestEqual(TEXT("이전 버튼으로 첫 상품 복귀"),
		SelectedName->GetText().ToString(), FString(TEXT("행운의 동전")));

	SkillTab->OnClicked.Broadcast();
	TestEqual(TEXT("스킬 탭 뒤 아티팩트 패널 숨김"),
		ArtifactPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("스킬 탭 패널 표시"), SkillPanel->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("스킬 탭 첫 상품 선택"), SelectedName->GetText().ToString(),
		FString(TEXT("수호 방벽")));
	TestEqual(TEXT("스킬 탭에서 선택 발광 프레임 표시"),
		SelectedSkillPlate->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestTrue(TEXT("장착 스킬 누르기 입력 바인딩"),
		HeldSkillButton->OnPressed.IsBound());
	TestTrue(TEXT("장착 스킬 떼기 입력 바인딩"),
		HeldSkillButton->OnReleased.IsBound());
	HeldSkillButton->OnPressed.Broadcast();
	// TimerManager는 같은 GFrameCounter에서 중복 Tick을 무시한다. 엔진의
	// TimerManagerTests와 같은 방식으로 테스트 프레임을 진행시킨다.
	++GFrameCounter;
	World->GetTimerManager().Tick(0.f);
	++GFrameCounter;
	World->GetTimerManager().Tick(0.50f);
	// 장착 스킬 상세도 전투 HUD의 공용 팝업으로 연다. 상점 중앙 카드는
	// 상품 이름/가격만 유지하며 길게 누른 스킬의 설명으로 대체되지 않는다.
	TestEqual(TEXT("길게 눌러도 판매 상품 이름 유지"),
		SelectedName->GetText().ToString(), FString(TEXT("수호 방벽")));
	TestEqual(TEXT("선택 상품 설명은 항상 숨김"),
		SelectedDescription->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("공용 상세를 열어도 판매 가격 유지"),
		SelectedPrice->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TArray<FColor> SkillHeldPixels;
	CaptureError.Reset();
	if (!Capture(*Widget, ShopSlate,
		TEXT("WBP_Shop_FullGenerated_SkillHeldDetail.png"), SkillHeldPixels, CaptureError))
	{
		AddError(CaptureError);
		return false;
	}
	HeldSkillButton->OnReleased.Broadcast();
	TestEqual(TEXT("손을 떼어도 판매 스킬 이름 유지"),
		SelectedName->GetText().ToString(), FString(TEXT("수호 방벽")));
	TestEqual(TEXT("손을 떼면 판매 가격 복귀"),
		SelectedPrice->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TArray<FColor> SkillPixels;
	CaptureError.Reset();
	if (!Capture(*Widget, ShopSlate,
		TEXT("WBP_Shop_FullGenerated_Skill.png"), SkillPixels, CaptureError))
	{
		AddError(CaptureError);
		return false;
	}

	RestTab->OnClicked.Broadcast();
	TestEqual(TEXT("휴식 탭 뒤 스킬 패널 숨김"),
		SkillPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("휴식 탭 패널 표시"), RestPanel->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	UTextBlock* RestHPBefore = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("RestUnitHPBeforeText_0")));
	UTextBlock* RestHPAfter = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("RestUnitHPAfterText_0")));
	UTextBlock* RestCostLabel = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("RestCostLabel")));
	if (!TestNotNull(TEXT("휴식 첫 유닛 현재 HP"), RestHPBefore)
		|| !TestNotNull(TEXT("휴식 첫 유닛 회복 HP"), RestHPAfter)
		|| !TestNotNull(TEXT("휴식 비용 라벨"), RestCostLabel))
	{
		return false;
	}
	TestEqual(TEXT("휴식 현재 HP 반영"), RestHPBefore->GetText().ToString(),
		FString(TEXT("42/100")));
	TestEqual(TEXT("휴식 회복 HP 반영"), RestHPAfter->GetText().ToString(),
		FString(TEXT("100/100")));
	TestTrue(TEXT("비용 라벨은 흰색"),
		RestCostLabel->GetColorAndOpacity().GetSpecifiedColor().Equals(
			FLinearColor::White, 0.01f));
	TArray<FColor> RestPixels;
	CaptureError.Reset();
	if (!Capture(*Widget, ShopSlate,
		TEXT("WBP_Shop_FullGenerated_Rest.png"), RestPixels, CaptureError))
	{
		AddError(CaptureError);
		return false;
	}

	MercenaryTab->OnClicked.Broadcast();
	TestEqual(TEXT("용병 탭에서 기존 선택 WBP 표시"),
		MercenaryHire->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	UTextBlock* HireAddLabel = Cast<UTextBlock>(
		MercenaryHire->GetWidgetFromName(TEXT("HireAddLabel")));
	UTextBlock* HireCostLabel = Cast<UTextBlock>(
		MercenaryHire->GetWidgetFromName(TEXT("DepartLabel")));
	UTextBlock* PartyName = Cast<UTextBlock>(
		MercenaryHire->GetWidgetFromName(TEXT("PartySlotName_0")));
	UTextBlock* PartyLevel = Cast<UTextBlock>(
		MercenaryHire->GetWidgetFromName(TEXT("PartySlotLevel_0")));
	UWidget* HireBackHolder = MercenaryHire->GetWidgetFromName(TEXT("HireBackHolder"));
	UWidget* ShopCloseHolder = Tree->FindWidget(TEXT("CloseHolder"));
	if (!TestNotNull(TEXT("용병 고용 버튼"), HireAddLabel)
		|| !TestNotNull(TEXT("용병 고용 비용"), HireCostLabel)
		|| !TestNotNull(TEXT("교체 대상 직업 텍스트"), PartyName)
		|| !TestNotNull(TEXT("교체 대상 레벨 텍스트"), PartyLevel)
		|| !TestNotNull(TEXT("용병 내부 뒤로 홀더"), HireBackHolder)
		|| !TestNotNull(TEXT("상점 나가기 홀더"), ShopCloseHolder))
	{
		return false;
	}
	TestEqual(TEXT("고용 버튼은 행동만 표시"),
		HireAddLabel->GetText().ToString(), FString(TEXT("고용")));
	TestTrue(TEXT("고용 비용은 별도 표시"),
		HireCostLabel->GetText().ToString().Contains(TEXT("고용비용"))
		&& HireCostLabel->GetText().ToString().Contains(TEXT("180")));
	TestFalse(TEXT("교체 대상 직업명에 Lv를 합치지 않음"),
		PartyName->GetText().ToString().Contains(TEXT("Lv")));
	TestTrue(TEXT("교체 대상 레벨은 두 번째 줄"),
		PartyLevel->GetText().ToString().StartsWith(TEXT("Lv.")));
	TestEqual(TEXT("용병 내부 뒤로 버튼 제거"),
		HireBackHolder->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("용병 모드에서 상점 나가기 버튼 제거"),
		ShopCloseHolder->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("용병 모드에서 상점 상단바 유지"),
		TopZone->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TArray<FColor> MercenaryPixels;
	CaptureError.Reset();
	if (!Capture(*Widget, ShopSlate,
		TEXT("WBP_Shop_FullGenerated_Mercenary.png"),
		MercenaryPixels, CaptureError))
	{
		AddError(CaptureError);
		return false;
	}

	TestTrue(TEXT("아티팩트와 스킬 캡처가 다름"),
		CountChangedPixels(ArtifactPixels, SkillPixels) > 10000);
	TestTrue(TEXT("스킬과 휴식 캡처가 다름"),
		CountChangedPixels(SkillPixels, RestPixels) > 10000);
	TestTrue(TEXT("휴식과 용병 고용 캡처가 다름"),
		CountChangedPixels(RestPixels, MercenaryPixels) > 10000);

	const TCHAR* ExpectedFiles[] = {
		TEXT("WBP_Shop_FullGenerated_Artifact.png"),
		TEXT("WBP_Shop_FullGenerated_Skill.png"),
		TEXT("WBP_Shop_FullGenerated_Rest.png"),
		TEXT("WBP_Shop_FullGenerated_Mercenary.png")
	};
	for (const TCHAR* FileName : ExpectedFiles)
	{
		const FString OutputPath = FPaths::Combine(OutputDirectory(), FileName);
		TestTrue(*FString::Printf(TEXT("캡처 파일 생성: %s"), FileName),
			IFileManager::Get().FileExists(*OutputPath));
		TestTrue(*FString::Printf(TEXT("캡처 파일이 비어 있지 않음: %s"), FileName),
			IFileManager::Get().FileSize(*OutputPath) > 1024);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
