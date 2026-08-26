#include "UI/WidgetFontAudit.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/TextBlock.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace WidgetFontAudit
{
	TUniquePtr<FAutoConsoleCommand> ExportCommand;
	TUniquePtr<FAutoConsoleCommand> ApplyCommand;
	TUniquePtr<FAutoConsoleCommand> RestoreMobileScaleCommand;
	TUniquePtr<FAutoConsoleCommand> ApplyAutoFitCommand;
	constexpr float LegacyMobileFontScale = 1.35f;

	FString SanitizeField(FString Value)
	{
		Value.ReplaceInline(TEXT("\t"), TEXT(" "));
		Value.ReplaceInline(TEXT("\r"), TEXT(" "));
		Value.ReplaceInline(TEXT("\n"), TEXT(" "));
		return Value;
	}

	TArray<FAssetData> FindWidgetBlueprints()
	{
		FARFilter Filter;
		Filter.PackagePaths.Add(TEXT("/Game/UI"));
		Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
		Filter.bRecursivePaths = true;

		TArray<FAssetData> Assets;
		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().GetAssets(Filter, Assets);
		Assets.Sort([](const FAssetData& Left, const FAssetData& Right)
		{
			return Left.PackageName.LexicalLess(Right.PackageName);
		});
		return Assets;
	}

	void Export(const TArray<FString>& Args)
	{
		const FString Label = Args.IsEmpty() ? TEXT("current") : Args[0];
		const FString OutputDirectory = FPaths::ProjectSavedDir() / TEXT("UIAudit");
		IFileManager::Get().MakeDirectory(*OutputDirectory, true);
		const FString ProgressPath = OutputDirectory /
			FString::Printf(TEXT("WidgetFonts_%s.progress.txt"), *Label);
		FFileHelper::SaveStringToFile(TEXT("command-start\n"), *ProgressPath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		const FString OutputPath = OutputDirectory /
			FString::Printf(TEXT("WidgetFonts_%s.tsv"), *Label);

		FString Output = TEXT("AssetPath\tWidgetName\tSize\tTypeface\tFontObject\tLetterSpacing\tOutlineSize\tText\n");
		int32 TextBlockCount = 0;
		int32 BlueprintCount = 0;

		const TArray<FAssetData> Assets = FindWidgetBlueprints();
		FFileHelper::SaveStringToFile(
			FString::Printf(TEXT("asset-count=%d\n"), Assets.Num()), *ProgressPath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(), FILEWRITE_Append);
		for (const FAssetData& Asset : Assets)
		{
			FFileHelper::SaveStringToFile(
				FString::Printf(TEXT("loading=%s\n"), *Asset.PackageName.ToString()),
				*ProgressPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
				&IFileManager::Get(), FILEWRITE_Append);
			UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(Asset.GetAsset());
			if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
			{
				continue;
			}

			++BlueprintCount;
			Blueprint->WidgetTree->ForEachWidget([&Output, &Asset, &TextBlockCount](UWidget* Widget)
			{
				const UTextBlock* TextBlock = Cast<UTextBlock>(Widget);
				if (TextBlock == nullptr)
				{
					return;
				}

				const FSlateFontInfo Font = TextBlock->GetFont();
				const UObject* FontObject = Font.FontObject;
				Output += FString::Printf(TEXT("%s\t%s\t%.3f\t%s\t%s\t%d\t%d\t%s\n"),
					*Asset.PackageName.ToString(),
					*TextBlock->GetName(),
					Font.Size,
					*SanitizeField(Font.TypefaceFontName.ToString()),
					FontObject != nullptr ? *FontObject->GetPathName() : TEXT(""),
					Font.LetterSpacing,
					Font.OutlineSettings.OutlineSize,
					*SanitizeField(TextBlock->GetText().ToString()));
				++TextBlockCount;
			});
		}

		if (!FFileHelper::SaveStringToFile(Output, *OutputPath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_WIDGET_FONT_AUDIT export failed path=%s"), *OutputPath);
			return;
		}
		FFileHelper::SaveStringToFile(TEXT("complete\n"), *ProgressPath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(), FILEWRITE_Append);

		UE_LOG(LogTemp, Display,
			TEXT("RD_WIDGET_FONT_AUDIT export success blueprints=%d text_blocks=%d path=%s"),
			BlueprintCount, TextBlockCount, *OutputPath);
	}

	void Apply(const TArray<FString>& Args)
	{
		if (Args.IsEmpty())
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_WIDGET_FONT_AUDIT usage: RD.Editor.ApplyWidgetFontSizes <absolute-tsv-path>"));
			return;
		}

		FString Input;
		if (!FFileHelper::LoadFileToString(Input, *Args[0]))
		{
			UE_LOG(LogTemp, Error, TEXT("RD_WIDGET_FONT_AUDIT cannot read %s"), *Args[0]);
			return;
		}

		TMap<FString, TMap<FName, float>> RequestedSizes;
		TArray<FString> Lines;
		Input.ParseIntoArrayLines(Lines, true);
		for (int32 LineIndex = 1; LineIndex < Lines.Num(); ++LineIndex)
		{
			TArray<FString> Fields;
			Lines[LineIndex].ParseIntoArray(Fields, TEXT("\t"), false);
			if (Fields.Num() < 3)
			{
				continue;
			}
			RequestedSizes.FindOrAdd(Fields[0]).Add(FName(Fields[1]), FCString::Atof(*Fields[2]));
		}

		int32 ChangedBlueprints = 0;
		int32 ChangedTextBlocks = 0;
		for (const TPair<FString, TMap<FName, float>>& AssetRequest : RequestedSizes)
		{
			UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
				*FString::Printf(TEXT("%s.%s"), *AssetRequest.Key,
					*FPackageName::GetLongPackageAssetName(AssetRequest.Key)));
			if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("RD_WIDGET_FONT_AUDIT missing blueprint %s"),
					*AssetRequest.Key);
				continue;
			}

			int32 BlueprintChanges = 0;
			for (const TPair<FName, float>& WidgetRequest : AssetRequest.Value)
			{
				UTextBlock* TextBlock = Cast<UTextBlock>(
					Blueprint->WidgetTree->FindWidget(WidgetRequest.Key));
				if (TextBlock == nullptr)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("RD_WIDGET_FONT_AUDIT missing text block asset=%s widget=%s"),
						*AssetRequest.Key, *WidgetRequest.Key.ToString());
					continue;
				}

				FSlateFontInfo Font = TextBlock->GetFont();
				if (Font.Size == WidgetRequest.Value)
				{
					continue;
				}
				Font.Size = WidgetRequest.Value;
				TextBlock->SetFont(Font);
				++BlueprintChanges;
				++ChangedTextBlocks;
			}

			if (BlueprintChanges == 0)
			{
				continue;
			}

			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			const FString Filename = FPackageName::LongPackageNameToFilename(
				Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
			if (!UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
				FSavePackageArgs()))
			{
				UE_LOG(LogTemp, Error, TEXT("RD_WIDGET_FONT_AUDIT save failed asset=%s"),
					*AssetRequest.Key);
				continue;
			}
			++ChangedBlueprints;
		}

		UE_LOG(LogTemp, Display,
			TEXT("RD_WIDGET_FONT_AUDIT apply success blueprints=%d text_blocks=%d"),
			ChangedBlueprints, ChangedTextBlocks);
	}

	void AddFontContract(TMap<FString, TMap<FName, float>>& Contracts,
		const TCHAR* AssetPath, const TCHAR* WidgetName, const float BaseSize)
	{
		Contracts.FindOrAdd(AssetPath).Add(FName(WidgetName), BaseSize);
	}

	void RestoreLegacyMobileScale()
	{
		TMap<FString, TMap<FName, float>> Contracts;
		constexpr const TCHAR* HireAsset =
			TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound");
		constexpr const TCHAR* CombatAsset =
			TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04");

		AddFontContract(Contracts, HireAsset, TEXT("HireTitleText"), 42.f);
		AddFontContract(Contracts, HireAsset, TEXT("HireDetailName"), 38.f);
		AddFontContract(Contracts, HireAsset, TEXT("HireDetailHP"), 24.f);
		AddFontContract(Contracts, HireAsset, TEXT("HireDetailAP"), 24.f);
		AddFontContract(Contracts, HireAsset, TEXT("HireDetailSpeed"), 24.f);
		AddFontContract(Contracts, HireAsset, TEXT("HireAddLabel"), 32.f);
		AddFontContract(Contracts, HireAsset, TEXT("PartyCountText"), 32.f);
		AddFontContract(Contracts, HireAsset, TEXT("DepartLabel"), 32.f);
		AddFontContract(Contracts, HireAsset, TEXT("HireBackLabel"), 32.f);
		for (int32 Index = 0; Index < 6; ++Index)
		{
			AddFontContract(Contracts, HireAsset,
				*FString::Printf(TEXT("HireName_%d"), Index), 29.f);
			AddFontContract(Contracts, HireAsset,
				*FString::Printf(TEXT("HireRole_%d"), Index), 16.f);
			AddFontContract(Contracts, HireAsset,
				*FString::Printf(TEXT("HireDetailSkillText_%d"), Index), 18.f);
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			AddFontContract(Contracts, HireAsset,
				*FString::Printf(TEXT("PartySlotName_%d"), Index), 26.f);
			AddFontContract(Contracts, HireAsset,
				*FString::Printf(TEXT("PartySlotLevel_%d"), Index), 16.f);
		}

		struct FNamedSize
		{
			const TCHAR* Name;
			float BaseSize;
		};
		const FNamedSize CombatText[] = {
			{ TEXT("EnemyBadgeText"), 24.f }, { TEXT("EnemyName"), 38.f },
			{ TEXT("EnemyHPText"), 27.f }, { TEXT("EnemyAPText"), 29.f },
			{ TEXT("EnemySpeedText"), 29.f }, { TEXT("EnemyStatusLabel"), 26.f },
			{ TEXT("EnemyStatus"), 22.f }, { TEXT("EnemySummaryHint"), 25.f },
			{ TEXT("AllyBadgeText"), 24.f }, { TEXT("AllyName"), 38.f },
			{ TEXT("AllyHPText"), 27.f }, { TEXT("AllyAPText"), 29.f },
			{ TEXT("AllySpeedText"), 29.f }, { TEXT("AllyStatusLabel"), 26.f },
			{ TEXT("AllyStatus"), 22.f }, { TEXT("AllySummaryHint"), 25.f },
			{ TEXT("MercenaryInventoryTabText"), 27.f },
			{ TEXT("MercenaryInventoryTitle"), 38.f },
			{ TEXT("MercenaryInventoryGoldLabel"), 28.f },
			{ TEXT("MercenaryInventoryGoldText"), 24.f },
			{ TEXT("MercenaryInventoryDescriptionText"), 20.f },
			{ TEXT("MercenaryTitleText"), 54.f },
			{ TEXT("MercenaryGoldLabel"), 28.f },
			{ TEXT("MercenaryGoldText"), 42.f },
			{ TEXT("MercenaryCloseText"), 34.f },
			{ TEXT("MercenaryDetailName"), 42.f },
			{ TEXT("MercenaryCritLabel"), 27.f },
			{ TEXT("MercenaryCritValue"), 30.f },
			{ TEXT("MercenarySkillHeading"), 32.f },
		};
		for (const FNamedSize& Entry : CombatText)
		{
			AddFontContract(Contracts, CombatAsset, Entry.Name, Entry.BaseSize);
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			AddFontContract(Contracts, CombatAsset,
				*FString::Printf(TEXT("EnemyStatusCount_%d"), Index), 20.f);
			AddFontContract(Contracts, CombatAsset,
				*FString::Printf(TEXT("AllyStatusCount_%d"), Index), 20.f);
			AddFontContract(Contracts, CombatAsset,
				*FString::Printf(TEXT("PartyName_%d"), Index), 27.f);
			AddFontContract(Contracts, CombatAsset,
				*FString::Printf(TEXT("PartyHPText_%d"), Index), 20.f);
			AddFontContract(Contracts, CombatAsset,
				*FString::Printf(TEXT("PartyAPText_%d"), Index), 21.f);
			AddFontContract(Contracts, CombatAsset,
				*FString::Printf(TEXT("PartyStatus_%d"), Index), 17.f);
			AddFontContract(Contracts, CombatAsset,
				*FString::Printf(TEXT("MercenaryChip%dLabel"), Index), 27.f);
		}
		for (const TCHAR* ValueName : { TEXT("MercenaryDetailHP"),
			TEXT("MercenaryDetailAP"), TEXT("MercenaryDetailSpeed") })
		{
			AddFontContract(Contracts, CombatAsset, ValueName, 30.f);
		}
		for (int32 Index = 0; Index < 7; ++Index)
		{
			AddFontContract(Contracts, CombatAsset,
				*FString::Printf(TEXT("MercenaryInventoryArtifactName_%d"), Index), 17.f);
		}
		for (int32 Index = 0; Index < 6; ++Index)
		{
			AddFontContract(Contracts, CombatAsset,
				*FString::Printf(TEXT("MercenarySkillName_%d"), Index), 18.f);
			AddFontContract(Contracts, CombatAsset,
				*FString::Printf(TEXT("MercenarySkillCost_%d"), Index), 20.f);
		}

		int32 ChangedBlueprints = 0;
		int32 ChangedTextBlocks = 0;
		int32 PreservedOverrides = 0;
		for (const TPair<FString, TMap<FName, float>>& AssetContract : Contracts)
		{
			UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
				*FString::Printf(TEXT("%s.%s"), *AssetContract.Key,
					*FPackageName::GetLongPackageAssetName(AssetContract.Key)));
			if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
			{
				continue;
			}

			int32 BlueprintChanges = 0;
			for (const TPair<FName, float>& WidgetContract : AssetContract.Value)
			{
				UTextBlock* TextBlock = Cast<UTextBlock>(
					Blueprint->WidgetTree->FindWidget(WidgetContract.Key));
				if (TextBlock == nullptr)
				{
					continue;
				}

				FSlateFontInfo Font = TextBlock->GetFont();
				const float EnlargedSize = FMath::RoundToFloat(
					WidgetContract.Value * LegacyMobileFontScale);
				if (!FMath::IsNearlyEqual(Font.Size, EnlargedSize))
				{
					++PreservedOverrides;
					continue;
				}
				Font.Size = WidgetContract.Value;
				TextBlock->SetFont(Font);
				++BlueprintChanges;
				++ChangedTextBlocks;
			}

			if (BlueprintChanges == 0)
			{
				continue;
			}
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			const FString Filename = FPackageName::LongPackageNameToFilename(
				Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
			if (UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
				FSavePackageArgs()))
			{
				++ChangedBlueprints;
			}
		}

		UE_LOG(LogTemp, Display,
			TEXT("RD_WIDGET_FONT_AUDIT mobile restore success blueprints=%d text_blocks=%d preserved_overrides=%d"),
			ChangedBlueprints, ChangedTextBlocks, PreservedOverrides);
	}

	bool WrapTextInDownOnlyScaleBox(UWidgetBlueprint* Blueprint,
		const FName TextName)
	{
		UTextBlock* Text = Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(TextName));
		if (Text == nullptr)
		{
			return false;
		}
		// 세로로는 자르지 않는다. 가로는 ScaleToFitX + DownOnly 가 이미 막고
		// 있어서, 자르기가 하는 일은 아래꼬리와 외곽선을 1~2px 깎는 것뿐이었다
		// (0824 검수: "글자 짤리는거"). 런타임의
		// URDUserWidget::NormalizeAutoFitTextClipping 과 같은 값이다.
		constexpr EWidgetClipping AutoFitClipping = EWidgetClipping::Inherit;

		// 이 명령은 여러 번 실행해도 같은 결과가 나와야 한다. 예전 구현은 이미
		// AutoFit 안에 든 TextBlock을 통째로 건너뛰어, 과거의 RenderTransform,
		// Padding, 정렬 값이 그대로 남았다. 기존 래퍼도 신규 래퍼와 똑같이
		// 표준화한다.
		if (UScaleBox* ExistingScale = Cast<UScaleBox>(Text->GetParent()))
		{
			ExistingScale->Modify();
			if (ExistingScale->Slot != nullptr)
			{
				ExistingScale->Slot->Modify();
			}
			Text->Modify();
			if (Text->Slot != nullptr)
			{
				Text->Slot->Modify();
			}
			ExistingScale->SetStretch(EStretch::ScaleToFitX);
			ExistingScale->SetStretchDirection(EStretchDirection::DownOnly);
			ExistingScale->SetClipping(AutoFitClipping);
			ExistingScale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (UOverlaySlot* OuterOverlay = Cast<UOverlaySlot>(ExistingScale->Slot))
			{
				OuterOverlay->SetPadding(FMargin(0.f));
				OuterOverlay->SetHorizontalAlignment(HAlign_Fill);
				OuterOverlay->SetVerticalAlignment(VAlign_Fill);
			}
			else if (UButtonSlot* OuterButton = Cast<UButtonSlot>(ExistingScale->Slot))
			{
				OuterButton->SetPadding(FMargin(0.f));
				OuterButton->SetHorizontalAlignment(HAlign_Fill);
				OuterButton->SetVerticalAlignment(VAlign_Fill);
			}
			if (UScaleBoxSlot* TextSlot = Cast<UScaleBoxSlot>(Text->Slot))
			{
				TextSlot->SetHorizontalAlignment(HAlign_Center);
				TextSlot->SetVerticalAlignment(VAlign_Center);
			}
			Text->SetMargin(FMargin(0.f));
			Text->SetJustification(ETextJustify::Center);
			Text->SetClipping(AutoFitClipping);
			if (UWidget* Center = ExistingScale->GetParent())
			{
				Center->SetClipping(AutoFitClipping);
			}
			Text->SetRenderTransform(FWidgetTransform());
			Text->SetRenderTransformPivot(FVector2D(.5f));
			Text->SetAutoWrapText(false);
			Text->SetMinDesiredWidth(0.f);
			return true;
		}

		UPanelWidget* Parent = Text->GetParent();
		UPanelSlot* OriginalSlot = Text->Slot;
		if (Parent == nullptr || OriginalSlot == nullptr)
		{
			return false;
		}

		const int32 ChildIndex = Parent->GetChildIndex(Text);
		UPanelSlot* SlotTemplate = DuplicateObject<UPanelSlot>(
			OriginalSlot, GetTransientPackage());
		if (ChildIndex == INDEX_NONE || SlotTemplate == nullptr)
		{
			return false;
		}

		const FName ScaleName(*FString::Printf(TEXT("%s_AutoFit"), *TextName.ToString()));
		UScaleBox* Scale = Cast<UScaleBox>(Blueprint->WidgetTree->FindWidget(ScaleName));
		if (Scale == nullptr)
		{
			Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
				UScaleBox::StaticClass(), ScaleName);
			Blueprint->OnVariableAdded(ScaleName);
		}

		Parent->RemoveChildAt(ChildIndex);
		UPanelSlot* NewOuterSlot = Parent->InsertChildAt(
			ChildIndex, Scale, SlotTemplate);
		if (NewOuterSlot == nullptr)
		{
			return false;
		}
		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(NewOuterSlot))
		{
			// 중앙 정렬된 TextBlock의 desired size가 아니라 부모가 준 전체 상자를
			// 축소 판단 기준으로 사용한다.
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}
		else if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(NewOuterSlot))
		{
			ButtonSlot->SetPadding(FMargin(0.f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Fill);
		}

		Scale->SetStretch(EStretch::ScaleToFitX);
		Scale->SetStretchDirection(EStretchDirection::DownOnly);
		Scale->SetClipping(AutoFitClipping);
		Scale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Scale->SetContent(Text);
		if (UScaleBoxSlot* TextSlot = Cast<UScaleBoxSlot>(Text->Slot))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Center);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		Text->SetMargin(FMargin(0.f));
		Text->SetJustification(ETextJustify::Center);
		Text->SetRenderTransform(FWidgetTransform());
		Text->SetRenderTransformPivot(FVector2D(.5f));
		Text->SetAutoWrapText(false);
		Text->SetMinDesiredWidth(0.f);
		return true;
	}

	void AddNames(TMap<FString, TSet<FName>>& Targets, const TCHAR* AssetPath,
		std::initializer_list<const TCHAR*> Names)
	{
		TSet<FName>& AssetTargets = Targets.FindOrAdd(AssetPath);
		for (const TCHAR* Name : Names)
		{
			AssetTargets.Add(FName(Name));
		}
	}

	bool EnsureMinimumCanvasSize(UWidgetBlueprint* Blueprint,
		const FName WidgetName, const FVector2D MinimumSize)
	{
		UWidget* Host = Blueprint->WidgetTree->FindWidget(WidgetName);
		while (Host != nullptr && Cast<UCanvasPanelSlot>(Host->Slot) == nullptr)
		{
			Host = Host->GetParent();
		}
		UCanvasPanelSlot* Slot = Host != nullptr
			? Cast<UCanvasPanelSlot>(Host->Slot) : nullptr;
		if (Slot == nullptr)
		{
			return false;
		}
		const FVector2D OldSize = Slot->GetSize();
		const FVector2D NewSize(FMath::Max(OldSize.X, MinimumSize.X),
			FMath::Max(OldSize.Y, MinimumSize.Y));
		if (OldSize.Equals(NewSize))
		{
			return false;
		}
		Slot->Modify();
		// 현재 버튼 중심은 유지하고 판과 클릭/라벨 영역만 함께 확장한다.
		Slot->SetPosition(Slot->GetPosition() - (NewSize - OldSize) * .5f);
		Slot->SetSize(NewSize);
		return true;
	}

	void ApplyAutoFitText()
	{
		TMap<FString, TSet<FName>> Targets;
		constexpr const TCHAR* HireAsset =
			TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound");
		constexpr const TCHAR* CombatAsset =
			TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04");
		constexpr const TCHAR* DetailAsset =
			TEXT("/Game/UI/CombatDetail/WBP_CombatDetailOverlay");
		constexpr const TCHAR* SkillContentAsset =
			TEXT("/Game/UI/CombatDetail/WBP_SkillDetailContent");
		constexpr const TCHAR* TacticalAsset =
			TEXT("/Game/UI/CombatDetail/SkillTactical/WBP_SkillTacticalDiagram");
		constexpr const TCHAR* MonsterAsset =
			TEXT("/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound");
		constexpr const TCHAR* SettingsAsset = TEXT("/Game/UI/WBP_SettingsPanel");

		AddNames(Targets, HireAsset, { TEXT("HireTitleText"),
			TEXT("HireDetailName"), TEXT("HireAddLabel"), TEXT("PartyCountText"),
			TEXT("DepartLabel"), TEXT("HireBackLabel") });
		for (int32 Index = 0; Index < 6; ++Index)
		{
			Targets.FindOrAdd(HireAsset).Add(
				FName(*FString::Printf(TEXT("HireName_%d"), Index)));
			Targets.FindOrAdd(HireAsset).Add(
				FName(*FString::Printf(TEXT("HireRole_%d"), Index)));
			Targets.FindOrAdd(HireAsset).Add(
				FName(*FString::Printf(TEXT("HireDetailSkillText_%d"), Index)));
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Targets.FindOrAdd(HireAsset).Add(
				FName(*FString::Printf(TEXT("PartySlotName_%d"), Index)));
			Targets.FindOrAdd(HireAsset).Add(
				FName(*FString::Printf(TEXT("PartySlotLevel_%d"), Index)));
		}

		AddNames(Targets, CombatAsset, { TEXT("SkillToggleLabel"),
			TEXT("ConfirmLabel"), TEXT("EndTurnLabel"), TEXT("EnemyName"),
			TEXT("AllyName"), TEXT("EnemyStatus"), TEXT("AllyStatus"),
			TEXT("EnemySummaryHint"), TEXT("AllySummaryHint"),
			TEXT("MercenaryInventoryTabText"), TEXT("MercenaryInventoryTitle"),
			TEXT("MercenaryInventoryDescriptionText"), TEXT("MercenaryTitleText"),
			TEXT("MercenaryCloseText"), TEXT("MercenaryDetailName"),
			TEXT("MercenarySubtitleText") });
		for (int32 Index = 0; Index < 6; ++Index)
		{
			for (const TCHAR* Prefix : { TEXT("CommandName"), TEXT("CommandDamage"),
				TEXT("CommandCooldown"), TEXT("MercenarySkillName") })
			{
				Targets.FindOrAdd(CombatAsset).Add(FName(*FString::Printf(
					TEXT("%s_%d"), Prefix, Index)));
			}
		}
		for (int32 Index = 0; Index < 7; ++Index)
		{
			Targets.FindOrAdd(CombatAsset).Add(FName(*FString::Printf(
				TEXT("MercenaryInventoryArtifactName_%d"), Index)));
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Targets.FindOrAdd(CombatAsset).Add(FName(*FString::Printf(
				TEXT("PartyName_%d"), Index)));
		}

		AddNames(Targets, DetailAsset, { TEXT("DetailTitleText"),
			TEXT("DetailSubtitleText"), TEXT("DetailSkillHeading"),
			TEXT("DetailStatHeading"), TEXT("DetailSelectHeading"),
			TEXT("DetailHitHeading"), TEXT("DetailSelectCaptionText"),
			TEXT("DetailHitCaptionText"), TEXT("DetailBlockerHeading"),
			TEXT("DetailAimBlockerLabel"), TEXT("DetailAimBlockerText"),
			TEXT("DetailEffectBlockerLabel"), TEXT("DetailEffectBlockerText"),
			TEXT("DetailCloseText"), TEXT("DetailExtraHeading") });
		for (int32 Index = 0; Index < 6; ++Index)
		{
			Targets.FindOrAdd(DetailAsset).Add(FName(*FString::Printf(
				TEXT("DetailSkillLabel_%d"), Index)));
		}
		for (int32 Index = 0; Index < 5; ++Index)
		{
			Targets.FindOrAdd(DetailAsset).Add(FName(*FString::Printf(
				TEXT("DetailChip%dLabel"), Index)));
			Targets.FindOrAdd(DetailAsset).Add(FName(*FString::Printf(
				TEXT("DetailChip%dValue"), Index)));
		}
		AddNames(Targets, TacticalAsset, { TEXT("TacticalEffectLegendText"),
			TEXT("TacticalSelectLegendText") });
		AddNames(Targets, SkillContentAsset, { TEXT("SkillSelectRangeText"),
			TEXT("SkillEffectRangeText") });

		AddNames(Targets, MonsterAsset, { TEXT("MonsterBackText"),
			TEXT("MonsterCenterNameText"), TEXT("MonsterDetailNameText"),
			TEXT("MonsterDetailTypeText"), TEXT("MonsterListHeading"),
			TEXT("MonsterSkillHeading"), TEXT("MonsterStatusHeading"),
			TEXT("MonsterTabTitleText") });
		for (int32 Index = 0; Index < 4; ++Index)
		{
			Targets.FindOrAdd(MonsterAsset).Add(FName(*FString::Printf(
				TEXT("MonsterSkillName_%d"), Index)));
			Targets.FindOrAdd(MonsterAsset).Add(FName(*FString::Printf(
				TEXT("MonsterChip%dLabel"), Index)));
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Targets.FindOrAdd(MonsterAsset).Add(FName(*FString::Printf(
				TEXT("MonsterRowName_%d"), Index)));
			Targets.FindOrAdd(MonsterAsset).Add(FName(*FString::Printf(
				TEXT("MonsterRowLevel_%d"), Index)));
		}

		AddNames(Targets, SettingsAsset, { TEXT("SettingsTitleText"),
			TEXT("AudioSectionHeader"), TEXT("DisplaySectionHeader"),
			TEXT("GameplaySectionHeader"), TEXT("MasterVolumeRow_Label"),
			TEXT("BGMVolumeRow_Label"), TEXT("SFXVolumeRow_Label"),
			TEXT("UIVolumeRow_Label"), TEXT("BrightnessRow_Label"),
			TEXT("ScreenShakeRow_Label"), TEXT("VibrationRow_Label"),
			TEXT("EffectsRow_Label"), TEXT("QualityRow_Label"),
			TEXT("FpsRow_Label"), TEXT("LanguageRow_Label"),
			TEXT("AutoEndTurnRow_Label"), TEXT("FastModeRow_Label"),
			TEXT("SkipAnimationRow_Label"), TEXT("BackButtonText"),
			TEXT("ResetButtonText"), TEXT("SaveAndExitButtonText"),
			TEXT("AbandonRunButtonText"), TEXT("AbandonConfirmTitleText"),
			TEXT("ConfirmAbandonButtonText"), TEXT("CancelAbandonButtonText"),
			TEXT("LowQualityButtonText"), TEXT("MediumQualityButtonText"),
			TEXT("HighQualityButtonText"), TEXT("LanguageKoreanButtonText"),
			TEXT("LanguageEnglishButtonText") });

		int32 ChangedBlueprints = 0;
		int32 WrappedTexts = 0;
		for (const TPair<FString, TSet<FName>>& AssetTargets : Targets)
		{
			UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
				*FString::Printf(TEXT("%s.%s"), *AssetTargets.Key,
					*FPackageName::GetLongPackageAssetName(AssetTargets.Key)));
			if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
			{
				continue;
			}
			Blueprint->Modify();
			Blueprint->WidgetTree->Modify();

			// 이름 장부에 빠진 새 버튼도 놓치지 않는다. Xxx_Center 계약이 있거나
			// 버튼의 콘텐츠인 TextBlock은 자동으로 같은 처리 대상에 포함한다.
			TSet<FName> EffectiveTargets = AssetTargets.Value;
			Blueprint->WidgetTree->ForEachWidget(
				[Blueprint, &EffectiveTargets](UWidget* Widget)
				{
					UTextBlock* Text = Cast<UTextBlock>(Widget);
					if (Text == nullptr)
					{
						return;
					}
					const FName CenterName(*FString::Printf(TEXT("%s_Center"),
						*Text->GetName()));
					UWidget* Parent = Text->GetParent();
					const bool bInsideButton = Cast<UButton>(Parent) != nullptr
						|| (Cast<UScaleBox>(Parent) != nullptr
							&& Cast<UButton>(Parent->GetParent()) != nullptr);
					if (Blueprint->WidgetTree->FindWidget(CenterName) != nullptr
						|| bInsideButton)
					{
						EffectiveTargets.Add(Text->GetFName());
					}
				});

			int32 BlueprintChanges = 0;
			for (const FName TextName : EffectiveTargets)
			{
				BlueprintChanges += WrapTextInDownOnlyScaleBox(Blueprint, TextName) ? 1 : 0;
			}
			if (AssetTargets.Key == DetailAsset)
			{
				BlueprintChanges += EnsureMinimumCanvasSize(Blueprint,
					TEXT("DetailCloseButton"), FVector2D(360.f, 96.f)) ? 1 : 0;
			}
			if (BlueprintChanges == 0)
			{
				continue;
			}

			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			const FString Filename = FPackageName::LongPackageNameToFilename(
				Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
			if (UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
				FSavePackageArgs()))
			{
				++ChangedBlueprints;
				WrappedTexts += BlueprintChanges;
			}
		}
		UE_LOG(LogTemp, Display,
			TEXT("RD_WIDGET_TEXT_AUTOFIT success blueprints=%d wrapped_texts=%d"),
			ChangedBlueprints, WrappedTexts);
	}
}

