/**
 * @file TitleMenuWidgetPrivate.h
 * @brief 타이틀 메뉴 분할 cpp들이 공유하는 내부 헬퍼/상수.
 *
 * TitleMenuWidget.cpp와 TitleMenuWidget_Flow.cpp가 같은 프로필 목록/이름 규칙을 쓴다.
 * 익명 네임스페이스에 각자 정의하면 유니티 빌드에서 같은 TU로 합쳐질 때 재정의 충돌이 나므로,
 * CombatTileMapHUDWidgetPrivate.h와 같은 방식의 명명 네임스페이스 inline 정의로 한 곳에 둔다.
 */

#pragma once

#include "RDMinimal.h"

namespace RDTitleMenu
{
	/** @brief 화면비별 타이틀 레이아웃 프로필 이름(WidgetSwitcher 자식/프로필 위젯 접미사와 1:1). */
	inline const FName TitleLayoutProfileBase16x9(TEXT("base_16_9"));
	inline const FName TitleLayoutProfilePhoneWide(TEXT("phone_wide"));
	inline const FName TitleLayoutProfilePhoneUltraWide(TEXT("phone_ultrawide"));
	inline const FName TitleLayoutProfileFoldInner(TEXT("fold_inner"));
	inline const FName TitleLayoutProfileTablet16x10(TEXT("tablet_16_10"));

	inline const FName TitleLayoutProfiles[] =
	{
		TitleLayoutProfileBase16x9,
		TitleLayoutProfilePhoneWide,
		TitleLayoutProfilePhoneUltraWide,
		TitleLayoutProfileFoldInner,
		TitleLayoutProfileTablet16x10,
	};

	/** @brief 프로필별 위젯 이름 규칙: "{Base}__{프로필}" (WBP 저작 규칙과 1:1). */
	inline FName MakeProfileWidgetName(const TCHAR* BaseName, const FName ProfileName)
	{
		return FName(*FString::Printf(TEXT("%s__%s"), BaseName, *ProfileName.ToString()));
	}
}
