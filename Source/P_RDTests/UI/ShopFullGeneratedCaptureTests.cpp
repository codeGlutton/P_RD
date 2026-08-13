/*****************************************************************//**
 * @file   ShopFullGeneratedCaptureTests.cpp
 * @brief  전용 생성 아트 상점 WBP의 실제 렌더와 기본 상호작용을 검증한다.
 * @details
 * 기존 WBP 캡처 테스트와 상태를 공유하지 않고, 새 WBP에 합성 상점 모델을
 * 연결해 아티팩트/스킬/휴식/인벤토리 화면을 각각 1672x941 PNG로 남긴다.
 * @date   2026-08-12
 *********************************************************************/

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Engine/Texture2D.h"
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
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/SOverlay.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace ShopFullGeneratedCaptureTests
{
	constexpr int32 CaptureWidth = 1672;
	constexpr int32 CaptureHeight = 941;
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
				Slot.mIcon = LoadTexture(SkillPaths[SkillIndex]);
			}
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
			Texture->UpdateResource();
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

		const TSharedRef<SWidget> CaptureRoot =
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
			OutError = TEXT("새 상점 1672x941 렌더 결과를 읽지 못함");
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
	Model->SetShop(SyntheticShop);
	Widget->BindUIModel(Model);
	Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UWidgetTree* Tree = Widget->WidgetTree;
	UWidget* ArtifactPanel = Tree->FindWidget(TEXT("mArtifactShopPanel"));
	UWidget* SkillPanel = Tree->FindWidget(TEXT("mSkillShopPanel"));
	UWidget* RestPanel = Tree->FindWidget(TEXT("mRestShopPanel"));
	UTextBlock* SelectedName = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("mSelectedItemNameText")));
	UButton* PreviousButton = Cast<UButton>(
		Tree->FindWidget(TEXT("mPreviousButton")));
	UButton* NextButton = Cast<UButton>(Tree->FindWidget(TEXT("mNextButton")));
	UButton* SkillTab = Cast<UButton>(Tree->FindWidget(TEXT("mSkillTabButton")));
	UButton* RestTab = Cast<UButton>(Tree->FindWidget(TEXT("mRestTabButton")));
	UButton* InventoryButton = Cast<UButton>(
		Tree->FindWidget(TEXT("mInventoryButton")));
	UWidget* InventoryPanel = Tree->FindWidget(TEXT("mArtifactInventoryPanel"));
	UButton* InventoryCloseButton = Cast<UButton>(
		Tree->FindWidget(TEXT("mArtifactInventoryCloseButton")));
	UWidget* SelectionPointer = Tree->FindWidget(TEXT("ShopSelectionPointer"));
	UWrapBox* OwnedArtifactBox = Cast<UWrapBox>(
		Tree->FindWidget(TEXT("mOwnedArtifactBox")));
	if (!TestNotNull(TEXT("아티팩트 패널"), ArtifactPanel)
		|| !TestNotNull(TEXT("스킬 패널"), SkillPanel)
		|| !TestNotNull(TEXT("휴식 패널"), RestPanel)
		|| !TestNotNull(TEXT("선택 상품 이름"), SelectedName)
		|| !TestNotNull(TEXT("이전 상품 버튼"), PreviousButton)
		|| !TestNotNull(TEXT("다음 상품 버튼"), NextButton)
		|| !TestNotNull(TEXT("스킬 탭"), SkillTab)
		|| !TestNotNull(TEXT("휴식 탭"), RestTab)
		|| !TestNotNull(TEXT("인벤토리 버튼"), InventoryButton)
		|| !TestNotNull(TEXT("인벤토리 패널"), InventoryPanel)
		|| !TestNotNull(TEXT("인벤토리 닫기"), InventoryCloseButton)
		|| !TestNotNull(TEXT("스킬 선택 포인터"), SelectionPointer)
		|| !TestNotNull(TEXT("보유 아티팩트 박스"), OwnedArtifactBox))
	{
		return false;
	}

	TestEqual(TEXT("첫 화면 아티팩트 패널 표시"),
		ArtifactPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("첫 화면 스킬 패널 숨김"),
		SkillPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("첫 화면 휴식 패널 숨김"),
		RestPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("아티팩트 화면에서 스킬 포인터 숨김"),
		SelectionPointer->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("첫 상품 선택"), SelectedName->GetText().ToString(),
		FString(TEXT("행운의 동전")));

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

	TestEqual(TEXT("인벤토리는 처음에 닫힘"), InventoryPanel->GetVisibility(),
		ESlateVisibility::Collapsed);
	InventoryButton->OnClicked.Broadcast();
	TestEqual(TEXT("인벤토리 버튼으로 오버레이 열림"),
		InventoryPanel->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("보유 아티팩트 3개 동적 렌더"),
		OwnedArtifactBox->GetChildrenCount(), 3);
	TArray<FColor> InventoryPixels;
	CaptureError.Reset();
	if (!Capture(*Widget, ShopSlate,
		TEXT("WBP_Shop_FullGenerated_Inventory.png"),
		InventoryPixels, CaptureError))
	{
		AddError(CaptureError);
		return false;
	}
	InventoryCloseButton->OnClicked.Broadcast();
	TestEqual(TEXT("인벤토리 닫기 버튼 동작"), InventoryPanel->GetVisibility(),
		ESlateVisibility::Collapsed);

	SkillTab->OnClicked.Broadcast();
	TestEqual(TEXT("스킬 탭 뒤 아티팩트 패널 숨김"),
		ArtifactPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("스킬 탭 패널 표시"), SkillPanel->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("스킬 탭 첫 상품 선택"), SelectedName->GetText().ToString(),
		FString(TEXT("수호 방벽")));
	TestEqual(TEXT("스킬 탭에서 인벤토리 버튼 숨김"),
		InventoryButton->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("스킬 탭에서 선택 포인터 표시"),
		SelectionPointer->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
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
	TestEqual(TEXT("휴식 탭에서 스킬 포인터 숨김"),
		SelectionPointer->GetVisibility(), ESlateVisibility::Collapsed);
	UTextBlock* RestHPBefore = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("RestUnitHPBeforeText_0")));
	UTextBlock* RestHPAfter = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("RestUnitHPAfterText_0")));
	if (!TestNotNull(TEXT("휴식 첫 유닛 현재 HP"), RestHPBefore)
		|| !TestNotNull(TEXT("휴식 첫 유닛 회복 HP"), RestHPAfter))
	{
		return false;
	}
	TestEqual(TEXT("휴식 현재 HP 반영"), RestHPBefore->GetText().ToString(),
		FString(TEXT("42/100")));
	TestEqual(TEXT("휴식 회복 HP 반영"), RestHPAfter->GetText().ToString(),
		FString(TEXT("100/100")));
	TArray<FColor> RestPixels;
	CaptureError.Reset();
	if (!Capture(*Widget, ShopSlate,
		TEXT("WBP_Shop_FullGenerated_Rest.png"), RestPixels, CaptureError))
	{
		AddError(CaptureError);
		return false;
	}

	TestTrue(TEXT("아티팩트와 인벤토리 캡처가 다름"),
		CountChangedPixels(ArtifactPixels, InventoryPixels) > 10000);
	TestTrue(TEXT("아티팩트와 스킬 캡처가 다름"),
		CountChangedPixels(ArtifactPixels, SkillPixels) > 10000);
	TestTrue(TEXT("스킬과 휴식 캡처가 다름"),
		CountChangedPixels(SkillPixels, RestPixels) > 10000);

	const TCHAR* ExpectedFiles[] = {
		TEXT("WBP_Shop_FullGenerated_Artifact.png"),
		TEXT("WBP_Shop_FullGenerated_Skill.png"),
		TEXT("WBP_Shop_FullGenerated_Rest.png"),
		TEXT("WBP_Shop_FullGenerated_Inventory.png")
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
