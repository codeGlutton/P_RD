/*****************************************************************//**
 * @file   ShopUITests.cpp
 * @brief  확정 가로 레일형 상점 WBP의 구조, 생성 아트, 실제 렌더를 검증한다.
 * @details
 * 정적 WBP 계약만 확인하면 런타임 데이터 바인딩 뒤 탭 전환이나 슬롯 갱신이
 * 깨져도 놓칠 수 있다. 그래서 실제 UShopUIModel 스냅샷을 연결하고 아티팩트,
 * 스킬, 휴식 화면을 각각 1672x941 PNG로 남긴다.
 * @date   2026-08-12
 *********************************************************************/

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/SlateTypes.h"
#include "UI/Shop/ShopUIModel.h"
#include "UI/Shop/ShopUIWidgetBase.h"
#include "UI/ShopUITestListener.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/SOverlay.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

void UShopUITestListener::HandleBuySkillRequested(const int32 SlotIndex,
	const int32 UnitIndex, const int32 SkillSlotIndex)
{
	++CallCount;
	LastSlotIndex = SlotIndex;
	LastUnitIndex = UnitIndex;
	LastSkillSlotIndex = SkillSlotIndex;
}

void UShopUITestListener::HandleRestRequested()
{
	++RestCallCount;
}

void UShopUITestListener::HandleUIChanged(const EShopUIDomain Domain)
{
	if (Domain == EShopUIDomain::Trade)
	{
		++TradeChangeCount;
	}
}

namespace ShopUITests
{
	constexpr int32 CaptureWidth = 1672;
	constexpr int32 CaptureHeight = 941;
	constexpr TCHAR ShopClassPath[] =
		TEXT("/Game/UI/Shop/WBP_Shop.WBP_Shop_C");

	struct FExpectedWidget
	{
		const TCHAR* Name;
		UClass* Type;
	};

