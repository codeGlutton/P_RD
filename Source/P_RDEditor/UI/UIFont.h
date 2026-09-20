// 판을 만드는 도구들이 함께 쓰는 글꼴 규칙.
#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

namespace UIFont
{
	/**
	 * @brief 프로젝트가 쓰는 글꼴.
	 *
	 * 전투 HUD 는 이걸 쓰는데(RoundText · MercenaryDetailName …) 새로 만든
	 * TextBlock 은 엔진 기본 Roboto 로 남는다. 한 게임에서 글꼴이 둘이면
	 * 같은 값도 다른 화면처럼 읽힌다. 몬스터탭이 그랬다.
	 */
	constexpr TCHAR ProjectFont[] =
		TEXT("/Game/SVN/OutSideAsset/Fonts/F_HUD_Oswald.F_HUD_Oswald");

	/** @brief 설정판처럼 한글 글꼴 하나로 통일하는 화면이 쓰는 글꼴. */
	constexpr TCHAR SettingsFont[] =
		TEXT("/Game/SVN/OutSideAsset/Fonts/F_HUD_LINESeedKR.F_HUD_LINESeedKR");

	inline int32 Size(const int32 Base)
	{
		return Base;
	}

	/**
	 * @brief 지정한 글꼴과 픽셀 크기를 배율 없이 그대로 넣는다.
	 *
	 * WBP에 저장할 최종 크기가 이미 정해진 빌더는 이 함수를 쓴다. Make처럼
	 * 모바일 배율을 다시 곱하지 않으므로 같은 빌더를 여러 번 실행해도 글자가
	 * 계속 커지지 않는다.
	 */
	inline FSlateFontInfo MakeExact(const FSlateFontInfo& Template,
		const TCHAR* FontPath, const int32 ExactSize)
	{
		FSlateFontInfo Font = Template;
		if (UObject* Loaded = LoadObject<UObject>(nullptr, FontPath))
		{
			Font.FontObject = Loaded;
			Font.TypefaceFontName = FName(TEXT("Bold"));
		}
		Font.Size = ExactSize;
		return Font;
	}

	/** @brief 프로젝트 복합 글꼴을 배율 없이 정확한 크기로 만든다. */
	inline FSlateFontInfo MakeProjectExact(const FSlateFontInfo& Template,
		const int32 ExactSize)
	{
		return MakeExact(Template, ProjectFont, ExactSize);
	}

	/** @brief 설정판 LINESeedKR 글꼴을 배율 없이 정확한 크기로 만든다. */
	inline FSlateFontInfo MakeSettingsExact(const FSlateFontInfo& Template,
		const int32 ExactSize)
	{
		return MakeExact(Template, SettingsFont, ExactSize);
	}

	/** @brief 글꼴 구조체에 프로젝트 글꼴과 지정한 크기를 그대로 넣는다. */
	inline FSlateFontInfo Make(const FSlateFontInfo& Template, const int32 Base)
	{
		return MakeProjectExact(Template, Size(Base));
	}
}
