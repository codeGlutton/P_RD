#include "SVNSettings.h"

USVNSettings::USVNSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

FName USVNSettings::GetCategoryName() const
{
	return FName(TEXT("SVN"));
}

#if WITH_EDITOR
FText USVNSettings::GetSectionText() const
{
	return FText::FromString(TEXT("SVN Link Setting"));
}

FText USVNSettings::GetSectionDescription() const
{
	return FText::FromString(TEXT("Various options for connecting SVN"));
}
#endif
