/*****************************************************************//**
 * @file   ImportArtifactDataCommandlet.cpp
 * @brief  노션 아티팩트 CSV로 아티팩트 DA를 만드는 커맨드릿 구현
 * @author 이문환
 * @date   2026-09-08
 *********************************************************************/

#include "Commandlet/ImportArtifactDataCommandlet.h"

#include "Commandlet/DataAssetImportUtils.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"

namespace
{
	// 노션 CSV 헤더 이름
	// 괄호와 공백은 제거되며 문서에서 컬럼 이름을 바꾸면 여기도 수정 필요
	namespace Column
	{
		const TCHAR* Name = TEXT("이름");
		const TCHAR* EnglishName = TEXT("영명");
		const TCHAR* Id = TEXT("ID");
		const TCHAR* Description = TEXT("설명");
		const TCHAR* PassiveId = TEXT("패시브 ID");
		const TCHAR* Attribute = TEXT("Attribute");
		const TCHAR* ModifierOp = TEXT("ModifierOp");
		const TCHAR* ModifierMagnitude = TEXT("ModifierMagnitude");
		const TCHAR* RarityType = TEXT("RarityType");
		const TCHAR* Price = TEXT("Price");
		const TCHAR* Exclude = TEXT("생성 제외");
	}

	// 출력 폴더, 에셋 이름 접두어, 참조할 패시브 DA 위치
	const TCHAR* PackagePath = TEXT("/Game/BP/DataAsset/Artifact");
	const TCHAR* AssetPrefix = TEXT("DA_Artifact_");
	const TCHAR* PassivePackagePath = TEXT("/Game/BP/DataAsset/Passive");
	const TCHAR* PassiveAssetPrefix = TEXT("DA_Passive_");
}

