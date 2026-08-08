#include "UI/WidgetTexturePurge.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Engine/Texture2D.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

/*
 * 지울 텍스처를 쓰는 브러시를 위젯 블루프린트에서 찾아 갈아 끼운다.
 *
 * 왜 C++ 인가
 * -----------
 * 파이썬에는 WidgetTree 를 훑는 길이 없다. get_children_count() 가 0 을 준다.
 * 그래서 지금까지는 이름을 아는 위젯만 찍을 수 있었고, 어느 위젯이 어떤 그림을
 * 쓰는지 모르는 경우에는 손을 못 댔다.
 *
 * 엔진의 참조 합치기(consolidate)도 WBP 안의 브러시는 못 바꿨다 -- 실제로
 * 32개가 그대로 남았다. 여기서는 ForEachWidget 으로 직접 훑는다.
 *
 * 어디를 보나
 * -----------
 * 브러시는 위젯 종류마다 다른 자리에 있다. Image 는 하나지만 Button 은 넷,
 * CheckBox 는 여섯이다. 한 자리라도 빠뜨리면 그 참조가 남아 지울 수 없다.
 *
 * 무엇으로 바꾸나
 * ---------------
 * 인자로 준 텍스처. 안 주면 **아무것도 안 그리게**(NoDrawType) 만든다.
 * 그냥 비우면 흰 사각형이 남는다 -- 몬스터탭에서 실제로 그랬다.
 *
 * 위젯을 지우지는 않는다. 이름을 지우면 블루프린트 변수 GUID 가 끊긴다.
 *
 * 쓰기:
 *   RD.Editor.PurgeWidgetTextures <목록파일> [바꿀텍스처경로]
 */
namespace WidgetTexturePurge
{
	static TUniquePtr<FAutoConsoleCommand> PurgeCommand;

