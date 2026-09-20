#include "UI/WidgetVariableCleanup.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

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

	static bool SaveBlueprint(UWidgetBlueprint* Blueprint)
	{
		UPackage* Package = Blueprint->GetOutermost();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Package, Blueprint, *Filename, FSavePackageArgs());
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
			UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
			if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("RD_WIDGET_VAR_CLEAN missing %s"), *AssetPath);
				continue;
			}

			// 지금 트리에 실제로 있는 이름을 먼저 모은다. 이 목록에 든 변수는
			// 절대 건드리지 않는다.
			TSet<FName> LiveNames;
			Blueprint->WidgetTree->ForEachWidget([&LiveNames](UWidget* Widget)
				{
					if (Widget != nullptr)
					{
						LiveNames.Add(Widget->GetFName());
					}
				});

			// 위젯 변수는 NewVariables 가 아니라 WidgetVariableNameToGuidMap 에
			// 산다. 컴파일러는 변수 여부와 무관하게 모든 SourceWidget 이름에 GUID가
			// 있기를 요구한다(WidgetBlueprintCompiler.cpp:794). 파이썬으로 새 위젯을
			// 만들면 이 맵에 자동 등록되지 않으므로, 빠진 쪽도 여기서 채운다.
			int32 Added = 0;
			Blueprint->WidgetTree->ForEachWidget([Blueprint, &Added](UWidget* Widget)
				{
					if (Widget != nullptr &&
						Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()) == false)
					{
						Blueprint->WidgetVariableNameToGuidMap.Add(
							Widget->GetFName(), FGuid::NewDeterministicGuid(Widget->GetPathName()));
						++Added;
					}
				});

			TArray<FName> Stale;
			for (const TPair<FName, FGuid>& Entry : Blueprint->WidgetVariableNameToGuidMap)
			{
				if (LiveNames.Contains(Entry.Key) == false)
				{
					Stale.Add(Entry.Key);
				}
			}
			for (const FName& Name : Stale)
			{
				Blueprint->WidgetVariableNameToGuidMap.Remove(Name);
				Blueprint->OnVariableRemoved(Name);
			}

			// 컴파일러도 이 짝을 스스로 걷어내지만, 걷어낸 결과를 저장하지 않으면
			// 다음 부팅에 그대로 되살아난다. 지운 게 없어도 늘 저장한다.
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			const bool bSaved = SaveBlueprint(Blueprint);
			UE_LOG(LogTemp, Display,
				TEXT("RD_WIDGET_VAR_CLEAN %s live=%d added=%d removed=%d saved=%d"),
				*AssetPath, LiveNames.Num(), Added, Stale.Num(), bSaved ? 1 : 0);
		}
	}
}

void RegisterWidgetVariableCleanupCommands()
{
	using namespace WidgetVariableCleanup;
	CleanCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("RD.Editor.CleanWidgetVariables"),
		TEXT("Drop widget-variable GUIDs whose widget no longer exists. Args: asset paths."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Clean));
}

void UnregisterWidgetVariableCleanupCommands()
{
	WidgetVariableCleanup::CleanCommand.Reset();
}
