#include "Setting/GamePlayType.h"

FNiagaraSpawnData FSoftNiagaraSpawnData::LoadSynchronous() const
{
	FNiagaraSpawnData SpawnData;
	SpawnData.mSocketName = mSocketName;
	SpawnData.mRelativeTransform = mRelativeTransform;
	SpawnData.mAttached = mAttached;

	if (mNiagaraSystem.IsNull() == false)
	{
		SpawnData.mNiagaraSystem = mNiagaraSystem.LoadSynchronous();
	}

	return SpawnData;
}

FVFXSpawnData FSoftVFXSpawnData::LoadSynchronous() const
{
	FVFXSpawnData VFXSpawnData;
	VFXSpawnData.mTimelineExecutionDatas = mTimelineExecutionDatas;
	VFXSpawnData.mIncludeSpawnedNiagaraInTimeline = mIncludeSpawnedNiagaraInTimeline;

	for (const FSoftNiagaraSpawnData& NiagaraSpawnData : mNiagaraSpawnDatas)
	{
		VFXSpawnData.mNiagaraSpawnDatas.Add(NiagaraSpawnData.LoadSynchronous());
	}

	return VFXSpawnData;
}

