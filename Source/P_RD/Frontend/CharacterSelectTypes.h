// @file CharacterSelectTypes.h
// @brief 캐릭터 선택 화면에서 쓰는 View 타입 정의
// @date 2026-06-04

#pragma once

#include "RDMinimal.h"
#include "DataAsset/UnitSpawnData/PlayerJobType.h"

#include "CharacterSelectTypes.generated.h"

class UTexture2D;

/** @brief 캐릭터 선택 UI가 표시할 플레이어 유닛 카드 데이터 */
// 이 타입은 캐릭터 선택 화면에 필요한 표시 데이터를 한 곳에 모은다.
// Widget은 PlayerUnit DataAsset, 해금 규칙, 시작 스탯 계산 방식을 직접 알지 않고
// FrontendGameMode가 변환해서 내려준 이 값만 화면에 그린다.
// 현재 타이틀 -> 캐릭터 선택 -> 첫 방 입장 흐름에서는 확정 시 실제 런 생성 API에 넘길
// PlayerUnit PrimaryAssetId와 안정적인 선택 index가 필요하다. 그래서 이름/역할/설명 같은
// 표시 값과 런 생성에 필요한 식별자를 같은 View 값으로 내려준다.
// 중요한 경계는 이렇다.
// - UI는 WBP에 배치된 WBP_CharacterCard 순서대로 이 배열의 값을 표시한다.
// - UI는 직업 enum, DataAsset 로드 상태, 시작 HP/골드 계산 방식을 판단하지 않는다.
// - Confirm 시에도 UI는 mPlayerUnitId만 FrontendGameMode에 다시 넘긴다.
USTRUCT(BlueprintType)
struct P_RD_API FFrontendCharacterOption
{
	GENERATED_BODY()

	// 화면 카드의 안정적인 선택 키. 배열 위치가 아니라 GameMode가 정렬 후 부여한 View index다.
	// WBP 카드 순서가 바뀌어도 클릭 이벤트는 이 값을 되돌려 보내야 한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	int32 mIndex = INDEX_NONE;

	// 카드와 상세 패널에 표시할 이름. 비어 있으면 GameMode가 PrimaryAsset 이름으로 fallback한다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mDisplayName;

	// 직업/역할 표시 문자열. 실제 직업 enum은 UI가 직접 해석하지 않는다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mRoleText;

	// 화면 아트 선택용 직업 키. 게임 로직 enum 이름과 표시 이름이 달라도 UI는 이 값으로 이미지를 고른다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	EPlayerJobType mJobType = EPlayerJobType::None;

	// 카드/상세 패널용 설명 문구.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mDescription;

	// 카드에 한 줄로 보여줄 표시용 스탯 요약. 상세 UI는 아래 숫자 필드로 다시 포맷한다.
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
	// 잠긴 카드는 선택 상세를 보여줄 수 있지만 Confirm에서는 PlayerUnitId를 비워 시작을 막는다.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	bool mSelectable = false;

	// 선택 불가일 때 상세 영역/상태 문구에서 사용할 사유.
	UPROPERTY(Category = Frontend, BlueprintReadOnly)
	FText mDisabledReason;
};
