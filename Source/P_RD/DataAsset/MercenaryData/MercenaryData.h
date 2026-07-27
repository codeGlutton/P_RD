/*****************************************************************//**
 * @file   MercenaryData.h
 * @brief  고용 게시판에 걸리는 용병 한 명의 정적 데이터.
 * @details
 * 프로토타입은 이름·체력·비용을 C++ 배열에 박아 두었다. 그러면 기획에서
 * 수치 하나 바꿀 때마다 코드를 고치고 빌드해야 한다. 데이터 에셋으로 빼면
 * 에디터에서 고치고 저장하면 끝이다.
 *
 * 여기 있는 것은 "게시판에 무엇을 보여줄까"에 필요한 것뿐이다. 전투에서
 * 쓰는 실제 유닛·스킬 데이터는 따로 있고, mUnitSpawnId 로 이어 붙인다 --
 * 게시판이 전투 데이터를 통째로 알 필요는 없다.
 * @author 박용수
 * @date   2026-07-27
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Engine/DataAsset.h"
#include "MercenaryData.generated.h"

class UTexture2D;

/**
 * @brief 용병이 전장에서 맡는 자리. 게시판에서 한눈에 갈리게 하는 표시다.
 */
UENUM(BlueprintType)
enum class EMercenaryRole : uint8
{
	Melee = 0		UMETA(DisplayName = "근접"),
	Ranged			UMETA(DisplayName = "원거리"),
	Magic			UMETA(DisplayName = "마법"),
	Support			UMETA(DisplayName = "지원"),
	Count			UMETA(Hidden)
};

/**
 * @brief 게시판 이력서 한 장에 필요한 것.
 */
UCLASS(BlueprintType)
class P_RD_API UMercenaryData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** @brief 이력서에 적히는 이름. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "용병")
	FText mName;

	/** @brief 근접·원거리·마법·지원. 배지로 보여준다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "용병")
	EMercenaryRole mRole = EMercenaryRole::Melee;

	/** @brief 최대 체력. 고를 때 보는 값이라 게시판에도 적는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "용병",
		meta = (ClampMin = "1"))
	int32 mMaxHP = 100;

	/**
	 * @brief 이력서에 적을 스킬 이름 두 줄.
	 *
	 * @details
	 * 실제 스킬 데이터가 아니라 보여줄 이름이다. 게시판에서 스킬의 수치나
	 * 사거리까지 보여주지 않기로 했으므로, 전투 스킬 에셋을 통째로 물고
	 * 있을 이유가 없다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "용병")
	TArray<FText> mSkillNames;

	/**
	 * @brief 왜 이 사람을 데려가는가. 한 줄.
	 *
	 * @details
	 * 여섯 중 셋을 고르는 것이 핵심이므로 판단할 거리가 있어야 한다.
	 * 수치만으로는 "체력 100 과 체력 75 중 무엇"이 되어 고를 수가 없다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "용병")
	FText mTrait;

	/** @brief 이력서에 붙는 얼굴. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "용병")
	TSoftObjectPtr<UTexture2D> mPortrait;

	/**
	 * @brief 고용했을 때 실제로 데려갈 유닛.
	 *
	 * @details
	 * 게시판은 이 값을 읽지 않는다. 출발할 때 파티에 넘겨줄 뿐이다 --
	 * 화면이 전투 데이터를 해석하기 시작하면 둘이 얽힌다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "용병")
	FPrimaryAssetId mUnitSpawnId;
};

/**
 * @brief 한 판에 게시판에 걸릴 용병들.
 *
 * @details
 * 값을 치르고 부르는 개념은 여기 없다. 처음 시작할 때는 그냥 셋을 고른다.
 * 돈으로 사는 것은 나중에 상점에서 들어온다.
 */
UCLASS(BlueprintType)
class P_RD_API UMercenaryBoardData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** @brief 게시판에 걸 이력서. 화면이 여섯 칸이라 여섯 장을 본다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "게시판")
	TArray<TSoftObjectPtr<UMercenaryData>> mMercenaries;

	/** @brief 데려갈 수 있는 인원. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "게시판",
		meta = (ClampMin = "1"))
	int32 mPartySize = 3;
};
