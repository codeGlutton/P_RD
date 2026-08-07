/**
 * @file TitleMenuWidgetPrivate.h
 * @brief 타이틀 메뉴 분할 cpp들이 공유하는 내부 헬퍼/상수.
 *
 * TitleMenuWidget.cpp와 TitleMenuWidget_Flow.cpp가 같은 프로필 목록/이름 규칙을 쓴다.
 * 익명 네임스페이스에 각자 정의하면 유니티 빌드에서 같은 TU로 합쳐질 때 재정의 충돌이 나므로,
 * 명명 네임스페이스 inline 정의로 위젯 이름을 한 곳에 둔다.
 */

#pragma once

#include "RDMinimal.h"

namespace RDTitleMenu
{
	/**
	 * @brief 타이틀 레이아웃 프로필. **한 벌뿐이다.**
	 *
	 * @details
	 * 전에는 화면비마다 한 벌씩 다섯이었다. 재 보니 넷은 1920x1080 기준에서
	 * 30~44px 밀린 것이 전부였고(위젯 크기는 하나도 안 달랐다), fold_inner 만
	 * 단추 묶음을 **0.898배**로 줄인 것이었다 -- 레이아웃이 아니라 배율이다.
	 *
	 * 그 차이를 캔버스 통째 복사로 표현하느라 위젯이 14 x 5 = 70개였고,
	 * 타이틀에 뭘 고치려면 다섯 군데를 고쳐야 했다. 자리 차이는 앵커가,
	 * 배율 차이는 ScaleBox 가 이미 하는 일이다.
	 *
	 * 이름은 남긴다 -- 위젯 접미사(``StartButton__base_16_9``)와 1:1 이고,
	 * 접미사까지 떼면 판을 다시 저작해야 한다.
	 */
	inline const FName TitleLayoutProfileBase16x9(TEXT("base_16_9"));

	inline const FName TitleLayoutProfiles[] =
	{
		TitleLayoutProfileBase16x9,
	};

	/** @brief 프로필별 위젯 이름 규칙: "{Base}__{프로필}" (WBP 저작 규칙과 1:1). */
	inline FName MakeProfileWidgetName(const TCHAR* BaseName, const FName ProfileName)
	{
		return FName(*FString::Printf(TEXT("%s__%s"), BaseName, *ProfileName.ToString()));
	}
}
