#include "Component/VFXTimelineComponent/VFXTimelineTrackEvent.h"
#include "NiagaraComponent.h"

void FVFXTimelineTrackEvent::Trigger(float Value, const TArray<TWeakObjectPtr<UPrimitiveComponent>>& MeshComps, const TArray<TWeakObjectPtr<UNiagaraComponent>>& NiagaraComps) const
{
	/* 타겟 메시 CPD 적용 */

	if (EnumHasAnyFlags(EVFXTimelineSyncTarget::PrimitiveData, StaticCast<EVFXTimelineSyncTarget>(mSyncTarget)) == true)
	{
		for (const TWeakObjectPtr<UPrimitiveComponent>& MeshComp : MeshComps)
		{
			if (MeshComp.IsValid() == false)
			{
				continue;
			}

			MeshComp->SetCustomPrimitiveDataFloat(mCPDIndex, Value);
		}
	}

	/* 타겟 나이아가라 User Param 적용 */

	if (EnumHasAnyFlags(EVFXTimelineSyncTarget::NiagaraUserParameter, StaticCast<EVFXTimelineSyncTarget>(mSyncTarget)) == true)
	{
		for (const TWeakObjectPtr<UNiagaraComponent>& NiagaraComp : NiagaraComps)
		{
			if (NiagaraComp.IsValid() == false)
			{
				continue;
			}

			NiagaraComp->SetVariableFloat(mParameterName, Value);
		}
	}
}

void FVFXTimelineTrackEvent::Trigger(const FVector& Value, const TArray<TWeakObjectPtr<UPrimitiveComponent>>& MeshComps, const TArray<TWeakObjectPtr<UNiagaraComponent>>& NiagaraComps) const
{
	/* 타겟 메시 CPD 적용 */

	if (EnumHasAnyFlags(EVFXTimelineSyncTarget::PrimitiveData, StaticCast<EVFXTimelineSyncTarget>(mSyncTarget)) == true)
	{
		for (const TWeakObjectPtr<UPrimitiveComponent>& MeshComp : MeshComps)
		{
			if (MeshComp.IsValid() == false)
			{
				continue;
			}

			MeshComp->SetCustomPrimitiveDataVector3(mCPDIndex, Value);
		}
	}

	/* 타겟 나이아가라 User Param 적용 */

	if (EnumHasAnyFlags(EVFXTimelineSyncTarget::NiagaraUserParameter, StaticCast<EVFXTimelineSyncTarget>(mSyncTarget)) == true)
	{
		for (const TWeakObjectPtr<UNiagaraComponent>& NiagaraComp : NiagaraComps)
		{
			if (NiagaraComp.IsValid() == false)
			{
				continue;
			}

			NiagaraComp->SetVariableVec3(mParameterName, Value);
		}
	}
}

void FVFXTimelineTrackEvent::Trigger(const FLinearColor& Value, const TArray<TWeakObjectPtr<UPrimitiveComponent>>& MeshComps, const TArray<TWeakObjectPtr<UNiagaraComponent>>& NiagaraComps) const
{
	/* 타겟 메시 CPD 적용 */

	if (EnumHasAnyFlags(EVFXTimelineSyncTarget::PrimitiveData, StaticCast<EVFXTimelineSyncTarget>(mSyncTarget)) == true)
	{
		for (const TWeakObjectPtr<UPrimitiveComponent>& MeshComp : MeshComps)
		{
			if (MeshComp.IsValid() == false)
			{
				continue;
			}

			MeshComp->SetCustomPrimitiveDataVector4f(mCPDIndex, Value);
		}
	}

	/* 타겟 나이아가라 User Param 적용 */

	if (EnumHasAnyFlags(EVFXTimelineSyncTarget::NiagaraUserParameter, StaticCast<EVFXTimelineSyncTarget>(mSyncTarget)) == true)
	{
		for (const TWeakObjectPtr<UNiagaraComponent>& NiagaraComp : NiagaraComps)
		{
			if (NiagaraComp.IsValid() == false)
			{
				continue;
			}

			NiagaraComp->SetVariableLinearColor(mParameterName, Value);
		}
	}
}

void FVFXTimelineEventTarget::Clear()
{
	mMeshComps.Empty();
	mNiagaraComps.Empty();
}
