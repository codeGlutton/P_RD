#include "UI/CharacterSelectWidgetPrivate.h"

#include "Internationalization/Internationalization.h"

#define LOCTEXT_NAMESPACE "CharacterSelectWidgetPrivate"

namespace RDCharacterSelect
{
	/** @brief 선택 화면 C++ 기본 문구를 키 기반으로 반환하고 알 수 없는 키는 디버깅 가능한 원문으로 돌려준다. */
	FText Text(const TCHAR* Key)
	{
		if (FCString::Strcmp(Key, TEXT("ConfirmText")) == 0)
		{
			return LOCTEXT("Confirm", "Confirm");
		}
		if (FCString::Strcmp(Key, TEXT("BackText")) == 0)
		{
			return LOCTEXT("Back", "Back");
		}
		if (FCString::Strcmp(Key, TEXT("ReadyStatusText")) == 0)
		{
			return FText::GetEmpty();
		}
		if (FCString::Strcmp(Key, TEXT("LoadingStatusText")) == 0)
		{
			return LOCTEXT("Loading", "Loading");
		}
		if (FCString::Strcmp(Key, TEXT("FailedStatusText")) == 0)
		{
			return LOCTEXT("Failed", "Failed");
		}
		if (FCString::Strcmp(Key, TEXT("NoCharacterStatusText")) == 0)
		{
			return LOCTEXT("No character data", "No character data");
		}
		if (FCString::Strcmp(Key, TEXT("CharacterSelectText")) == 0)
		{
			return LOCTEXT("Character Select", "Character Select");
		}
		if (FCString::Strcmp(Key, TEXT("SelectedCharacterFormat")) == 0)
		{
			return LOCTEXT("{0} selected", "{0} selected");
		}
		if (FCString::Strcmp(Key, TEXT("CharacterLockedStatus")) == 0)
		{
			return LOCTEXT("This character is not available", "This character is not available");
		}
		if (FCString::Strcmp(Key, TEXT("PortraitFallbackText")) == 0)
		{
			return LOCTEXT("No portrait", "No portrait");
		}
		if (FCString::Strcmp(Key, TEXT("CharacterStatFormat")) == 0)
		{
			return LOCTEXT("HP {0} / Gold {1}", "HP {0} / Gold {1}");
		}
		return FText::FromString(Key);
	}
}

#undef LOCTEXT_NAMESPACE
