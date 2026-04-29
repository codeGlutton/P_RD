#include "FunctionLibrary/GASTargetFunctionLibrary.h"
#include "Pawn/Unit.h"

FGameplayAbilityTargetDataHandle UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(const AUnit* Source)
{
	FGameplayAbilityTargetDataHandle Handle;
	Handle.Add(Source->MakeSnapshotTargetData());

	return Handle;
}

FGameplayAbilityTargetDataHandle UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(const AUnit* Source, const AUnit* Target)
{
	FGameplayAbilityTargetDataHandle Handle;
	Handle.Add(Source->MakeSnapshotTargetData());
	Handle.Add(Target->MakeSnapshotTargetData());

	return Handle;
}

const FUnitSnapshotTargetData* UGASTargetFunctionLibrary::GetSnapshotTargetData(const FGameplayAbilityTargetDataHandle& Handle, int32 Index)
{
	const FGameplayAbilityTargetData* TargetData = Handle.Get(Index);
	if (TargetData == nullptr)
	{
		return nullptr;
	}

	if (TargetData->GetScriptStruct()->IsChildOf(FUnitSnapshotTargetData::StaticStruct()) == false)
	{
		return nullptr;
	}
	
	const FUnitSnapshotTargetData* SnapshotTargetData = StaticCast<const FUnitSnapshotTargetData*>(TargetData);
	return SnapshotTargetData;
}
