/*****************************************************************//**
 * @file   ImportArtifactDataCommandlet.h
 * @brief  노션 아티팩트 CSV로 아티팩트 DA를 만드는 커맨드릿
 * @author 이문환
 * @date   2026-09-08
 *********************************************************************/

#pragma once

#include "RDEditorMinimal.h"
#include "Commandlets/Commandlet.h"
#include "ImportArtifactDataCommandlet.generated.h"

class UStaticArtifactData;
struct FImportCsvTable;

/**
 * @brief 아티팩트 DA 임포트 커맨드릿
 *
 * @details
 * 실행: UnrealEditor-Cmd.exe P_RD.uproject -run=ImportArtifactData -csv=<경로> [-ids=A001,A002]
 * CSV의 행마다 /Game/BP/DataAsset/Artifact/DA_Artifact_<ID>_<영명> 을 만들거나 덮어씀.
 * 예) DA_Artifact_A001_ChaliceOfLife
 * 패시브 ID는 /Game/BP/DataAsset/Passive/DA_Passive_<ID> 참조로 연결.
 * 아이콘은 표에 없으므로 건드리지 않음.
 */
UCLASS()
class UImportArtifactDataCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UImportArtifactDataCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	/**
	 * @brief CSV 행 하나를 DA에 채움
	 * @return 필수 항목 해석에 실패하면 false
	 */
	bool FillArtifactData(IN const FImportCsvTable& Table, IN int32 RowIndex, IN OUT UStaticArtifactData* Data) const;
};
