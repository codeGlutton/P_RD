/*****************************************************************//**
 * @file   StageBuilder.h
 * @brief  스테이지 내 방들을 생성해주는 빌더 객체 구현 헤더
 * @author 모호재
 * @date   2026-05-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "PCGStage/Stage.h"
#include "Setting/GameBalanceType.h"
#include "DataAsset/EquipmentData/EquipmentType.h"
#include "DataAsset/SkillData/SkillType.h"
#include "DataAsset/UnitSpawnData/UnitJobType.h"
#include "DataTable/StageBuilderParams.h"

// Stage Builder 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogStageBuilder, Log, All)

struct FRoomEdge
{
	TArray<int32> mPreRoomColumns;
};

/**
 * @brief  스테이지 내 방들을 생성해주는 빌더 객체
 */
struct FStageBuilder
{
private:
	FStageBuilder(const FRandomStream& BuildStream, const FGlobalStageBuildSetting& GlobalSetting);

public:
	static FStageBuilder Make(const FRandomStream& BuildStream, const FGlobalStageBuildSetting& GlobalSetting);
	static FStageBuilder Make(const FRandomStream& BuildStream, const FGlobalStageBuildSetting& GlobalSetting, const FStageBuilderParams& Params);
	FStageBuilder& SetParams(const FStageBuilderParams& Params);
	FStage Build() const;
	void Build(OUT FStage& NewStage) const;

protected:
	/**
	 * 방 랜덤 요소를 정하기 위해 미리 에셋 모든 ID 배열을 캐싱해두는 함수
	 */
	void LoadAllAssetIds();

protected:
	void InitStage(OUT FStage& Stage) const;
	
	TArray<FRoomEdge> MakeStartRoutes(OUT FStage& Stage) const;
	TArray<FRoomEdge> MakeNextRoutes(OUT FStage& Stage, int32 CurRowIndex, const TArray<FRoomEdge>& CurEdges) const;

	void CreateStartRoom(OUT FStage& Stage) const;
	void CreateRooms(OUT FStage& Stage, int32 CurRowIndex, const TArray<FRoomEdge>& CurEdges) const;

protected:
	FRoom& CreateRoom(ERoomType Type, int32 Row, int32 Column, TInstancedStruct<FRoom>& Room) const;

protected:
	TArray<FPrimaryAssetId> GetFilteredPrimaryAssets(const FPrimaryAssetType& AssetType, TFunctionRef<bool(const FAssetData&)> Filter) const;
	TArray<FPrimaryAssetId> GetFilteredPrimaryAssets(const TArray<FAssetData>& AssetDataList, TFunctionRef<bool(const FAssetData&)> Filter) const;
	TArray<FAssetData> GetFilteredPrimaryAssetDatas(const FPrimaryAssetType& AssetType, TFunctionRef<bool(const FAssetData&)> Filter) const;

	uint8 GetRandomRarityIndex(const FRarityRate& RarityRate) const;
	ERarityType GetRandomRarity(const FRarityRate& RarityRate) const;

protected:
	const FRandomStream& mBuildStream;
	const FGlobalStageBuildSetting& mGlobalSetting;
	FStageBuilderParams mParams;

protected:
	bool mIsLoadedIds = false;
	TArray<FPrimaryAssetId> mRoomAssetIds[static_cast<uint8>(ERoomType::Count)];
	TArray<FPrimaryAssetId> mEquipmentAssetIds[static_cast<uint8>(ERarityType::Count)];
	TArray<FPrimaryAssetId> mJobSkillAssetIds[static_cast<uint8>(EUnitJobType::PlayerJobCount)][static_cast<uint8>(ERarityType::Count)];
	TArray<FPrimaryAssetId> mCommonSkillAssetIds[static_cast<uint8>(ERarityType::Count)];
};
