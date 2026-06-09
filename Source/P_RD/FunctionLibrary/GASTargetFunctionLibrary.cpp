#include "FunctionLibrary/GASTargetFunctionLibrary.h"
#include "Pawn/Unit.h"

FGameplayAbilityTargetDataHandle UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(const ITileTargetable* Source)
{
	FGameplayAbilityTargetDataHandle Handle;
	Handle.Add(Source->MakeSnapshotTargetData());

	return Handle;
}

FGameplayAbilityTargetDataHandle UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(const ITileTargetable* Source, const ITileTargetable* Target)
{
	FGameplayAbilityTargetDataHandle Handle;
	Handle.Add(Source->MakeSnapshotTargetData());
	Handle.Add(Target->MakeSnapshotTargetData());

	return Handle;
}

const FTileTargetSnapshotTargetData* UGASTargetFunctionLibrary::GetSnapshotTargetData(const FGameplayAbilityTargetDataHandle& Handle, int32 Index)
{
	const FGameplayAbilityTargetData* TargetData = Handle.Get(Index);
	if (TargetData == nullptr)
	{
		return nullptr;
	}

	if (TargetData->GetScriptStruct()->IsChildOf(FTileTargetSnapshotTargetData::StaticStruct()) == false)
	{
		return nullptr;
	}
	
	const FTileTargetSnapshotTargetData* SnapshotTargetData = StaticCast<const FTileTargetSnapshotTargetData*>(TargetData);
	return SnapshotTargetData;
}