void RegisterWidgetFontAuditCommands()
{
	using namespace WidgetFontAudit;
	ExportCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.ExportWidgetFonts"),
		TEXT("Export every /Game/UI Widget Blueprint TextBlock font size to a TSV."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Export));
	ApplyCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.ApplyWidgetFontSizes"),
		TEXT("Apply TextBlock font sizes from a WidgetFonts TSV without changing layout or text."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Apply));
	RestoreMobileScaleCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RestoreLegacyMobileFontScale"),
		TEXT("Restore only TextBlocks that still exactly match the legacy 1.35 mobile font scale."),
		FConsoleCommandDelegate::CreateStatic(&RestoreLegacyMobileScale));
	ApplyAutoFitCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.ApplyTextAutoFit"),
		TEXT("Wrap bounded one-line UI text in ScaleToFitX/DownOnly ScaleBoxes."),
		FConsoleCommandDelegate::CreateStatic(&ApplyAutoFitText));
}

void UnregisterWidgetFontAuditCommands()
{
	WidgetFontAudit::ExportCommand.Reset();
	WidgetFontAudit::ApplyCommand.Reset();
	WidgetFontAudit::RestoreMobileScaleCommand.Reset();
	WidgetFontAudit::ApplyAutoFitCommand.Reset();
}
