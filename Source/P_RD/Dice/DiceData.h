/*****************************************************************//**
 * @file   DiceData.h
 * @brief  전투 중 주사위 한 개의 "런타임 상태"를 담는 복제 가능한 데이터
 * @date   2026-06-16
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/RarityType.h"
#include "UObject/PrimaryAssetId.h"

#include "DiceData.generated.h"

class UStaticDiceData;

/**
 * @brief 주사위 한 개의 가변 런타임 상태(비주얼과 분리된 데이터 계층).
 *
 * @details
 * PM이 정한 "비주얼(Actor/Component) ↔ 복제 가능한 데이터" 분리에서 데이터 쪽이다.
 * 비주얼은 ADiceComponent/CombatDicePreviewActor가 맡고, 이 객체는 굴림 값/상태만 들고 있는다.
 *
 * 예측은 Clone()으로 사본을 떠 사본 위에서 Roll()/계산을 돌려 결과만 큐로 보내고 버린다.
 * 라이브 상태는 그대로 두므로 데미지 미리보기/AI 선읽기가 부작용 없이 가능하다.
 * GAS에 의존하지 않는 평범한 UObject라 DuplicateObject로 깊은 복제가 된다(GAS 폐기 방향과 일치).
 *
 * @note 현재 주사위 모델은 단순하다(희귀도 + 1..면수 결과값). 면별 효과가 생기면 여기에 면 데이터를 더한다.
 */
UCLASS(BlueprintType)
class P_RD_API UDiceData : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 희귀도/식별자로 초기 상태를 채운다(아직 안 굴린 상태). */
	void Init(ERarityType Rarity, const FPrimaryAssetId& SourceDiceId, int32 FaceCount = 6);

	/** @brief 불변 템플릿(UStaticDiceData)에서 희귀도를 읽어 초기화한다. */
	void InitFromStatic(const UStaticDiceData* StaticData, const FPrimaryAssetId& SourceDiceId);

	/** @brief 주어진 난수 스트림으로 굴려 현재 값을 정하고 그 값을 반환한다(1..면수). */
	int32 Roll(const FRandomStream& Stream);

	/** @brief 예측용 깊은 복제본을 만든다(사본에서 굴려 라이브를 건드리지 않기 위함). */
	UDiceData* Clone(UObject* InOuter = nullptr) const;

	ERarityType GetRarity() const { return mRarityType; }
	int32 GetFaceCount() const { return mFaceCount; }
	int32 GetCurrentValue() const { return mCurrentValue; }
	bool IsRolled() const { return mIsRolled; }
	const FPrimaryAssetId& GetSourceDiceId() const { return mSourceDiceId; }

	/** @brief 이번 턴에 이미 스킬에 쓰였는지(다음 굴림까지 재사용 불가). */
	bool IsUsed() const { return mIsUsed; }
	void SetUsed(bool bUsed) { mIsUsed = bUsed; }

private:
	/** @brief 희귀도. 아트/색/표시 분류에 쓰인다(게임 로직은 결과값으로). */
	UPROPERTY()
	ERarityType mRarityType = ERarityType::Common;

	/** @brief 면 수. 현재 전부 6면체. */
	UPROPERTY()
	int32 mFaceCount = 6;

	/** @brief 마지막 굴림 결과(1..면수). 0이면 아직 안 굴림. */
	UPROPERTY()
	int32 mCurrentValue = 0;

	/** @brief 한 번이라도 굴렸는지. */
	UPROPERTY()
	bool mIsRolled = false;

	/** @brief 이번 턴에 스킬에 쓰였는지(다음 굴림 시 false로 리셋). */
	UPROPERTY()
	bool mIsUsed = false;

	/** @brief 이 런타임 주사위가 어느 정적 주사위(런 보유 id)에서 나왔는지. */
	UPROPERTY()
	FPrimaryAssetId mSourceDiceId;
};
