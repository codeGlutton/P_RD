#include "UI/MonsterTabWidgetBuilder.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace MonsterTabWidgetBuilder
{
	constexpr TCHAR AssetPath[] =
		TEXT("/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound.WBP_MonsterTab_Marchbound");
	constexpr TCHAR CanonicalRowTexturePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/MonsterTab/T_MT_RowNormal.T_MT_RowNormal");
	constexpr TCHAR CanonicalPortraitFramePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Portrait_Frame.T_KitA_Portrait_Frame");
	TUniquePtr<FAutoConsoleCommand> BuildCommand;

	void Expose(UWidgetBlueprint* Blueprint, UWidget* Widget)
	{
		check(Blueprint != nullptr && Widget != nullptr);
		if (Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()) == false)
		{
			Blueprint->OnVariableAdded(Widget->GetFName());
		}
	}

	bool HasCanonicalModernLayout(UWidgetBlueprint* Blueprint)
	{
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr
			|| Blueprint->WidgetTree->FindWidget(TEXT("MonsterNamePlate")) == nullptr
			|| Blueprint->WidgetTree->FindWidget(TEXT("MonsterSkillDivider")) == nullptr)
		{
			return false;
		}

		const UImage* Row = Cast<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("MonsterRowNormal_0")));
		const UImage* PortraitFrame = Cast<UImage>(
			Blueprint->WidgetTree->FindWidget(TEXT("MonsterPortraitFrame")));
		const UObject* RowResource = Row != nullptr
			? Row->GetBrush().GetResourceObject() : nullptr;
		const UObject* FrameResource = PortraitFrame != nullptr
			? PortraitFrame->GetBrush().GetResourceObject() : nullptr;
		return RowResource != nullptr && FrameResource != nullptr
			&& RowResource->GetPathName() == CanonicalRowTexturePath
			&& FrameResource->GetPathName() == CanonicalPortraitFramePath;
	}

	void StyleTransparentButton(UButton* Button)
	{
		FButtonStyle Style;
		FSlateBrush Empty;
		Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.SetNormal(Empty);
		Style.SetHovered(Empty);
		Style.SetPressed(Empty);
		Style.SetDisabled(Empty);
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::Transparent);
		Button->SetColorAndOpacity(FLinearColor::White);
		Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
		Button->SetClickMethod(EButtonClickMethod::PreciseClick);
		Button->SetVisibility(ESlateVisibility::Visible);
		Button->SetIsEnabled(true);
	}

	/**
	 * Add only the four long-press hit targets required by runtime code.
	 *
	 * The monster tab is a designer-authored asset.  The old implementation removed
	 * its root and rebuilt a legacy three-column screen every time this command ran.
	 * Refuse to touch anything unless the modern texture/name signature is present,
	 * then copy each authored skill socket's exact canvas rectangle to one transparent
	 * button.  Re-running the command is intentionally idempotent.
	 */
	void RepairSkillInputOnly()
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
		if (HasCanonicalModernLayout(Blueprint) == false)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_MONSTER_TAB_REPAIR refused: canonical modern layout signature is missing"));
			return;
		}

		UCanvasPanel* Canvas = Cast<UCanvasPanel>(
			Blueprint->WidgetTree->FindWidget(TEXT("MonsterTabCanvas")));
		if (Canvas == nullptr)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_MONSTER_TAB_REPAIR refused: MonsterTabCanvas is missing"));
			return;
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FName SlotName(*FString::Printf(TEXT("MonsterSkillSlot_%d"), Index));
			const FName ButtonName(*FString::Printf(TEXT("MonsterSkillButton_%d"), Index));
			UImage* SkillSlot = Cast<UImage>(Blueprint->WidgetTree->FindWidget(SlotName));
			UCanvasPanelSlot* AuthoredSlot = SkillSlot != nullptr
				? Cast<UCanvasPanelSlot>(SkillSlot->Slot) : nullptr;
			if (SkillSlot == nullptr || SkillSlot->GetParent() != Canvas
				|| AuthoredSlot == nullptr)
			{
				UE_LOG(LogTemp, Error,
					TEXT("RD_MONSTER_TAB_REPAIR refused: authored socket %s is invalid"),
					*SlotName.ToString());
				return;
			}

			UButton* Button = Cast<UButton>(Blueprint->WidgetTree->FindWidget(ButtonName));
			if (Button == nullptr)
			{
				Button = Blueprint->WidgetTree->ConstructWidget<UButton>(
					UButton::StaticClass(), ButtonName);
			}
			if (Button->GetParent() != Canvas)
			{
				if (UPanelWidget* Parent = Button->GetParent())
				{
					Parent->RemoveChild(Button);
				}
				Canvas->AddChildToCanvas(Button);
			}

			UCanvasPanelSlot* ButtonSlot = CastChecked<UCanvasPanelSlot>(Button->Slot);
			ButtonSlot->SetAnchors(AuthoredSlot->GetAnchors());
			ButtonSlot->SetAlignment(AuthoredSlot->GetAlignment());
			ButtonSlot->SetAutoSize(AuthoredSlot->GetAutoSize());
			ButtonSlot->SetPosition(AuthoredSlot->GetPosition());
			ButtonSlot->SetSize(AuthoredSlot->GetSize());
			ButtonSlot->SetZOrder(24);
			StyleTransparentButton(Button);
			Expose(Blueprint, Button);
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		if (UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
			FSavePackageArgs()) == false)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_MONSTER_TAB_REPAIR save failed"));
			return;
		}

		UE_LOG(LogTemp, Display,
			TEXT("RD_MONSTER_TAB_REPAIR success preserved=modern-canonical added=MonsterSkillButton_0..3"));
	}
}

void RegisterMonsterTabWidgetBuilderCommands()
{
	using namespace MonsterTabWidgetBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.BuildMonsterTab"),
		TEXT("Repair only the four skill hit targets in the authored modern monster tab."),
		FConsoleCommandDelegate::CreateStatic(&RepairSkillInputOnly));
}

void UnregisterMonsterTabWidgetBuilderCommands()
{
	MonsterTabWidgetBuilder::BuildCommand.Reset();
}
