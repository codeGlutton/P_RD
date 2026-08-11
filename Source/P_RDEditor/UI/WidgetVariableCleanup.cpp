#include "UI/WidgetVariableCleanup.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"

/*
 * 위젯을 지운 뒤 남은 변수 GUID 를 걷어낸다.
 *
 * 왜 필요한가
 * -----------
 * 배치를 파이썬으로 다시 짤 때 ``WidgetTree`` 의 자식을 통째로 갈아 끼운다.
 * 그러면 옛 위젯 객체는 사라지는데 블루프린트의 **변수 목록**에는 그 이름의
 * GUID 가 그대로 남는다. 컴파일할 때마다
 *
 *     Variable [X] was deleted but still has a GUID referenced by WidgetBlueprint
 *
 * 가 이름 수만큼 뜬다(설정 패널 재배치 직후 실측 98개). 게임은 돌지만 매 부팅
 * 로그가 이 줄로 덮여 진짜 문제를 못 본다.
 *
 * 파이썬에는 이 GUID 를 지우는 길이 없다. ``UWidgetBlueprint::OnVariableRemoved``
 * 는 C++ 전용이라, 에디터 명령 하나로 열어 준다.
 *
 * 안전 장치
 * ---------
 * **트리에 같은 이름의 위젯이 실제로 없을 때만 지운다.** 살아 있는 위젯의
 * 변수를 지우면 그 위젯이 블루프린트 그래프에서 사라져 배선이 끊긴다.
 */
namespace WidgetVariableCleanup
{
	static TUniquePtr<FAutoConsoleCommand> CleanCommand;
	static TUniquePtr<FAutoConsoleCommand> RemoveWidgetsCommand;

	static bool SaveBlueprint(UWidgetBlueprint* Blueprint)
	{
		UPackage* Package = Blueprint->GetOutermost();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Package, Blueprint, *Filename, FSavePackageArgs());
	}

	static UWidgetBlueprint* LoadBlueprint(const FString& AssetPath)
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
		if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("RD_WIDGET_VAR_CLEAN missing %s"), *AssetPath);
			return nullptr;
		}
		return Blueprint;
	}

	static void RepairVariablesAndSave(UWidgetBlueprint* Blueprint,
		const FString& AssetPath, const int32 RemovedWidgets = 0)
	{
		TSet<FName> LiveNames;
		Blueprint->ForEachSourceWidget([&LiveNames](UWidget* Widget)
		{
			if (Widget != nullptr)
			{
				LiveNames.Add(Widget->GetFName());
			}
		});
		for (const UWidgetAnimation* Animation : Blueprint->Animations)
		{
			if (Animation != nullptr)
			{
				LiveNames.Add(Animation->GetFName());
			}
		}

		int32 Added = 0;
		Blueprint->ForEachSourceWidget([Blueprint, &Added](UWidget* Widget)
		{
			if (Widget != nullptr
				&& !Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
			{
				Blueprint->WidgetVariableNameToGuidMap.Add(
					Widget->GetFName(), FGuid::NewDeterministicGuid(Widget->GetPathName()));
				++Added;
			}
		});
		for (const UWidgetAnimation* Animation : Blueprint->Animations)
		{
			if (Animation != nullptr
				&& !Blueprint->WidgetVariableNameToGuidMap.Contains(Animation->GetFName()))
			{
				Blueprint->WidgetVariableNameToGuidMap.Add(
					Animation->GetFName(), FGuid::NewDeterministicGuid(Animation->GetPathName()));
				++Added;
			}
		}

		TArray<FName> StaleNames;
		for (const TPair<FName, FGuid>& Entry : Blueprint->WidgetVariableNameToGuidMap)
		{
			if (!LiveNames.Contains(Entry.Key))
			{
				StaleNames.Add(Entry.Key);
			}
		}
		for (const FName Name : StaleNames)
		{
			Blueprint->OnVariableRemoved(Name);
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const bool bSaved = SaveBlueprint(Blueprint);
		UE_LOG(LogTemp, Display,
			TEXT("RD_WIDGET_VAR_CLEAN %s live=%d added=%d stale=%d widgets=%d saved=%d"),
			*AssetPath, LiveNames.Num(), Added, StaleNames.Num(), RemovedWidgets,
			bSaved ? 1 : 0);
	}

	static void Clean(const TArray<FString>& Args)
	{
		if (Args.Num() == 0)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_WIDGET_VAR_CLEAN needs an asset path, e.g. /Game/UI/WBP_SettingsPanel"));
			return;
		}

		for (const FString& AssetPath : Args)
		{
			if (UWidgetBlueprint* Blueprint = LoadBlueprint(AssetPath))
			{
				RepairVariablesAndSave(Blueprint, AssetPath);
			}
		}
	}

	static void RemoveWidgets(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogTemp, Error,
				TEXT("RD_WIDGET_REMOVE needs an asset path followed by widget names."));
			return;
		}

		UWidgetBlueprint* Blueprint = LoadBlueprint(Args[0]);
		if (Blueprint == nullptr)
		{
			return;
		}

		TSet<UWidget*> WidgetsToRemove;
		for (int32 Index = 1; Index < Args.Num(); ++Index)
		{
			if (UWidget* Widget = Blueprint->WidgetTree->FindWidget(FName(*Args[Index])))
			{
				WidgetsToRemove.Add(Widget);
			}
		}

		FWidgetBlueprintEditorUtils::DeleteWidgets(
			Blueprint,
			WidgetsToRemove,
			FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		RepairVariablesAndSave(Blueprint, Args[0], WidgetsToRemove.Num());
	}
}

void RegisterWidgetVariableCleanupCommands()
{
	using namespace WidgetVariableCleanup;
	CleanCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.CleanWidgetVariables"),
		TEXT("Drop widget-variable GUIDs whose widget no longer exists. Args: asset paths."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Clean));
	RemoveWidgetsCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.RemoveWidgets"),
		TEXT("Remove named widgets and stale variable GUIDs. Args: asset path, widget names."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&RemoveWidgets));
}

void UnregisterWidgetVariableCleanupCommands()
{
	WidgetVariableCleanup::RemoveWidgetsCommand.Reset();
	WidgetVariableCleanup::CleanCommand.Reset();
}
