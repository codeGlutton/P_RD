/*****************************************************************//**
 * @file   CharacterSelectTypes.h
 * @brief  캐릭터 선택 화면에서 쓰는 데이터 타입 정의 헤더
 * @author Codex
 * @date   2026-06-03
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "CharacterSelectTypes.generated.h"

class UTexture2D;

/**
 * @brief 캐릭터 선택 UI가 캐릭터 하나를 표시할 때 사용하는 값
 *
 * @details
 * 이 구조체는 "UI에 보여줄 캐릭터 한 명"이라고 보면 된다.
 * CharacterSelectWidget과 CharacterCardWidget은 이 값만 보고 이름, 역할, 설명, 선택 가능 여부를 그린다.
 *
 * UI는 이 값이 임시 하드코딩인지, 데이터 테이블에서 온 값인지, 플레이어 유닛 데이터 에셋에서 온 값인지 알 필요가 없다.
 * 이번 UI-only 브랜치에서는 CharacterSelectWidget 안에서 임시 목록을 직접 만들고,
 * 실제 캐릭터 목록 제공자는 담당 파트에서 준비된 뒤 별도 브랜치로 연결한다.
 *
 * 현재는 feature/create-srpg-framework-base 통합 전이라
 * CharacterSelectWidget이 이 구조체를 코드에서 직접 채운다.
 * 그 브랜치가 통합되면 UStaticPlayerUnitSpawnData 같은 실제 데이터 에셋 값을
 * 이 구조체로 옮겨 담는 방식으로 바꾸면 된다.
 *
 * @note
 * 캐릭터가 3명에서 5명으로 늘어나도 UI 코드는 버튼 함수를 새로 만들지 않는다.
 * GetCharacterOptions()가 이 구조체 배열을 5개 채워주면 UI는 카드도 5개 만든다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FFrontendCharacterOption
{
	GENERATED_BODY()

public:
	/** @brief 카드와 상세 영역에 보여줄 캐릭터 이름 */
	UPROPERTY(Category = "Frontend", BlueprintReadOnly)
	FText mDisplayName;

	/** @brief FRONT, RANGE, SPELL 같은 역할 또는 직업 표시 문구 */
	UPROPERTY(Category = "Frontend", BlueprintReadOnly)
	FText mRoleText;

	/** @brief 상세 영역과 카드에 보여줄 짧은 캐릭터 설명 */
	UPROPERTY(Category = "Frontend", BlueprintReadOnly)
	FText mDescription;

	/**
	 * @brief HP, 주사위, 돈 같은 핵심 스탯을 한 줄로 요약한 문구
	 *
	 * @todo feature/create-srpg-framework-base 통합 후 실제 플레이어 유닛 스탯 값으로 교체한다.
	 */
	UPROPERTY(Category = "Frontend", BlueprintReadOnly)
	FText mStatSummary;

	/**
	 * @brief 하단 캐릭터 아이콘 버튼에 넣을 작은 이미지
	 *
	 * @details
	 * 값이 비어 있으면 CharacterCardWidget은 단색 placeholder 아이콘을 보여준다.
	 * 나중에 실제 캐릭터 아이콘 텍스처가 준비되면 이 값만 채우면 된다.
	 */
	UPROPERTY(Category = "Frontend", BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> mIcon;

	/** @brief 상세 영역에 보여줄 큰 이미지. 값이 있으면 비동기로 로드한다. */
	UPROPERTY(Category = "Frontend", BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> mPortrait;

	/**
	 * @brief Confirm으로 게임을 시작할 수 있으면 true
	 *
	 * @note
	 * false여도 카드는 클릭 가능하다.
	 * 잠긴 카드를 눌렀을 때 "왜 못 고르는지"를 보여주기 위해서다.
	 */
	UPROPERTY(Category = "Frontend", BlueprintReadOnly)
	bool bSelectable = false;

	/** @brief bSelectable이 false일 때 상세 영역에 보여줄 이유 */
	UPROPERTY(Category = "Frontend", BlueprintReadOnly)
	FText mDisabledReason;
};
