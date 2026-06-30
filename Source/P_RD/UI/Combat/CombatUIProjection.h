#pragma once

/*
 * 게임플레이 모델 -> 전투 UI 표시 DTO 변환(무상태 함수 모음).
 *
 * #216에서는 CombatGameMode가 너무 비대해져서, "모델을 읽어 DTO로 바꾸는 본문"만 이 파일로 뺐다.
 * #217에서 이 파일을 따로 보여주는 이유는, 변환 정책이 UI 시각화와 게임플레이 데이터 해석 사이에 걸쳐 있기 때문이다.
 *
 * 이 파일이 하지 않는 것:
 * - UObject를 만들지 않는다.
 * - 대리자를 Broadcast하지 않는다.
 * - 커맨드를 제출하지 않는다.
 * - UI 위젯을 직접 만지지 않는다.
 *
 * 이 파일이 하는 것:
 * - CombatModel, DicePool, SkillComponent, EquipmentComponent 등 권위 모델을 읽는다.
 * - 위젯이 바로 그릴 수 있는 CombatUITypes DTO로 옮긴다.
 * - 게임플레이 enum/데이터를 UI enum/표현값으로 낮춘다.
 */

#include "RDMinimal.h"
#include "UI/Combat/CombatUITypes.h"                // 표시 DTO + UI enum (+ SRPGFrameworkType: EAimPattern/EEffectPattern/ESRPG*BuildPhase)
#include "DataAsset/EquipmentData/EquipmentType.h"  // EEquipmentType
#include "DataAsset/RarityType.h"                   // ERarityType

class USRPGCombatModel;
class UDicePoolModel;
class USkillComponentModel;
class UEquipmentComponentModel;
class UPlayerUnitModel;

namespace CombatUIProjection
{
	/** @brief 게임플레이 스킬 빌드 페이즈 -> UI 빌드 페이즈. CombatGameMode 입력 람다에서 사용. */
	ECombatBuildPhaseUI ToCombatBuildPhaseUI(ESRPGSkillBuildPhase Phase);
	/** @brief 게임플레이 이동 빌드 페이즈 -> UI 빌드 페이즈. */
	ECombatBuildPhaseUI ToCombatBuildPhaseUI(ESRPGMoveBuildPhase Phase);

	/** @brief 전체 유닛 표시 DTO(HP/스탯/타일/위치/상태태그). */
	TArray<FUnitUI> BuildUnitUIs(USRPGCombatModel* CombatModel);
	/** @brief 보유 주사위 표시 DTO. */
	TArray<FDiceSlotUI> BuildDiceUIs(UDicePoolModel* DicePoolModel);
	/** @brief 스킬 빌드에 선택된 주사위 index 목록. (합계는 호출부에서 DicePoolModel->GetSelectedDiceSum()) */
	TArray<int32> BuildSelectedDiceIndices(UDicePoolModel* DicePoolModel);
	/** @brief 현재 턴 표시 DTO. */
	FTurnUI BuildTurnUI(USRPGCombatModel* CombatModel);
	/** @brief 스킬 레일 표시 DTO. */
	TArray<FSkillUI> BuildSkillUIs(USkillComponentModel* SkillComponentModel);
	/** @brief 장비 슬롯 표시 DTO. */
	TArray<FEquipmentUI> BuildEquipmentUIs(UEquipmentComponentModel* EquipmentComponentModel);
	/** @brief 플레이어 메타(Gold/Lv/Exp) 표시 DTO. */
	FPlayerMetaUI BuildPlayerMetaUI(UPlayerUnitModel* PlayerUnit);
}
