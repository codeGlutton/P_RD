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
