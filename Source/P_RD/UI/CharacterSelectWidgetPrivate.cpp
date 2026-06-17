#include "UI/CharacterSelectWidgetPrivate.h"

namespace RDCharacterSelect
{
	/** @brief 선택 화면 C++ 기본 문구를 키 기반으로 반환하고 알 수 없는 키는 디버깅 가능한 원문으로 돌려준다. */
	FText Text(const TCHAR* Key)
	{
		if (FCString::Strcmp(Key, TEXT("ConfirmText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "ConfirmText", "확인");
		}
		if (FCString::Strcmp(Key, TEXT("BackText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "BackText", "뒤로가기");
		}
		if (FCString::Strcmp(Key, TEXT("ReadyStatusText")) == 0)
		{
			return FText::GetEmpty();
		}
		if (FCString::Strcmp(Key, TEXT("LoadingStatusText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "LoadingStatusText", "Loading");
		}
		if (FCString::Strcmp(Key, TEXT("FailedStatusText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "FailedStatusText", "Failed");
		}
		if (FCString::Strcmp(Key, TEXT("NoCharacterStatusText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "NoCharacterStatusText", "No character data");
		}
		if (FCString::Strcmp(Key, TEXT("CharacterSelectText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "CharacterSelectText", "Character Select");
		}
		if (FCString::Strcmp(Key, TEXT("SelectedCharacterFormat")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "SelectedCharacterFormat", "{0} selected");
		}
		if (FCString::Strcmp(Key, TEXT("CharacterLockedStatus")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "CharacterLockedStatus", "This character is not available");
		}
		if (FCString::Strcmp(Key, TEXT("PortraitFallbackText")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "PortraitFallbackText", "No portrait");
		}
		if (FCString::Strcmp(Key, TEXT("CharacterStatFormat")) == 0)
		{
			return NSLOCTEXT("TitleMenuWidget", "CharacterStatFormat", "HP {0} / Dice {1} / Gold {2}");
		}
		return FText::FromString(Key);
	}
}
