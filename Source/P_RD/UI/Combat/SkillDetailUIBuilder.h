// @file SkillDetailUIBuilder.h
#pragma once

#include "CoreMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"   // EAimPattern/EEffectPattern
#include "UI/Combat/CombatUITypes.h"           // ECombatSkill*ShapeUI

class UStaticArtifactData;
class UStaticSkillData;
struct FCombatArtifactUI;
struct FSkillDetailUI;
struct FFrontendSkillOption;

/**
 * @brief 스킬 상세 DTO(FSkillDetailUI) 공용 조립기.
 *
 * @details
 * 전투/상점/지도/캐릭터 선택이 같은 상세창을 쓰려면 같은 조립 규칙이 필요하다.
 * 화면별로 모양 enum 변환이 세 벌로 갈라져 표기가 어긋났던 것을
 * (Square가 어디서는 원형, 어디서는 사각) 전투 기준 하나로 묶는다.
 *
 * 게임플레이 모델/DA를 인자로 받는 Fill* 계열은 GameMode 전용이다 — 위젯이
 * 직접 부르면 모델 결합이 되살아난다 (PR #426 규칙). 반면 DTO→DTO 변환
 * (FillFromFrontendOption, ToSelectShape/ToHitShape)은 호출자 자유다.
 */
namespace SkillDetailUIBuilder
{
	/** @brief 조준 패턴 → UI 시전 형태. 화면 공용 단일 변환표. */
	P_RD_API ECombatSkillSelectShapeUI ToSelectShape(EAimPattern Pattern);

	/** @brief 영향 패턴 → UI 타격 형태. Star는 8방향(Star)으로 그대로 보존한다. */
	P_RD_API ECombatSkillHitShapeUI ToHitShape(EEffectPattern Pattern);

	/**
	 * @brief 정적 스킬 데이터에서 표시 수치와 타게팅을 채운다.
	 * @details mSkillIndex는 호출자 몫이다. 쿨다운은 정적 값을 쓰므로
	 * 컴포넌트가 별도 값을 가지면 호출자가 덮어쓴다.
	 */
	P_RD_API void FillFromSkillData(const UStaticSkillData* SkillData,
		FSkillDetailUI& OutDetail);

	/** @brief 캐릭터 선택/고용 옵션 값에서 채운다. 변환 규칙은 FillFromSkillData와 같다. */
	P_RD_API void FillFromFrontendOption(const FFrontendSkillOption& Option,
		FSkillDetailUI& OutDetail);

	/**
	 * @brief 정적 아티팩트 데이터에서 상세 DTO(FCombatArtifactUI)를 채운다.
	 *
	 * @details
	 * 이름·아이콘·희귀도는 DA 에서, 효과 줄은 붙은 패시브의 mDescription 을
	 * 모아 만든다 -- 전투 HUD/상점이 각자 하던 조립을 하나로 묶은 것이다.
	 * 설명 없는 순수 스탯 아티팩트는 스탯 강화 기본 문구 한 줄로 폴백한다.
	 */
	P_RD_API void FillFromArtifactData(const UStaticArtifactData* ArtifactData,
		FCombatArtifactUI& OutDetail);
}
