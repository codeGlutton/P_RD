#include "Animation/Section/BoardEventSectionBase.h"

UBoardEventSectionBase::UBoardEventSectionBase()
{
}

#if WITH_EDITOR
void UBoardEventSectionBase::PostRename(UObject* OldOuter, const FName OldName)
{
	if (OldOuter != GetOuter())
	{
		Super::PostRename(OldOuter, OldName);
	}
}

void UBoardEventSectionBase::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
}

void UBoardEventSectionBase::RemoveForCook()
{
	Super::RemoveForCook();
}
#endif