UImportArtifactDataCommandlet::UImportArtifactDataCommandlet()
{
	// 에디터 없이 돌아가지만 에셋 저장에 에디터 기능이 필요
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UImportArtifactDataCommandlet::Main(const FString& Params)
{
	// -csv 는 필수, -ids 는 선택
	FString CsvPath;
	if (FParse::Value(*Params, TEXT("csv="), CsvPath) == false)
	{
		UE_LOG(LogDataAssetImport, Error, TEXT("사용법: -run=ImportArtifactData -csv=<경로> [-ids=A001,A002]"));
		return 1;
	}
	FString IdsText;
	TArray<FString> OnlyIds;
	if (FParse::Value(*Params, TEXT("ids="), IdsText))
	{
		IdsText.ParseIntoArray(OnlyIds, TEXT(","));
	}

	// CSV 읽기
	FImportCsvTable Table;
	if (DataAssetImportUtils::LoadCsv(CsvPath, Table) == false)
	{
		return 1;
	}

	// 필수 컬럼이 없으면 이름을 찍고 중단. 희귀도/가격은 없으면 경고만, 생성 제외 체크박스는 없어도 됨
	const TCHAR* RequiredColumns[] = {
		Column::Name, Column::EnglishName, Column::Id, Column::Description, Column::PassiveId,
		Column::Attribute, Column::ModifierOp, Column::ModifierMagnitude,
	};
	bool bColumnsOk = true;
	for (const TCHAR* Name : RequiredColumns)
	{
		if (Table.HasColumn(Name) == false)
		{
			UE_LOG(LogDataAssetImport, Error, TEXT("CSV에 컬럼이 없음: %s"), Name);
			bColumnsOk = false;
		}
	}
	if (bColumnsOk == false)
	{
		return 1;
	}
	if (Table.HasColumn(Column::RarityType) == false || Table.HasColumn(Column::Price) == false)
	{
		UE_LOG(LogDataAssetImport, Warning, TEXT("RarityType/Price 컬럼이 없어 Common/0 으로 채움"));
	}

	// 행마다 DA 생성 또는 덮어쓰기
	int32 NumCreated = 0;
	int32 NumUpdated = 0;
	int32 NumSkipped = 0;
	int32 NumFailed = 0;
	TSet<FString> SeenIds;
	for (int32 Row = 0; Row < Table.mRows.Num(); ++Row)
	{
		const FString Id = Table.Get(Row, Column::Id).TrimStartAndEnd();
		if (Id.IsEmpty() || (OnlyIds.Num() > 0 && OnlyIds.Contains(Id) == false))
		{
			continue;
		}
		SeenIds.Add(Id);

		// 생성 제외 체크박스가 켜져 있으면 건너뜀
		if (DataAssetImportUtils::IsChecked(Table.Get(Row, Column::Exclude)))
		{
			UE_LOG(LogDataAssetImport, Display, TEXT("%s: 생성 제외"), *Id);
			++NumSkipped;
			continue;
		}

		// 에셋 이름: DA_Artifact_<ID>_<영명 파스칼 표기>. 영명이 비어 있으면 ID만 쓰고 경고
		// 예) DA_Artifact_A001_ChaliceOfLife
		const FString EnglishName = DataAssetImportUtils::ToPascalCase(Table.Get(Row, Column::EnglishName));
		if (EnglishName.IsEmpty())
		{
			UE_LOG(LogDataAssetImport, Warning, TEXT("%s: 영명이 비어 있어 ID만으로 이름을 만듦. 나중에 채우면 파일이 하나 더 생기니 옛 파일은 삭제 필요"), *Id);
		}
		const FString AssetName = EnglishName.IsEmpty() ? AssetPrefix + Id : FString::Printf(TEXT("%s%s_%s"), AssetPrefix, *Id, *EnglishName);

		// DA를 만들거나 로드해서 채우고, 검증한 뒤 저장
		bool bCreated = false;
		UStaticArtifactData* Data = DataAssetImportUtils::FindOrCreateAsset<UStaticArtifactData>(PackagePath, AssetName, bCreated);
		if (Data == nullptr || FillArtifactData(Table, Row, Data) == false)
		{
			UE_LOG(LogDataAssetImport, Error, TEXT("%s: 생성 실패"), *Id);
			++NumFailed;
			continue;
		}
		DataAssetImportUtils::ValidateAsset(Data);
		if (DataAssetImportUtils::SaveAsset(Data) == false)
		{
			++NumFailed;
			continue;
		}
		bCreated ? ++NumCreated : ++NumUpdated;
	}

	// -ids 에 있는데 CSV에 없는 ID 경고
	for (const FString& Id : OnlyIds)
	{
		if (SeenIds.Contains(Id) == false)
		{
			UE_LOG(LogDataAssetImport, Warning, TEXT("CSV에 없는 ID: %s"), *Id);
		}
	}

	UE_LOG(LogDataAssetImport, Display, TEXT("아티팩트 DA 임포트 끝: 생성 %d, 갱신 %d, 제외 %d, 실패 %d"), NumCreated, NumUpdated, NumSkipped, NumFailed);
	return (NumFailed == 0) ? 0 : 1;
}

bool UImportArtifactDataCommandlet::FillArtifactData(const FImportCsvTable& Table, int32 RowIndex, UStaticArtifactData* Data) const
{
	const FString Id = Table.Get(RowIndex, Column::Id).TrimStartAndEnd();
	bool bOk = true;

	// 이름과 설명
	Data->mName = FText::FromString(Table.Get(RowIndex, Column::Name).TrimStartAndEnd());
	Data->mDescription = FText::FromString(Table.Get(RowIndex, Column::Description).TrimStartAndEnd());

	// 희귀도: Common/Rare/Epic. 비었거나 틀리면 Common 으로 채우고 경고
	const FString RarityText = Table.Get(RowIndex, Column::RarityType).TrimStartAndEnd();
	const int64 RarityValue = StaticEnum<ERarityType>()->GetValueByNameString(RarityText);
	if (RarityValue == INDEX_NONE)
	{
		UE_LOG(LogDataAssetImport, Warning, TEXT("%s: 희귀도가 비었거나 잘못돼 Common 으로 채움: '%s'"), *Id, *RarityText);
	}
	Data->mRarityType = (RarityValue == INDEX_NONE) ? ERarityType::Common : static_cast<ERarityType>(RarityValue);

	// 가격: 숫자가 아니면 0 으로 채우고 경고
	const FString PriceText = Table.Get(RowIndex, Column::Price).TrimStartAndEnd();
	if (PriceText.IsNumeric() == false)
	{
		UE_LOG(LogDataAssetImport, Warning, TEXT("%s: 가격이 비었거나 잘못돼 0 으로 채움: '%s'"), *Id, *PriceText);
	}
	Data->mPrice = PriceText.IsNumeric() ? FCString::Atoi(*PriceText) : 0;

	// 패시브 참조: ID가 비어 있으면 스탯 전용 아티팩트. 있으면 DA_Passive_<ID> 를 소프트 참조
	// 패시브 DA 파일이 아직 없으면 에러
	Data->mStaticPassiveData.Reset();
	const FString PassiveId = Table.Get(RowIndex, Column::PassiveId).TrimStartAndEnd();
	if (PassiveId.IsEmpty() == false)
	{
		const FString PassiveAssetName = PassiveAssetPrefix + PassiveId;
		const FString PassivePackage = FString(PassivePackagePath) / PassiveAssetName;
		if (FPackageName::DoesPackageExist(PassivePackage) == false)
		{
			UE_LOG(LogDataAssetImport, Error, TEXT("%s: 패시브 DA가 없음: %s"), *Id, *PassivePackage);
			bOk = false;
		}
		Data->mStaticPassiveData.Add(TSoftObjectPtr<UStaticPassiveData>(FSoftObjectPath(PassivePackage + TEXT(".") + PassiveAssetName)));
	}

	// 스탯 수정자: Attribute 가 비어 있으면 없음. 있으면 속성/연산/크기 하나
	Data->mStatModifiers.Reset();
	const FString AttributeText = Table.Get(RowIndex, Column::Attribute).TrimStartAndEnd();
	if (AttributeText.IsEmpty() == false)
	{
		FTacticalModifierInfo Modifier;
		bOk &= DataAssetImportUtils::ParseAttribute(AttributeText, Modifier.mAttribute);

		const FString OpText = Table.Get(RowIndex, Column::ModifierOp).TrimStartAndEnd();
		const int64 OpValue = StaticEnum<ETacticalModOp::Type>()->GetValueByNameString(OpText);
		if (OpValue == INDEX_NONE)
		{
			UE_LOG(LogDataAssetImport, Error, TEXT("%s: 알 수 없는 연산: '%s'"), *Id, *OpText);
			bOk = false;
		}
		Modifier.mModifierOp = (OpValue == INDEX_NONE) ? ETacticalModOp::AddBase : static_cast<ETacticalModOp::Type>(OpValue);
		Modifier.mModifierMagnitude = FCString::Atof(*Table.Get(RowIndex, Column::ModifierMagnitude));
		Data->mStatModifiers.Add(Modifier);
	}

	return bOk;
}
