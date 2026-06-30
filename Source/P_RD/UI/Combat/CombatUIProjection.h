#pragma once

/** @brief 게임플레이 모델 -> 전투 UI 표시 DTO 변환(무상태 함수 모음). */
// 책임: USRPGCombatModel/DicePool/Skill/Equipment 등 게임플레이 모델을 읽어 CombatUITypes의 표시 DTO로 옮긴다.
// CombatGameMode의 Push*UI()는 이 함수들을 호출해 UCombatUIModel에 Set*하고 OnRefresh*UI를 Broadcast만 한다.
// (UObject/델리게이트를 만들지 않는다 — GameMode 경계는 그대로 두고 "변환 본문"만 떼어낸 것.)

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