	FString OutputDirectory()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UI"),
			TEXT("Shop"));
	}

	UTexture2D* LoadTexture(const TCHAR* ObjectPath)
	{
		return LoadObject<UTexture2D>(nullptr, ObjectPath);
	}

	/** @brief 실제 화면과 같은 아이콘 자원으로 충분히 큰 상점 스냅샷을 만든다. */
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
		// 실제 ShopGameMode와 같은 고정 6직업 탐색 목록. 미보유 직업도
		// 선택/상세 확인은 가능하지만 구매 대상은 될 수 없다.
		const EUnitJobType TargetJobs[] = {
			EUnitJobType::Knight, EUnitJobType::Ranger, EUnitJobType::Mage,
			EUnitJobType::Barbarian, EUnitJobType::Rogue, EUnitJobType::Druid
		};
		for (const EUnitJobType TargetJob : TargetJobs)
		{
			const FShopOwnedUnitUI* Owned = Shop.mOwnedUnits.FindByPredicate(
				[TargetJob](const FShopOwnedUnitUI& Unit)
				{
					return Unit.mJobType == TargetJob;
				});
			FShopOwnedUnitUI& JobCard = Shop.mSkillTargetUnits.AddDefaulted_GetRef();
			JobCard.mUnitIndex = INDEX_NONE;
			JobCard.mJobType = TargetJob;
			JobCard.mIsOwned = Owned != nullptr;
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

	/** @brief 첫 draw 전에 WBP와 동적 데이터가 참조하는 브러시를 스트리밍한다. */
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

	bool Capture(UShopUIWidgetBase& ShopWidget, const TSharedRef<SWidget>& ShopSlate,
		const TCHAR* FileName, TArray<FColor>& OutPixels, FString& OutError)
	{
		ShopWidget.SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ShopWidget.ForceLayoutPrepass();
		const int32 TextureCount = MakeBrushTexturesResident(ShopWidget);
		if (TextureCount == 0)
		{
			OutError = TEXT("상점 WBP 브러시에 Texture2D가 하나도 없음");
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
		UTextureRenderTarget2D* RenderTarget = Renderer.DrawWidget(
			CaptureRoot, FVector2D(CaptureWidth, CaptureHeight));
		if (RenderTarget == nullptr)
		{
			OutError = TEXT("FWidgetRenderer가 상점 렌더 타깃을 만들지 못함");
			return false;
		}

		FlushRenderingCommands();
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(false);
		if (!RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(
				OutPixels, ReadFlags)
			|| OutPixels.Num() != CaptureWidth * CaptureHeight)
		{
			OutError = TEXT("1672x941 상점 렌더 결과를 읽지 못함");
			return false;
		}

		uint8 MinChannel = 255;
		uint8 MaxChannel = 0;
		for (FColor& Pixel : OutPixels)
		{
			// RGBA8 WidgetRenderer 경로에서 중복 적용된 감마를 한 번 되돌린다.
			Pixel.R = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
			Pixel.G = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
			Pixel.B = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));
			MinChannel = FMath::Min3(MinChannel, Pixel.R,
				FMath::Min(Pixel.G, Pixel.B));
			MaxChannel = FMath::Max3(MaxChannel, Pixel.R,
				FMath::Max(Pixel.G, Pixel.B));
		}
		if (int32(MaxChannel) - int32(MinChannel) < 16)
		{
			OutError = TEXT("상점 캡처가 단색이다. WBP가 그려지지 않음");
			return false;
		}

		TArray64<uint8> PngData;
		FImageUtils::PNGCompressImageArray(
			CaptureWidth, CaptureHeight, OutPixels, PngData);
		IFileManager::Get().MakeDirectory(*OutputDirectory(), true);
		const FString OutputPath = FPaths::Combine(OutputDirectory(), FileName);
		if (!FFileHelper::SaveArrayToFile(PngData, *OutputPath))
		{
			OutError = FString::Printf(TEXT("상점 캡처 저장 실패: %s"), *OutputPath);
			return false;
		}

		UE_LOG(LogTemp, Display, TEXT("[ShopUI] %s (%d textures) -> %s"),
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
	FShopWBPContractTest,
	"P_RD.UI.Shop.WBPContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopWBPContractTest::RunTest(const FString& Parameters)
{
	// 한글 표시 문자열을 단언하므로 ko 컬처로 고정한다(0823).
	struct FScopedKoreanCulture
	{
		FString mOriginal;
		FScopedKoreanCulture()
			: mOriginal(FInternationalization::Get().GetCurrentCulture()->GetName())
		{
			FInternationalization::Get().SetCurrentCulture(TEXT("ko"));
		}
		~FScopedKoreanCulture()
		{
			FInternationalization::Get().SetCurrentCulture(mOriginal);
		}
	};
	const FScopedKoreanCulture ScopedKoreanCulture;
	using namespace ShopUITests;

	UClass* ShopClass = LoadClass<UShopUIWidgetBase>(nullptr, ShopClassPath);
	if (!TestNotNull(TEXT("WBP_Shop 클래스"), ShopClass))
	{
		return false;
	}
	TestTrue(TEXT("WBP_Shop의 네이티브 부모는 UShopUIWidgetBase"),
		ShopClass->IsChildOf(UShopUIWidgetBase::StaticClass()));

	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("상점 계약 검사 에디터 월드"), World))
	{
		return false;
	}
	UShopUIWidgetBase* Widget = CreateWidget<UShopUIWidgetBase>(World, ShopClass);
	if (!TestNotNull(TEXT("WBP_Shop 인스턴스"), Widget)
		|| !TestNotNull(TEXT("WBP_Shop WidgetTree"),
			Widget != nullptr ? Widget->WidgetTree.Get() : nullptr))
	{
		return false;
	}
	Widget->TakeWidget();
	UWidgetTree* Tree = Widget->WidgetTree;

	const FExpectedWidget Required[] = {
		{ TEXT("ShopViewportRoot"), UOverlay::StaticClass() },
		{ TEXT("ShopLetterbox"), UBorder::StaticClass() },
		{ TEXT("ShopDesignCanvas"), UCanvasPanel::StaticClass() },
		{ TEXT("mArtifactShopPanel"), UCanvasPanel::StaticClass() },
		{ TEXT("mSkillShopPanel"), UCanvasPanel::StaticClass() },
		{ TEXT("mRestShopPanel"), UCanvasPanel::StaticClass() },
		{ TEXT("mTitleText"), UTextBlock::StaticClass() },
		{ TEXT("mGoldText"), UTextBlock::StaticClass() },
		{ TEXT("mItemBox"), UHorizontalBox::StaticClass() },
		{ TEXT("mSkillItemBox"), UWrapBox::StaticClass() },
		{ TEXT("mArtifactItemBox"), UWrapBox::StaticClass() },
		{ TEXT("mOwnedArtifactBox"), UWrapBox::StaticClass() },
		{ TEXT("mOwnedUnitBox"), UWrapBox::StaticClass() },
		{ TEXT("mCloseButton"), UButton::StaticClass() },
		{ TEXT("mCloseButtonText"), UTextBlock::StaticClass() },
		{ TEXT("mArtifactTabButton"), UButton::StaticClass() },
		{ TEXT("mSkillTabButton"), UButton::StaticClass() },
		{ TEXT("mRestTabButton"), UButton::StaticClass() },
		{ TEXT("mArtifactTabText"), UTextBlock::StaticClass() },
		{ TEXT("mSkillTabText"), UTextBlock::StaticClass() },
		{ TEXT("mRestTabText"), UTextBlock::StaticClass() },
		{ TEXT("mPreviousButton"), UButton::StaticClass() },
		{ TEXT("mNextButton"), UButton::StaticClass() },
		{ TEXT("mBuyButton"), UButton::StaticClass() },
		{ TEXT("mBuyButtonText"), UTextBlock::StaticClass() },
		{ TEXT("mRestButton"), UButton::StaticClass() },
		{ TEXT("mRestButtonText"), UTextBlock::StaticClass() },
		{ TEXT("mInventoryButton"), UButton::StaticClass() },
		{ TEXT("mInventoryButtonText"), UTextBlock::StaticClass() },
		{ TEXT("mArtifactInventoryPanel"), UCanvasPanel::StaticClass() },
		{ TEXT("mArtifactInventoryCloseButton"), UButton::StaticClass() },
		{ TEXT("mSelectedItemIcon"), UImage::StaticClass() },
		{ TEXT("mSelectedItemNameText"), UTextBlock::StaticClass() },
		{ TEXT("mSelectedItemDescriptionText"), UTextBlock::StaticClass() },
		{ TEXT("mSelectedItemPriceText"), UTextBlock::StaticClass() },
	};
	bool bContractValid = true;
	for (const FExpectedWidget& Expected : Required)
	{
		UWidget* Found = Tree->FindWidget(FName(Expected.Name));
		bContractValid &= TestNotNull(*FString::Printf(
			TEXT("필수 상점 위젯 %s"), Expected.Name), Found);
		if (Found != nullptr)
		{
			bContractValid &= TestTrue(*FString::Printf(
				TEXT("%s 타입은 %s"), Expected.Name, *Expected.Type->GetName()),
				Found->IsA(Expected.Type));
		}
	}

	for (int32 Index = 0; Index < 5; ++Index)
	{
		const FString ButtonName = FString::Printf(TEXT("ShopRailButton_%d"), Index);
		const FString PlateName = FString::Printf(TEXT("ShopRailPlate_%d"), Index);
		const FString IconName = FString::Printf(TEXT("ShopRailIcon_%d"), Index);
		const FString PriceName = FString::Printf(TEXT("ShopRailPriceText_%d"), Index);
		UButton* Button = Cast<UButton>(Tree->FindWidget(FName(*ButtonName)));
		bContractValid &= TestNotNull(*ButtonName, Button);
		bContractValid &= TestNotNull(*PlateName,
			Cast<UImage>(Tree->FindWidget(FName(*PlateName))));
		bContractValid &= TestNotNull(*IconName,
			Cast<UImage>(Tree->FindWidget(FName(*IconName))));
		bContractValid &= TestNotNull(*PriceName,
			Cast<UTextBlock>(Tree->FindWidget(FName(*PriceName))));
		if (Button != nullptr)
		{
			bContractValid &= TestTrue(*FString::Printf(
				TEXT("레일 버튼 %d 짧은 탭 이벤트 연결"), Index),
				Button->OnClicked.IsBound());
		}
	}

	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FString ButtonName = FString::Printf(TEXT("mUnitSelectButton_%d"), Index);
		const FString PlateName = FString::Printf(TEXT("UnitSelectPlate_%d"), Index);
		const FString IconName = FString::Printf(TEXT("mUnitSelectIcon_%d"), Index);
		UButton* Button = Cast<UButton>(Tree->FindWidget(FName(*ButtonName)));
		bContractValid &= TestNotNull(*ButtonName, Button);
		bContractValid &= TestNotNull(*PlateName,
			Cast<UImage>(Tree->FindWidget(FName(*PlateName))));
		bContractValid &= TestNotNull(*IconName,
			Cast<UImage>(Tree->FindWidget(FName(*IconName))));
		if (Button != nullptr)
		{
			bContractValid &= TestTrue(*FString::Printf(
				TEXT("유닛 선택 버튼 %d 런타임 이벤트 연결"), Index),
				Button->OnClicked.IsBound());
		}

		const FExpectedWidget RestWidgets[] = {
			{ TEXT("RestUnitPlate"), UImage::StaticClass() },
			{ TEXT("RestUnitIcon"), UImage::StaticClass() },
			{ TEXT("RestUnitHPBeforeText"), UTextBlock::StaticClass() },
			{ TEXT("RestUnitHPAfterText"), UTextBlock::StaticClass() },
			{ TEXT("RestUnitAPBeforeText"), UTextBlock::StaticClass() },
			{ TEXT("RestUnitAPAfterText"), UTextBlock::StaticClass() },
			{ TEXT("RestUnitHPBeforeFill"), UImage::StaticClass() },
			{ TEXT("RestUnitHPAfterFill"), UImage::StaticClass() },
			{ TEXT("RestUnitAPBeforeFill"), UImage::StaticClass() },
			{ TEXT("RestUnitAPAfterFill"), UImage::StaticClass() },
		};
		for (const FExpectedWidget& Expected : RestWidgets)
		{
			const FString WidgetName = FString::Printf(
				TEXT("%s_%d"), Expected.Name, Index);
			UWidget* Found = Tree->FindWidget(FName(*WidgetName));
			bContractValid &= TestNotNull(*FString::Printf(
				TEXT("필수 휴식 위젯 %s"), *WidgetName), Found);
			if (Found != nullptr)
			{
				bContractValid &= TestTrue(*FString::Printf(
					TEXT("%s 타입은 %s"), *WidgetName, *Expected.Type->GetName()),
					Found->IsA(Expected.Type));
			}
		}
		const FString RestRowHolderName = FString::Printf(
			TEXT("RestUnitRowHolder_%d"), Index);
		UWidget* RestRowHolder = Tree->FindWidget(FName(*RestRowHolderName));
		bContractValid &= TestNotNull(*FString::Printf(
			TEXT("필수 휴식 행 홀더 %s"), *RestRowHolderName), RestRowHolder);
		if (RestRowHolder != nullptr)
		{
			bContractValid &= TestTrue(*FString::Printf(
				TEXT("%s 타입은 CanvasPanel"), *RestRowHolderName),
				RestRowHolder->IsA(UCanvasPanel::StaticClass()));
		}
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FString ButtonName = FString::Printf(TEXT("mSkillSlotButton_%d"), Index);
		const FString PlateName = FString::Printf(TEXT("SkillSlotPlate_%d"), Index);
		const FString IconName = FString::Printf(TEXT("mSkillSlotIcon_%d"), Index);
		UButton* Button = Cast<UButton>(Tree->FindWidget(FName(*ButtonName)));
		bContractValid &= TestNotNull(*ButtonName, Button);
		bContractValid &= TestNotNull(*PlateName,
			Cast<UImage>(Tree->FindWidget(FName(*PlateName))));
		bContractValid &= TestNotNull(*IconName,
			Cast<UImage>(Tree->FindWidget(FName(*IconName))));
		if (Button != nullptr)
		{
			bContractValid &= TestTrue(*FString::Printf(
				TEXT("스킬 슬롯 버튼 %d 누르기 이벤트 연결"), Index),
				Button->OnPressed.IsBound());
			bContractValid &= TestTrue(*FString::Printf(
				TEXT("스킬 슬롯 버튼 %d 떼기 이벤트 연결"), Index),
				Button->OnReleased.IsBound());
		}
	}

	for (const TCHAR* ButtonName : {
		TEXT("mArtifactTabButton"), TEXT("mSkillTabButton"), TEXT("mRestTabButton"),
		TEXT("mPreviousButton"), TEXT("mNextButton"), TEXT("mBuyButton"),
		TEXT("mRestButton"), TEXT("mInventoryButton"),
		TEXT("mArtifactInventoryCloseButton"), TEXT("mCloseButton") })
	{
		UButton* Button = Cast<UButton>(Tree->FindWidget(ButtonName));
		if (Button != nullptr)
		{
			bContractValid &= TestTrue(*FString::Printf(
				TEXT("%s 런타임 이벤트 연결"), ButtonName),
				Button->OnClicked.IsBound());
		}
	}

	TestNull(TEXT("고정 디자인 ScaleBox 제거"),
		Tree->FindWidget(TEXT("ShopMasterScale")));
	TestNull(TEXT("고정 1600x1000 SizeBox 제거"),
		Tree->FindWidget(TEXT("ShopDesignSize")));
	TestNull(TEXT("아티팩트 검은 레일 배경 제거"),
		Tree->FindWidget(TEXT("ArtifactRailScrim")));
	TestNull(TEXT("스킬 검은 레일 배경 제거"),
		Tree->FindWidget(TEXT("SkillRailScrim")));
	TestNull(TEXT("휴식 검은 배경 제거"),
		Tree->FindWidget(TEXT("RestContentScrim")));
	if (UTextBlock* CloseText = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("mCloseButtonText"))))
	{
		TestEqual(TEXT("상점 종료 버튼 문구"), CloseText->GetText().ToString(),
			FString(TEXT("나가기")));
	}
	return bContractValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopGeneratedTextureContractTest,
	"P_RD.UI.Shop.GeneratedTextureContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopGeneratedTextureContractTest::RunTest(const FString& Parameters)
{
	const TCHAR* GeneratedTexturePaths[] = {
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Shop/Backgrounds/T_Shop_ArtifactMarket_Background.T_Shop_ArtifactMarket_Background"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Shop/Backgrounds/T_Shop_SkillCourtyard_Background.T_Shop_SkillCourtyard_Background"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Shop/Backgrounds/T_Shop_RestInn_Background.T_Shop_RestInn_Background"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Shop/Panels/T_Shop_RailCard_Normal.T_Shop_RailCard_Normal"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Shop/Panels/T_Shop_RailCard_Selected.T_Shop_RailCard_Selected")
	};

	bool bAllValid = true;
	for (const TCHAR* ObjectPath : GeneratedTexturePaths)
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, ObjectPath);
		bAllValid &= TestNotNull(*FString::Printf(
			TEXT("생성 상점 텍스처가 로드됨: %s"), ObjectPath), Texture);
		if (Texture == nullptr)
		{
			continue;
		}
		bAllValid &= TestEqual(*FString::Printf(
			TEXT("UI LOD 그룹: %s"), ObjectPath), Texture->LODGroup,
			TEnumAsByte<TextureGroup>(TEXTUREGROUP_UI));
		bAllValid &= TestEqual(*FString::Printf(
			TEXT("NoMipmaps: %s"), ObjectPath), Texture->MipGenSettings,
			TEnumAsByte<TextureMipGenSettings>(TMGS_NoMipmaps));
		const FIntPoint ImportedSize = Texture->GetImportedSize();
		bAllValid &= TestTrue(*FString::Printf(
			TEXT("유효한 원본 크기: %s"), ObjectPath),
			ImportedSize.X > 0 && ImportedSize.Y > 0);
	}
	return bAllValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopRenderedCaptureTest,
	"P_RD.UI.Shop.RenderedCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopRenderedCaptureTest::RunTest(const FString& Parameters)
{
	using namespace ShopUITests;

	if (GUsingNullRHI == true)
	{
		AddInfo(TEXT("NullRHI 환경이라 상점 WBP 캡처 생략"));
		return true;
	}

	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("상점 캡처 에디터 월드"), World))
	{
		return false;
	}
	UClass* ShopClass = LoadClass<UShopUIWidgetBase>(nullptr, ShopClassPath);
	if (!TestNotNull(TEXT("상점 캡처 WBP 클래스"), ShopClass))
	{
		return false;
	}
	UShopUIWidgetBase* Widget = CreateWidget<UShopUIWidgetBase>(World, ShopClass);
	if (!TestNotNull(TEXT("상점 캡처 WBP 인스턴스"), Widget)
		|| !TestNotNull(TEXT("상점 캡처 WidgetTree"),
			Widget != nullptr ? Widget->WidgetTree.Get() : nullptr))
	{
		return false;
	}

	const TSharedRef<SWidget> ShopSlate = Widget->TakeWidget();
	UShopUIModel* Model = NewObject<UShopUIModel>(Widget, TEXT("ShopCaptureModel"));
	if (!TestNotNull(TEXT("합성 상점 UIModel"), Model))
	{
		return false;
	}
	const FShopUI SyntheticShop = MakeSyntheticShop();
	TestEqual(TEXT("합성 판매 항목 10개"), SyntheticShop.mItems.Num(), 10);
	TestEqual(TEXT("합성 소지 유닛 3명"), SyntheticShop.mOwnedUnits.Num(), 3);
	TestEqual(TEXT("합성 휴식 대상 3명"), SyntheticShop.mRest.mUnits.Num(), 3);
	TestEqual(TEXT("합성 휴식 가격"), SyntheticShop.mRest.mPrice, 100);
	for (int32 UnitIndex = 0; UnitIndex < SyntheticShop.mOwnedUnits.Num(); ++UnitIndex)
	{
		TestEqual(*FString::Printf(TEXT("합성 유닛 %d 실제 스킬 슬롯 6개"), UnitIndex),
			SyntheticShop.mOwnedUnits[UnitIndex].mSkillSlots.Num(), 6);
	}
	Model->SetShop(SyntheticShop);
	Widget->BindUIModel(Model);
	Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UWidgetTree* Tree = Widget->WidgetTree;
	UWidget* ArtifactPanel = Tree->FindWidget(TEXT("mArtifactShopPanel"));
	UWidget* SkillPanel = Tree->FindWidget(TEXT("mSkillShopPanel"));
	UWidget* RestPanel = Tree->FindWidget(TEXT("mRestShopPanel"));
	UTextBlock* SelectedName = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("mSelectedItemNameText")));
	UButton* SkillTab = Cast<UButton>(Tree->FindWidget(TEXT("mSkillTabButton")));
	UButton* RestTab = Cast<UButton>(Tree->FindWidget(TEXT("mRestTabButton")));
	UButton* RestButton = Cast<UButton>(Tree->FindWidget(TEXT("mRestButton")));
	UButton* InventoryButton = Cast<UButton>(
		Tree->FindWidget(TEXT("mInventoryButton")));
	UWidget* ArtifactInventoryPanel = Tree->FindWidget(
		TEXT("mArtifactInventoryPanel"));
	UButton* ArtifactInventoryCloseButton = Cast<UButton>(
		Tree->FindWidget(TEXT("mArtifactInventoryCloseButton")));
	UWrapBox* OwnedArtifactBox = Cast<UWrapBox>(
		Tree->FindWidget(TEXT("mOwnedArtifactBox")));
	if (!TestNotNull(TEXT("아티팩트 화면 패널"), ArtifactPanel)
		|| !TestNotNull(TEXT("스킬 화면 패널"), SkillPanel)
		|| !TestNotNull(TEXT("휴식 화면 패널"), RestPanel)
		|| !TestNotNull(TEXT("선택 상품 이름"), SelectedName)
		|| !TestNotNull(TEXT("스킬 탭 버튼"), SkillTab)
		|| !TestNotNull(TEXT("휴식 탭 버튼"), RestTab)
		|| !TestNotNull(TEXT("휴식 실행 버튼"), RestButton)
		|| !TestNotNull(TEXT("인벤토리 열기 버튼"), InventoryButton)
		|| !TestNotNull(TEXT("아티팩트 인벤토리 패널"), ArtifactInventoryPanel)
		|| !TestNotNull(TEXT("인벤토리 닫기 버튼"), ArtifactInventoryCloseButton)
		|| !TestNotNull(TEXT("보유 아티팩트 박스"), OwnedArtifactBox))
	{
		return false;
	}
	TestEqual(TEXT("첫 화면은 아티팩트 패널 표시"),
		ArtifactPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("첫 화면은 스킬 패널 숨김"),
		SkillPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("첫 화면은 휴식 패널 숨김"),
		RestPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("첫 선택은 아티팩트"), SelectedName->GetText().ToString(),
		FString(TEXT("행운의 동전")));
	TestEqual(TEXT("아티팩트 탭은 인벤토리 버튼 표시"),
		InventoryButton->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("인벤토리는 처음에 닫힘"),
		ArtifactInventoryPanel->GetVisibility(), ESlateVisibility::Collapsed);
	InventoryButton->OnClicked.Broadcast();
	TestEqual(TEXT("인벤토리 버튼으로 오버레이 열림"),
		ArtifactInventoryPanel->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("기존 보유 아티팩트 3개를 동적으로 렌더"),
		OwnedArtifactBox->GetChildrenCount(), 3);
	TArray<FColor> InventoryPixels;
	FString InventoryCaptureError;
	if (!Capture(*Widget, ShopSlate, TEXT("WBP_Shop_Inventory.png"),
		InventoryPixels, InventoryCaptureError))
	{
		AddError(InventoryCaptureError);
		return false;
	}
	ArtifactInventoryCloseButton->OnClicked.Broadcast();
	TestEqual(TEXT("닫기 버튼으로 인벤토리 오버레이 닫힘"),
		ArtifactInventoryPanel->GetVisibility(), ESlateVisibility::Collapsed);

	TArray<FColor> ArtifactPixels;
	FString CaptureError;
	if (!Capture(*Widget, ShopSlate, TEXT("WBP_Shop_Artifact.png"),
		ArtifactPixels, CaptureError))
	{
		AddError(CaptureError);
		return false;
	}

	InventoryButton->OnClicked.Broadcast();
	TestEqual(TEXT("탭 전환 전 인벤토리 오버레이 열림"),
		ArtifactInventoryPanel->GetVisibility(), ESlateVisibility::Visible);
	SkillTab->OnClicked.Broadcast();
	TestEqual(TEXT("다른 탭 전환 시 인벤토리 자동 닫힘"),
		ArtifactInventoryPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("스킬 탭은 인벤토리 버튼 숨김"),
		InventoryButton->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("스킬 탭 뒤 아티팩트 패널 숨김"),
		ArtifactPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("스킬 탭 뒤 스킬 패널 표시"),
		SkillPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("스킬 탭 첫 선택"), SelectedName->GetText().ToString(),
		FString(TEXT("수호 방벽")));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (UButton* UnitButton = Cast<UButton>(Tree->FindWidget(FName(
			*FString::Printf(TEXT("mUnitSelectButton_%d"), Index)))))
		{
			TestEqual(*FString::Printf(TEXT("유닛 %d 선택기 표시"), Index),
				UnitButton->GetVisibility(), ESlateVisibility::Visible);
		}
	}
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (UButton* SlotButton = Cast<UButton>(Tree->FindWidget(FName(
			*FString::Printf(TEXT("mSkillSlotButton_%d"), Index)))))
		{
			TestEqual(*FString::Printf(TEXT("스킬 슬롯 %d 표시"), Index),
				SlotButton->GetVisibility(), ESlateVisibility::Visible);
		}
	}

	// 이동은 모델 밖의 공용 명령이다. 화면 0..3은 기본 공격(0번) 다음의
	// 실제 모델 교체 슬롯 1..4로 왕복해야 한다.
	UShopUITestListener* Listener = NewObject<UShopUITestListener>(Widget);
	Model->OnBuySkillRequested.AddDynamic(
		Listener, &UShopUITestListener::HandleBuySkillRequested);
	Model->OnRestRequested.AddDynamic(
		Listener, &UShopUITestListener::HandleRestRequested);
	Model->OnUIChanged.AddDynamic(
		Listener, &UShopUITestListener::HandleUIChanged);
	UButton* LastSkillSlot = Cast<UButton>(
		Tree->FindWidget(TEXT("mSkillSlotButton_3")));
	UButton* BuyButton = Cast<UButton>(Tree->FindWidget(TEXT("mBuyButton")));
	if (!TestNotNull(TEXT("마지막 교체 슬롯 버튼"), LastSkillSlot)
		|| !TestNotNull(TEXT("스킬 교체 버튼"), BuyButton))
	{
		return false;
	}
	// 스킬 슬롯은 Clicked 가 아니라 Pressed/Released 로 동작한다 -- 길게
	// 누르면 상세, 그냥 떼면 선택이다. Clicked 만 쏘면 아무 일도 안 일어나
	// 선택이 0번에 머문 채 구매가 나간다.
	LastSkillSlot->OnPressed.Broadcast();
	LastSkillSlot->OnReleased.Broadcast();
	BuyButton->OnClicked.Broadcast();
	TestEqual(TEXT("스킬 구매 요청 1회"), Listener->CallCount, 1);
	TestEqual(TEXT("판매 슬롯 payload"), Listener->LastSlotIndex, 100);
	TestEqual(TEXT("선택 유닛 payload"), Listener->LastUnitIndex, 0);
	TestEqual(TEXT("화면 4번째는 실제 스킬 슬롯 4"),
		Listener->LastSkillSlotIndex, 4);

	// 같은 직업 용병이 둘 이상이면 직업 카드를 없애거나 복제하지 않는다.
	// 선택된 직업 카드를 다시 누를 때 실제 구매 대상만 다음 용병으로 순환한다.
	FShopUI DuplicateJobShop = SyntheticShop;
	FShopOwnedUnitUI DuplicateKnight = DuplicateJobShop.mOwnedUnits[0];
	DuplicateKnight.mUnitIndex = 7;
	DuplicateKnight.mLevel = 9;
	DuplicateKnight.mSkillSlots[4].mName = FText::FromString(TEXT("두 번째 기사 스킬"));
	DuplicateJobShop.mOwnedUnits.Add(DuplicateKnight);
	Model->SetShop(DuplicateJobShop);
	UButton* KnightUnitButton = Cast<UButton>(
		Tree->FindWidget(TEXT("mUnitSelectButton_0")));
	if (!TestNotNull(TEXT("기사 직업 선택 버튼"), KnightUnitButton))
	{
		return false;
	}
	KnightUnitButton->OnClicked.Broadcast();
	BuyButton->OnClicked.Broadcast();
	TestEqual(TEXT("같은 직업 카드 재선택은 다음 실제 용병을 대상으로 함"),
		Listener->LastUnitIndex, 7);
	KnightUnitButton->OnClicked.Broadcast();
	BuyButton->OnClicked.Broadcast();
	TestEqual(TEXT("같은 직업 대상 선택은 끝에서 처음으로 순환"),
		Listener->LastUnitIndex, 0);
	TestEqual(TEXT("동일 직업 두 대상에 구매 요청 전달"), Listener->CallCount, 3);

	TArray<FColor> SkillPixels;
	CaptureError.Reset();
	if (!Capture(*Widget, ShopSlate, TEXT("WBP_Shop_Skill.png"),
		SkillPixels, CaptureError))
	{
		AddError(CaptureError);
		return false;
	}

	// 0824 합의: 스킬 목록은 직업으로 거르지 않는다. 어느 용병을 골라도 이
	// 상점이 파는 스킬 전부가 같은 순서로 보이고, 고른 용병이 못 쓰는 것만
	// 상태 줄과 구매 버튼에서 갈린다.
	FShopUI JobFilteredShop = SyntheticShop;
	const EUnitJobType RequiredJobs[] = {
		EUnitJobType::Knight, EUnitJobType::Mage, EUnitJobType::Rogue,
		EUnitJobType::Common, EUnitJobType::Common
	};
	int32 SkillOrdinal = 0;
	for (FShopItemUI& Item : JobFilteredShop.mItems)
	{
		if (Item.mKind == EShopItemKind::Skill
			&& SkillOrdinal < UE_ARRAY_COUNT(RequiredJobs))
		{
			Item.mRequiredJobType = RequiredJobs[SkillOrdinal++];
		}
	}
	TestEqual(TEXT("직업 필터용 스킬 5개 분류"), SkillOrdinal, 5);
	const int32 TradeChangesBeforeFilterPush = Listener->TradeChangeCount;
	Model->SetShop(JobFilteredShop);
	TestEqual(TEXT("직업 필터 스냅샷 Trade 알림 1회"),
		Listener->TradeChangeCount, TradeChangesBeforeFilterPush + 1);
	UButton* RangerUnitButton = Cast<UButton>(
		Tree->FindWidget(TEXT("mUnitSelectButton_1")));
	UButton* MageUnitButton = Cast<UButton>(
		Tree->FindWidget(TEXT("mUnitSelectButton_2")));
	UButton* NextButton = Cast<UButton>(Tree->FindWidget(TEXT("mNextButton")));
	if (!TestNotNull(TEXT("미보유 레인저 선택 버튼"), RangerUnitButton)
		|| !TestNotNull(TEXT("마법사 선택 버튼"), MageUnitButton)
		|| !TestNotNull(TEXT("다음 상품 버튼"), NextButton))
	{
		return false;
	}
	UTextBlock* SelectedNote = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("mSelectedItemDescriptionText")));
	UTextBlock* BuyLabel = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("mBuyButtonText")));
	if (!TestNotNull(TEXT("선택 상품 상태 줄"), SelectedNote)
		|| !TestNotNull(TEXT("구매 버튼 라벨"), BuyLabel))
	{
		return false;
	}
	auto CountVisibleRailButtons = [Tree]()
	{
		int32 Visible = 0;
		for (int32 Index = 0; Index < 5; ++Index)
		{
			if (const UButton* RailButton = Cast<UButton>(Tree->FindWidget(FName(
				*FString::Printf(TEXT("ShopRailButton_%d"), Index))));
				RailButton != nullptr
				&& RailButton->GetVisibility() == ESlateVisibility::Visible)
			{
				++Visible;
			}
		}
		return Visible;
	};

	RangerUnitButton->OnClicked.Broadcast();
	TestTrue(TEXT("미보유 용병도 상세 확인용 선택 가능"),
		RangerUnitButton->GetIsEnabled());
	TestEqual(TEXT("미보유 레인저는 공용 스킬만 표시"),
		CountVisibleRailButtons(), 2);
	TestTrue(TEXT("미보유 대상은 구매 상태에 표시"),
		SelectedNote->GetText().ToString().Contains(TEXT("미보유")));
	TestTrue(TEXT("미보유 용병은 회색 처리"),
		RangerUnitButton->GetRenderOpacity() < .5f);

	MageUnitButton->OnClicked.Broadcast();
	TestEqual(TEXT("마법사는 전용+공용 스킬만 표시"),
		CountVisibleRailButtons(), 3);
	TestEqual(TEXT("마법사 선택 시 마법사 전용 스킬이 첫 상품"),
		SelectedName->GetText().ToString(), FString(TEXT("회전 베기")));
	TestFalse(TEXT("자기 직업 전용 스킬은 막지 않는다"),
		SelectedNote->GetText().ToString().Contains(TEXT("불가")));
	// 이미 가진 스킬은 직업 필터 목록에는 남기되 "보유"로 갈라 두 번
	// 사지 않게 한다.
	FShopUI OwnedMarkedShop = JobFilteredShop;
	for (FShopItemUI& Item : OwnedMarkedShop.mItems)
	{
		if (Item.mKind == EShopItemKind::Skill)
		{
			Item.mOwnedByUnitIndices.Add(
				OwnedMarkedShop.mOwnedUnits[1].mUnitIndex);
		}
	}
	Model->SetShop(OwnedMarkedShop);
	TestEqual(TEXT("보유 스킬도 목록에서 빠지지 않는다"),
		CountVisibleRailButtons(), 3);
	TestTrue(TEXT("보유 스킬은 상태 줄에 보유로 적힌다"),
		SelectedNote->GetText().ToString().Contains(TEXT("이미 보유")));
	TestEqual(TEXT("보유 스킬은 구매 버튼에서 막힌다"),
		BuyLabel->GetText().ToString(), FString(TEXT("이미 보유")));

	// 휴식 탭은 파티 전원의 회복 전/후 값을 그리고, 단일 요청만 모델에 전달한다.
	Model->SetShop(SyntheticShop);
	RestTab->OnClicked.Broadcast();
	TestEqual(TEXT("휴식 탭 뒤 아티팩트 패널 숨김"),
		ArtifactPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("휴식 탭 뒤 스킬 패널 숨김"),
		SkillPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("휴식 탭 뒤 휴식 패널 표시"),
		RestPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UWidget* UnitPlate = Tree->FindWidget(FName(
			*FString::Printf(TEXT("RestUnitPlate_%d"), Index)));
		if (TestNotNull(*FString::Printf(TEXT("휴식 유닛 %d 패널"), Index), UnitPlate))
		{
			TestEqual(*FString::Printf(TEXT("휴식 유닛 %d 표시"), Index),
				UnitPlate->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		}
	}
	UTextBlock* RestHPBefore0 = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("RestUnitHPBeforeText_0")));
	UTextBlock* RestHPAfter0 = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("RestUnitHPAfterText_0")));
	UTextBlock* RestAPBefore0 = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("RestUnitAPBeforeText_0")));
	UTextBlock* RestAPAfter0 = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("RestUnitAPAfterText_0")));
	UImage* RestHPBeforeFill0 = Cast<UImage>(
		Tree->FindWidget(TEXT("RestUnitHPBeforeFill_0")));
	UTextBlock* RestButtonText = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("mRestButtonText")));
	if (!TestNotNull(TEXT("휴식 첫 유닛 HP 현재값"), RestHPBefore0)
		|| !TestNotNull(TEXT("휴식 첫 유닛 HP 회복값"), RestHPAfter0)
		|| !TestNotNull(TEXT("휴식 첫 유닛 AP 현재값"), RestAPBefore0)
		|| !TestNotNull(TEXT("휴식 첫 유닛 AP 회복값"), RestAPAfter0)
		|| !TestNotNull(TEXT("휴식 첫 유닛 HP 현재 바"), RestHPBeforeFill0)
		|| !TestNotNull(TEXT("휴식 버튼 라벨"), RestButtonText))
	{
		return false;
	}
	TestEqual(TEXT("휴식 첫 유닛 HP 현재값 반영"),
		RestHPBefore0->GetText().ToString(), FString(TEXT("42/100")));
	TestEqual(TEXT("휴식 첫 유닛 HP 회복값 반영"),
		RestHPAfter0->GetText().ToString(), FString(TEXT("92/100")));
	TestEqual(TEXT("휴식 첫 유닛 AP 현재값 반영"),
		RestAPBefore0->GetText().ToString(), FString(TEXT("6/12")));
	TestEqual(TEXT("휴식 첫 유닛 AP 회복값 반영"),
		RestAPAfter0->GetText().ToString(), FString(TEXT("6/12")));
	TestTrue(TEXT("휴식 첫 유닛 HP 현재 바 비율 반영"), FMath::IsNearlyEqual(
		RestHPBeforeFill0->GetRenderTransform().Scale.X, 0.42f));
	TestTrue(TEXT("휴식 요청 가능"), RestButton->GetIsEnabled());
	const FString RestButtonLabel = RestButtonText->GetText().ToString();
	TestTrue(TEXT("휴식 버튼 라벨"),
		RestButtonLabel == TEXT("휴식하기") || RestButtonLabel == TEXT("Rest"));
	RestButton->OnClicked.Broadcast();
	TestEqual(TEXT("휴식 요청 1회"), Listener->RestCallCount, 1);
	FShopUI UsedRestShop = SyntheticShop;
	UsedRestShop.mRest.mIsUsed = true;
	UsedRestShop.mRest.mIsAffordable = false;
	Model->SetShop(UsedRestShop);
	TestFalse(TEXT("사용 완료 휴식 버튼 비활성"), RestButton->GetIsEnabled());
	const FString RestCompleteLabel = RestButtonText->GetText().ToString();
	TestTrue(TEXT("사용 완료 휴식 버튼 라벨"),
		RestCompleteLabel == TEXT("휴식 완료")
		|| RestCompleteLabel == TEXT("Rest Complete"));
	RestButton->OnClicked.Broadcast();
	TestEqual(TEXT("사용 완료 뒤 휴식 요청 추가 없음"), Listener->RestCallCount, 1);
	FShopUI OneUnitRestShop = SyntheticShop;
	OneUnitRestShop.mRest.mUnits.SetNum(1);
	Model->SetShop(OneUnitRestShop);
	for (int32 EmptyRowIndex = 1; EmptyRowIndex < 3; ++EmptyRowIndex)
	{
		UWidget* EmptyRowHolder = Tree->FindWidget(FName(*FString::Printf(
			TEXT("RestUnitRowHolder_%d"), EmptyRowIndex)));
		if (TestNotNull(*FString::Printf(TEXT("빈 휴식 행 %d 홀더"), EmptyRowIndex),
			EmptyRowHolder))
		{
			TestEqual(*FString::Printf(TEXT("빈 휴식 행 %d 전체 숨김"), EmptyRowIndex),
				EmptyRowHolder->GetVisibility(), ESlateVisibility::Collapsed);
		}
	}
	// The completed state is asserted above; capture the actionable initial state
	// so the visual artifact matches the approved shop concept.
	Model->SetShop(SyntheticShop);

	TArray<FColor> RestPixels;
	CaptureError.Reset();
	if (!Capture(*Widget, ShopSlate, TEXT("WBP_Shop_Rest.png"),
		RestPixels, CaptureError))
	{
		AddError(CaptureError);
		return false;
	}

	TestTrue(TEXT("아티팩트/스킬 캡처가 실제로 다른 화면"),
		CountChangedPixels(ArtifactPixels, SkillPixels) > 10000);
	TestTrue(TEXT("스킬/휴식 캡처가 실제로 다른 화면"),
		CountChangedPixels(SkillPixels, RestPixels) > 10000);
	TestTrue(TEXT("아티팩트 캡처 파일 생성"), IFileManager::Get().FileExists(
		*FPaths::Combine(OutputDirectory(), TEXT("WBP_Shop_Artifact.png"))));
	TestTrue(TEXT("스킬 캡처 파일 생성"), IFileManager::Get().FileExists(
		*FPaths::Combine(OutputDirectory(), TEXT("WBP_Shop_Skill.png"))));
	TestTrue(TEXT("휴식 캡처 파일 생성"), IFileManager::Get().FileExists(
		*FPaths::Combine(OutputDirectory(), TEXT("WBP_Shop_Rest.png"))));
	TestTrue(TEXT("인벤토리 캡처 파일 생성"), IFileManager::Get().FileExists(
		*FPaths::Combine(OutputDirectory(), TEXT("WBP_Shop_Inventory.png"))));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