	static bool SaveBlueprint(UWidgetBlueprint* Blueprint)
	{
		UPackage* Package = Blueprint->GetOutermost();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Package, Blueprint, *Filename, FSavePackageArgs());
	}

	/** @brief 이 브러시가 지울 그림을 쓰고 있으면 갈아 끼운다. */
	static bool FixBrush(FSlateBrush& Brush, const TSet<FString>& Doomed,
		UTexture2D* Replacement)
	{
		const UObject* Resource = Brush.GetResourceObject();
		if (Resource == nullptr || Doomed.Contains(Resource->GetName()) == false)
		{
			return false;
		}
		if (Replacement != nullptr)
		{
			Brush.SetResourceObject(Replacement);
		}
		else
		{
			Brush.SetResourceObject(nullptr);
			Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
		}
		return true;
	}

	/** @brief 위젯 하나가 들고 있는 브러시를 모두 본다. */
	static int32 FixWidget(UWidget* Widget, const TSet<FString>& Doomed,
		UTexture2D* Replacement)
	{
		int32 Fixed = 0;
		if (UImage* Image = Cast<UImage>(Widget))
		{
			FSlateBrush Brush = Image->GetBrush();
			if (FixBrush(Brush, Doomed, Replacement))
			{
				Image->SetBrush(Brush);
				++Fixed;
			}
		}
		else if (UBorder* Border = Cast<UBorder>(Widget))
		{
			FSlateBrush Brush = Border->Background;
			if (FixBrush(Brush, Doomed, Replacement))
			{
				Border->SetBrush(Brush);
				++Fixed;
			}
		}
		else if (UButton* Button = Cast<UButton>(Widget))
		{
			FButtonStyle Style = Button->GetStyle();
			int32 Local = 0;
			Local += FixBrush(Style.Normal, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.Hovered, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.Pressed, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.Disabled, Doomed, Replacement) ? 1 : 0;
			if (Local > 0)
			{
				Button->SetStyle(Style);
				Fixed += Local;
			}
		}
		else if (UCheckBox* Check = Cast<UCheckBox>(Widget))
		{
			FCheckBoxStyle Style = Check->GetWidgetStyle();
			int32 Local = 0;
			Local += FixBrush(Style.UncheckedImage, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.UncheckedHoveredImage, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.UncheckedPressedImage, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.CheckedImage, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.CheckedHoveredImage, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.CheckedPressedImage, Doomed, Replacement) ? 1 : 0;
			if (Local > 0)
			{
				Check->SetWidgetStyle(Style);
				Fixed += Local;
			}
		}
		else if (USlider* Slider = Cast<USlider>(Widget))
		{
			FSliderStyle Style = Slider->GetWidgetStyle();
			int32 Local = 0;
			Local += FixBrush(Style.NormalBarImage, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.HoveredBarImage, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.NormalThumbImage, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.HoveredThumbImage, Doomed, Replacement) ? 1 : 0;
			if (Local > 0)
			{
				Slider->SetWidgetStyle(Style);
				Fixed += Local;
			}
		}
		else if (UProgressBar* Bar = Cast<UProgressBar>(Widget))
		{
			FProgressBarStyle Style = Bar->GetWidgetStyle();
			int32 Local = 0;
			Local += FixBrush(Style.BackgroundImage, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.FillImage, Doomed, Replacement) ? 1 : 0;
			Local += FixBrush(Style.MarqueeImage, Doomed, Replacement) ? 1 : 0;
			if (Local > 0)
			{
				Bar->SetWidgetStyle(Style);
				Fixed += Local;
			}
		}
		return Fixed;
	}

	static void Purge(const TArray<FString>& Args)
	{
		if (Args.Num() == 0)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_PURGE needs a list file. 두 번째 인자로 바꿀 텍스처 경로를 줄 수 있다."));
			return;
		}

		FString Body;
		if (FFileHelper::LoadFileToString(Body, *Args[0]) == false)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_PURGE cannot read %s"), *Args[0]);
			return;
		}
		TArray<FString> Names;
		Body.ParseIntoArrayLines(Names);
		TSet<FString> Doomed;
		for (FString& Name : Names)
		{
			Name.TrimStartAndEndInline();
			if (Name.IsEmpty() == false && Name.StartsWith(TEXT("#")) == false)
			{
				Doomed.Add(Name);
			}
		}

		UTexture2D* Replacement = nullptr;
		if (Args.Num() > 1)
		{
			Replacement = LoadObject<UTexture2D>(nullptr, *Args[1]);
			if (Replacement == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("RD_PURGE replacement not found: %s"), *Args[1]);
				return;
			}
		}

		FAssetRegistryModule& RegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> Assets;
		RegistryModule.Get().GetAssetsByClass(
			UWidgetBlueprint::StaticClass()->GetClassPathName(), Assets, true);

		int32 TouchedAssets = 0;
		int32 TouchedBrushes = 0;
		for (const FAssetData& Data : Assets)
		{
			UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(Data.GetAsset());
			if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
			{
				continue;
			}
			int32 Fixed = 0;
			Blueprint->WidgetTree->ForEachWidget(
				[&Doomed, Replacement, &Fixed](UWidget* Widget)
				{
					if (Widget != nullptr)
					{
						Fixed += FixWidget(Widget, Doomed, Replacement);
					}
				});
			if (Fixed == 0)
			{
				continue;
			}
			Blueprint->Modify();
			Blueprint->WidgetTree->Modify();
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			const bool bSaved = SaveBlueprint(Blueprint);
			++TouchedAssets;
			TouchedBrushes += Fixed;
			UE_LOG(LogTemp, Display, TEXT("RD_PURGE %s  브러시 %d개  saved=%d"),
				*Data.PackageName.ToString(), Fixed, bSaved ? 1 : 0);
		}

		UE_LOG(LogTemp, Display,
			TEXT("RD_PURGE done  이름 %d종 · 고친 자산 %d개 · 브러시 %d개"),
			Doomed.Num(), TouchedAssets, TouchedBrushes);
	}
}

void RegisterWidgetTexturePurgeCommands()
{
	using namespace WidgetTexturePurge;
	PurgeCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.PurgeWidgetTextures"),
		TEXT("Replace brushes that use listed textures. Args: <list file> [replacement path]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Purge));
}

void UnregisterWidgetTexturePurgeCommands()
{
	WidgetTexturePurge::PurgeCommand.Reset();
}
