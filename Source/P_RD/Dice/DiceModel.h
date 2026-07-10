// @file DiceModel.h
// @brief 전투 중 주사위 한 개의 런타임 상태/로직을 담는 컴포넌트형 데이터 모델
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "DataAsset/RarityType.h"
#include "UObject/PrimaryAssetId.h"

#include "DiceModel.generated.h"

class UStaticDiceData;
class UTexture;

/** @brief 주사위 한 개의 가변 런타임 상태/로직을 담는 컴포넌트형 데이터 모델입니다. */
// 보드 액터(플레이어)가 소유하는 컴포넌트형 Model이라 UComponentModel(=IObjectModel)을 상속한다.
// 굴림 값/면/사용 상태/로직만 들고, 실제 표시·3D 캡처는 별도 View 계층이 맡는다(데이터/비주얼 분리).
// 예측은 Clone()으로 사본을 떠 사본 위에서 Roll()/계산을 돌린 뒤 결과만 큐로 보낼 수 있게 한다.
UCLASS(BlueprintType)
class P_RD_API UDiceModel : public UComponentModel
{
	GENERATED_BODY()

public:
	/** @brief 희귀도/식별자로 초기 상태를 채운다(아직 안 굴린 상태). */
	void Init(ERarityType Rarity, const FPrimaryAssetId& SourceDiceId, int32 FaceCount = 6);

	/** @brief 면별 값/텍스처까지 포함해 초기 상태를 채운다. */
	void InitWithFaces(ERarityType Rarity, const FPrimaryAssetId& SourceDiceId, int32 FaceCount, const TArray<int32>& FaceValues, const TArray<TObjectPtr<UTexture>>& FaceTextures);

	/** @brief 불변 템플릿(UStaticDiceData)에서 면/희귀도를 읽어 초기화한다. */
	void InitFromStatic(const UStaticDiceData* StaticData, const FPrimaryAssetId& SourceDiceId);

	/** @brief 주어진 난수 스트림으로 면 index를 뽑고, 그 면에 적힌 값을 현재 값으로 반환한다. */
	int32 Roll(const FRandomStream& Stream);

	/** @brief 외부(물리 굴림 연출)가 확정한 면 index를 굴림 결과로 기록한다("보이는 면 = 기록 숫자"). */
	int32 ApplyRolledFace(int32 FaceIndex);

	/** @brief 예측용 깊은 복제본을 만든다(사본에서 굴려 라이브를 건드리지 않기 위함). */
	UDiceModel* Clone(UObject* InOuter = nullptr) const;

	ERarityType GetRarity() const { return mRarityType; }
	int32 GetFaceCount() const { return mFaceCount; }
	int32 GetCurrentValue() const { return mCurrentValue; }
	int32 GetRolledFaceIndex() const { return mRolledFaceIndex; }
	bool IsRolled() const { return mIsRolled; }
	const FPrimaryAssetId& GetSourceDiceId() const { return mSourceDiceId; }
	const TArray<int32>& GetFaceValues() const { return mFaceValues; }
	const TArray<TObjectPtr<UTexture>>& GetFaceTextures() const { return mFaceTextures; }

	/** @brief 이번 턴에 이미 스킬에 쓰였는지 여부입니다. */
	bool IsUsed() const { return mIsUsed; }
	void SetUsed(bool bUsed) { mIsUsed = bUsed; }

	/** @brief 스킬 빌드에 올린(선택된) 주사위인지 여부입니다. */
	bool IsSelected() const { return mIsSelected; }
	void SetSelected(bool bSelected) { mIsSelected = bSelected; }

private:
	/** @brief 희귀도. 아트/색/표시 분류에 쓰인다(게임 로직은 결과값으로). */
	UPROPERTY()
	ERarityType mRarityType = ERarityType::Common;

	/** @brief 면 수입니다. d2/d4/d6/d8/d12/d20 등 정적 데이터가 지정한 값을 사용합니다. */
	UPROPERTY()
	int32 mFaceCount = 6;

	/** @brief 각 물리 면에 적힌 실제 값. index가 물리 면 번호(0-base)다. */
	UPROPERTY()
	TArray<int32> mFaceValues;

	/** @brief 각 물리 면 위에 덮을 선택 텍스처. nullptr이면 기본 면 텍스처를 쓴다. */
	UPROPERTY()
	TArray<TObjectPtr<UTexture>> mFaceTextures;

	/** @brief 마지막에 굴러진 물리 면 index. INDEX_NONE이면 아직 안 굴림. */
	UPROPERTY()
	int32 mRolledFaceIndex = INDEX_NONE;

	/** @brief 마지막 굴림 결과(굴러진 면의 mFaceValues 값). 0이면 아직 안 굴림. */
	UPROPERTY()
	int32 mCurrentValue = 0;

	/** @brief 한 번이라도 굴렸는지. */
	UPROPERTY()
	bool mIsRolled = false;

	/** @brief 이번 턴에 스킬에 쓰였는지 여부입니다. 턴 리셋 또는 새 턴 굴림에서 false가 됩니다. */
	UPROPERTY()
	bool mIsUsed = false;

	/** @brief 스킬 빌드에 올린(선택된) 주사위인지 여부입니다. 스킬 확정/취소 시 해제됩니다. */
	UPROPERTY()
	bool mIsSelected = false;

	/** @brief 이 런타임 주사위가 어느 정적 주사위(런 보유 id)에서 나왔는지. */
	UPROPERTY()
	FPrimaryAssetId mSourceDiceId;
};
