/*****************************************************************//**
 * @file   CharacterSelectTypes.h
 * @brief  캐릭터 선택 화면에서 쓰는 View 타입 정의
 * @author Codex
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "CharacterSelectTypes.generated.h"

class UTexture2D;

/**
 * @brief 캐릭터 선택 UI가 표시할 플레이어 유닛 카드 데이터
 *
 * @details
 * 이 타입은 feature/create-srpg-framework-base 브랜치에서 이미 마련된 캐릭터 선택 View 타입의
 * 책임을 이어받는다. Widget은 PlayerUnit DataAsset, 해금 규칙, 시작 스탯 계산 방식을 직접 알지 않고
 * FrontendGameMode가 변환해서 내려준 이 값만 화면에 그린다.
 *
 * PM 브랜치의 기본 타입은 이름/역할/설명/스탯 요약/아이콘/선택 가능 여부를 담는 표시 전용 구조다.
 * 현재 타이틀 -> 캐릭터 선택 -> 지도 흐름에서는 확정 시 실제 런 생성 API에 넘길
 * PlayerUnit PrimaryAssetId와 안정적인 선택 index가 추가로 필요하다. 그래서 PM 브랜치의
 * 공개 필드명은 유지하고, 런 생성과 현재 상세 UI에 필요한 값만 뒤에 보강한다.
 *
 * @note API 출처
 * feature/create-srpg-framework-base 브랜치에 이미 존재하던 FFrontendCharacterOption을 가져와
 * UI/map 흐름에 필요한 mIndex, mMaxHP, mDice, mGold, mPlayerUnitId 등을 보강한 타입이다.
 * 즉 새로 만든 완전 신규 API가 아니라 PM 브랜치 타입을 UI 파트가 확장한 것이다.
 *
 * 중요한 경계는 이렇다.
 * - UI는 이 배열의 개수만큼 WBP_CharacterCard를 만들고 값만 표시한다.
 * - UI는 직업 enum, DataAsset 로드 상태, 시작 HP/골드 계산 방식을 판단하지 않는다.
 * - Confirm 시에도 UI는 mPlayerUnitId만 FrontendGameMode에 다시 넘긴다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FFrontendCharacterOption
{
	GENERATED_BODY()

	// 화면 카드의 안정적인 선택 키. 배열 위치가 아니라 GameMode가 정렬 후 부여한 View index다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mIndex = INDEX_NONE;

	// 카드와 상세 패널에 표시할 이름. 비어 있으면 GameMode가 PrimaryAsset 이름으로 fallback한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mDisplayName;

	// 직업/역할 표시 문자열. 실제 직업 enum은 UI가 직접 해석하지 않는다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mRoleText;

	// 카드/상세 패널용 설명 문구.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mDescription;

	// PM 브랜치 타입과 호환되는 표시용 스탯 요약. 상세 UI는 당분간 아래 숫자 필드로 다시 포맷한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mStatSummary;

	// 시작 최대 체력. UI는 표시만 하고 계산하지 않는다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mMaxHP = 0;

	// 시작 주사위 수. UI는 표시만 하고 계산하지 않는다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mDice = 0;

	// 시작 골드. UI는 표시만 하고 계산하지 않는다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mGold = 0;

	// 상세 패널에 크게 보여줄 초상화. SoftObject라 필요할 때 비동기로 로드한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> mPortrait;

	// 카드에 작게 보여줄 아이콘. 없으면 Widget에서 초상화를 fallback으로 사용한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> mIcon;

	// 런 생성 때 GameMode/Subsystem으로 넘길 실제 캐릭터 식별자.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FPrimaryAssetId mPlayerUnitId;

	// 선택 가능 여부. 해금/잠금 판단은 GameMode가 끝내고, UI는 이 값만 신뢰한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool bSelectable = false;

	// 선택 불가일 때 상세 영역/상태 문구에서 사용할 사유.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mDisabledReason;
};
