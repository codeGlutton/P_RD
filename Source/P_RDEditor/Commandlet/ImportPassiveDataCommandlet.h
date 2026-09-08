/*****************************************************************//**
 * @file   ImportPassiveDataCommandlet.h
 * @brief  노션 패시브 CSV로 패시브 DA를 만드는 커맨드릿
 * @author 이문환
 * @date   2026-09-08
 *********************************************************************/

#pragma once

#include "RDEditorMinimal.h"
#include "Commandlets/Commandlet.h"
#include "ImportPassiveDataCommandlet.generated.h"

class UStaticPassiveData;
struct FImportCsvTable;

/**
 * @brief 패시브 DA 임포트 커맨드릿
 *
 * @details
 * 실행: UnrealEditor-Cmd.exe P_RD.uproject -run=ImportPassiveData -csv=<경로> [-ids=P001,P015]
 * CSV의 행마다 /Game/BP/DataAsset/Passive/DA_Passive_<ID> 를 만들거나 덮어씀.
 * -ids 를 주면 그 ID의 행만 처리. 비고에 "생성 제외"가 있는 행은 건너뜀.
 * 생성 후 IsDataValid 결과를 로그로 출력.
 */
UCLASS()
class UImportPassiveDataCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UImportPassiveDataCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	/**
	 * @brief CSV 행 하나를 DA에 채움
	 * @return 필수 항목 해석에 실패하면 false
	 */
	bool FillPassiveData(IN const FImportCsvTable& Table, IN int32 RowIndex, IN OUT UStaticPassiveData* Data) const;
};
