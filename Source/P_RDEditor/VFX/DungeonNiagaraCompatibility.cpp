#include "VFX/DungeonNiagaraCompatibility.h"

#include "NiagaraNodeFunctionCall.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"
#include "P_RDEditor.h"
#include "UObject/ICookInfo.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

namespace
{
	constexpr TCHAR DungeonSystemPath[] = TEXT("/Game/SVN/OutSideAsset/Background/Fantastic_Dungeon_Pack/effects/PS_FX_particles_dungeon_01_Niagara.PS_FX_particles_dungeon_01_Niagara");
	constexpr TCHAR LegacyNodeSuffix[] = TEXT(":Particles_Big_0.NiagaraScriptSource_0.NiagaraGraph_0.NiagaraNodeFunctionCall_10");
	constexpr TCHAR ConverterModulePath[] = TEXT("/CascadeToNiagaraConverter/NiagaraScripts/ModuleScripts/AddVectorToPosition.AddVectorToPosition");
	const FGuid LegacyNodeGuid(0x956B1183, 0x4188EF5B, 0x643E8E97, 0x59757231);
	FDelegateHandle AssetLoadedHandle;

	UNiagaraNodeFunctionCall* FindLegacyNode(const UNiagaraSystem* System)
	{
		return System ? FindObject<UNiagaraNodeFunctionCall>(nullptr, *(System->GetPathName() + LegacyNodeSuffix)) : nullptr;
	}

	bool RepairDungeonSystem(UNiagaraSystem* System)
	{
		if (!System || System->GetPathName() != DungeonSystemPath) return false;
		UNiagaraNodeFunctionCall* Node = FindLegacyNode(System);
		if (!Node || Node->NodeGuid != LegacyNodeGuid || Node->FunctionScript) return false;

		// This is the converter's parameter-map module, not the similarly named dynamic input.
		UNiagaraScript* Module = nullptr;
		{
			// Only the compiled system ships; this standalone module supplies editor graph source.
			FCookLoadScope EditorOnlyLoad(ECookLoadType::EditorOnly);
			Module = LoadObject<UNiagaraScript>(nullptr, ConverterModulePath);
		}
		if (!Module)
		{
			UE_LOG(LogRDEditor, Error, TEXT("Dungeon Niagara requires the editor-only CascadeToNiagaraConverter module: %s"), ConverterModulePath);
			return false;
		}

		const bool bPackageWasDirty = System->GetOutermost()->IsDirty();
		Node->FunctionScript = Module;
		FPropertyChangedEvent ChangedProperty(FindFProperty<FObjectPropertyBase>(
			UNiagaraNodeFunctionCall::StaticClass(), GET_MEMBER_NAME_CHECKED(UNiagaraNodeFunctionCall, FunctionScript)));
		Node->PostEditChangeProperty(ChangedProperty);
		System->RequestCompile(true);
		System->WaitForCompilationComplete(false, false);
		// The correction is regenerated on each editor/cook load. Do not prompt to save SVN content.
		if (!bPackageWasDirty) System->GetOutermost()->SetDirtyFlag(false);
		UE_LOG(LogRDEditor, Display, TEXT("Restored legacy dungeon Niagara module in memory: %s (valid=%d)"),
			*System->GetPathName(), System->IsValid());
		return true;
	}

	void OnAssetLoaded(UObject* Asset)
	{
		RepairDungeonSystem(Cast<UNiagaraSystem>(Asset));
	}
}

void RegisterDungeonNiagaraCompatibility()
{
	if (!AssetLoadedHandle.IsValid())
	{
		AssetLoadedHandle = FCoreUObjectDelegates::OnAssetLoaded.AddStatic(&OnAssetLoaded);
		// Also support module reload after the asset has already been opened.
		RepairDungeonSystem(FindObject<UNiagaraSystem>(nullptr, DungeonSystemPath));
	}
}

void UnregisterDungeonNiagaraCompatibility()
{
	FCoreUObjectDelegates::OnAssetLoaded.Remove(AssetLoadedHandle);
	AssetLoadedHandle.Reset();
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonNiagaraCompatibilityTest,
	"P_RD.Editor.Niagara.DungeonLegacyModuleRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDungeonNiagaraCompatibilityTest::RunTest(const FString& Parameters)
{
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, DungeonSystemPath);
	if (!TestNotNull(TEXT("Shipping Stage 3 ambient system loads"), System)) return false;
	UNiagaraNodeFunctionCall* Node = FindLegacyNode(System);
	if (!TestNotNull(TEXT("Known legacy call exists"), Node)) return false;
	UNiagaraScript* ExistingScript = Node->FunctionScript;
	if (!TestNotNull(TEXT("Load hook restores the missing module"), ExistingScript)) return false;
	TestEqual(TEXT("Correct parameter-map module is restored"), ExistingScript->GetPathName(), FString(ConverterModulePath));
	TestTrue(TEXT("Converter graph module is editor-only"), ExistingScript->IsEditorOnly());
	TestTrue(TEXT("Restored system compiles successfully"), System->IsValid());
	TestFalse(TEXT("Existing script reference is never replaced"), RepairDungeonSystem(System));
	TestTrue(TEXT("Existing reference remains identical"), Node->FunctionScript == ExistingScript);

	UNiagaraSystem* UnrelatedSystem = NewObject<UNiagaraSystem>(GetTransientPackage());
	TestFalse(TEXT("Unrelated systems remain untouched"), RepairDungeonSystem(UnrelatedSystem));
	return true;
}
#endif
