/*****************************************************************//**
 * @file   DataAssetImportUtils.cpp
 * @brief  노션 CSV로 DA를 만들 때 쓰는 공통 유틸 구현
 * @author 이문환
 * @date   2026-09-08
 *********************************************************************/

#include "Commandlet/DataAssetImportUtils.h"

#include "Misc/FileHelper.h"
#include "Misc/DataValidation.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "TAS/Effect/TacticalEffect.h"

DEFINE_LOG_CATEGORY(LogDataAssetImport)

FString FImportCsvTable::Get(int32 RowIndex, const FString& Column) const
{
	// 컬럼이 없거나 그 칸까지 셀이 없으면 빈 칸으로 취급
	const int32* ColumnIndex = mColumnIndex.Find(NormalizeColumn(Column));
	if (ColumnIndex == nullptr || mRows.IsValidIndex(RowIndex) == false || mRows[RowIndex].IsValidIndex(*ColumnIndex) == false)
	{
		return FString();
	}
	return mRows[RowIndex][*ColumnIndex];
}

FString FImportCsvTable::NormalizeColumn(const FString& Name)
{
	// 괄호 안은 삭제하고 공백도 전부 제거
	FString Result;
	int32 Depth = 0;
	for (const TCHAR Ch : Name)
	{
		if (Ch == TEXT('('))
		{
			++Depth;
		}
		else if (Ch == TEXT(')'))
		{
			Depth = FMath::Max(0, Depth - 1);
		}
		else if (Depth == 0 && FChar::IsWhitespace(Ch) == false)
		{
			Result.AppendChar(Ch);
		}
	}
	return Result;
}

bool DataAssetImportUtils::LoadCsv(const FString& FilePath, FImportCsvTable& OutTable)
{
	FString Content;
	if (FFileHelper::LoadFileToString(Content, *FilePath) == false)
	{
		UE_LOG(LogDataAssetImport, Error, TEXT("CSV 파일을 열 수 없음: %s"), *FilePath);
		return false;
	}

	// 구분자: .tsv면 탭, 아니면 쉼표
	const TCHAR Delimiter = FilePath.EndsWith(TEXT(".tsv")) ? TEXT('\t') : TEXT(',');

	// 문자 단위로 검사해서 따옴표를 기준으로 셀과 행으로 구분
	TArray<TArray<FString>> Records;
	TArray<FString> Cells;
	FString Cell;
	bool bInQuotes = false;
	for (int32 Index = 0; Index < Content.Len(); ++Index)
	{
		const TCHAR Ch = Content[Index];

		// 따옴표 안: 쉼표와 줄바꿈도 셀 값. 따옴표 두 개는 따옴표 문자 하나, 홀로 있으면 닫힘
		if (bInQuotes)
		{
			if (Ch != TEXT('"'))
			{
				Cell.AppendChar(Ch);
			}
			else if (Index + 1 < Content.Len() && Content[Index + 1] == TEXT('"'))
			{
				Cell.AppendChar(TEXT('"'));
				++Index;
			}
			else
			{
				bInQuotes = false;
			}
			continue;
		}

		// 따옴표 밖: 여는 따옴표, 구분자, 줄바꿈을 처리. 줄바꿈은 \n만 쓰고 \r은 건너뜀
		if (Ch == TEXT('"'))
		{
			bInQuotes = true;
		}
		else if (Ch == Delimiter)
		{
			Cells.Add(Cell);
			Cell.Reset();
		}
		else if (Ch == TEXT('\n'))
		{
			Cells.Add(Cell);
			Cell.Reset();
			Records.Add(MoveTemp(Cells));
			Cells.Reset();
		}
		else if (Ch != TEXT('\r'))
		{
			Cell.AppendChar(Ch);
		}
	}

	// 파일 끝에 줄바꿈이 없으면 남은 셀을 마지막 행으로 마감
	if (Cell.Len() > 0 || Cells.Num() > 0)
	{
		Cells.Add(Cell);
		Records.Add(MoveTemp(Cells));
	}

	// 첫 행이 헤더니까 이름을 확인해서 컬럼 번호와 매핑
	if (Records.Num() == 0)
	{
		UE_LOG(LogDataAssetImport, Error, TEXT("CSV에 헤더가 없음: %s"), *FilePath);
		return false;
	}
	OutTable.mColumnIndex.Reset();
	for (int32 Index = 0; Index < Records[0].Num(); ++Index)
	{
		OutTable.mColumnIndex.Add(FImportCsvTable::NormalizeColumn(Records[0][Index]), Index);
	}

	// 나머지 행은 데이터만 있음. 모든 셀이 빈 행은 버림
	OutTable.mRows.Reset();
	for (int32 Index = 1; Index < Records.Num(); ++Index)
	{
		const bool bHasValue = Records[Index].ContainsByPredicate([](const FString& Cell) { return Cell.TrimStartAndEnd().Len() > 0; });
		if (bHasValue)
		{
			OutTable.mRows.Add(MoveTemp(Records[Index]));
		}
	}
	return true;
}

TMap<FString, FString> DataAssetImportUtils::ParseLabels(const FString& Text)
{
	TMap<FString, FString> Labels;

	// 줄마다 "라벨 : 값"으로 해석
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);
	for (const FString& RawLine : Lines)
	{
		const FString Line = RawLine.TrimStartAndEnd();
		if (Line.IsEmpty())
		{
			continue;
		}

		// 콜론이 있으면 콜론, 없으면 첫 공백을 기준으로 라벨과 값을 나눔. 둘 다 없으면 숫자 하나로 보고 고정값
		FString Label;
		FString Value;
		if (Line.Split(TEXT(":"), &Label, &Value) == false && Line.Split(TEXT(" "), &Label, &Value) == false)
		{
			Label = TEXT("고정값");
			Value = Line;
		}
		Labels.Add(Label.TrimStartAndEnd(), Value.TrimStartAndEnd());
	}
	return Labels;
}

bool DataAssetImportUtils::IsChecked(const FString& Text)
{
	// 노션 Yes, 시트 TRUE/1, 손 입력 O/✓ 를 모두 켜진 것으로 봄. 대소문자 무시
	const FString Value = Text.TrimStartAndEnd().ToUpper();
	return Value == TEXT("YES") || Value == TEXT("TRUE") || Value == TEXT("1") || Value == TEXT("O") || Value == TEXT("✓");
}

bool DataAssetImportUtils::ParseTimingTag(const FString& Text, FGameplayTag& OutTag)
{
	// 비어 있으면 시점 없음
	const FString Name = Text.TrimStartAndEnd();
	if (Name.IsEmpty())
	{
		OutTag = FGameplayTag();
		return true;
	}

	// 노션엔 OnStartRoom처럼 짧게 적으니 공통 접두어를 붙인 뒤, 등록된 태그인지 확인
	// 이미 접두어가 붙어 있으면 그대로 사용
	const FString Prefix = TEXT("GameplayAbility.Passive.");
	const FString TagName = Name.StartsWith(Prefix) ? Name : Prefix + Name;
	OutTag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
	if (OutTag.IsValid() == false)
	{
		UE_LOG(LogDataAssetImport, Error, TEXT("알 수 없는 시점: %s"), *Name);
		return false;
	}
	return true;
}

bool DataAssetImportUtils::ParseOperand(const FString& KindText, const FString& LabelText, FPassiveOperand& OutOperand)
{
	OutOperand = FPassiveOperand();

	// 종류 이름 -> enum. 노션 표기가 "캡처"와 "캡쳐"로 섞여 있어 둘 다 허용
	static const TMap<FString, EPassiveOperandKind> KindByName = {
		{ TEXT("고정값"), EPassiveOperandKind::Const },
		{ TEXT("속성값"), EPassiveOperandKind::Attribute },
		{ TEXT("태그 개수"), EPassiveOperandKind::TagCount },
		{ TEXT("카운터"), EPassiveOperandKind::Counter },
		{ TEXT("거리"), EPassiveOperandKind::Distance },
		{ TEXT("타겟 수"), EPassiveOperandKind::TargetCount },
		{ TEXT("캡처값"), EPassiveOperandKind::Captured },
		{ TEXT("캡쳐값"), EPassiveOperandKind::Captured },
		{ TEXT("이동 거리"), EPassiveOperandKind::MovedDistance },
		{ TEXT("팀 관계"), EPassiveOperandKind::Team },
	};
	const EPassiveOperandKind* Kind = KindByName.Find(KindText.TrimStartAndEnd());
	if (Kind == nullptr)
	{
		UE_LOG(LogDataAssetImport, Error, TEXT("알 수 없는 피연산자 종류: %s"), *KindText);
		return false;
	}
	OutOperand.mKind = *Kind;

	// 공통 라벨: 출처(자신/대상), 배수
	const TMap<FString, FString> Labels = ParseLabels(LabelText);
	if (const FString* Source = Labels.Find(TEXT("출처")))
	{
		OutOperand.mSource = (*Source == TEXT("대상")) ? EPassiveOperandSource::Target : EPassiveOperandSource::Self;
	}
	if (const FString* Multiplier = Labels.Find(TEXT("배수")))
	{
		OutOperand.mMultiplier = FCString::Atof(**Multiplier);
	}

	// 종류별 필드: 고정값 / 속성 / 태그 / 캡처 키. 나머지 종류는 공통 라벨만으로 충분
	switch (OutOperand.mKind)
	{
	case EPassiveOperandKind::Const:
		{
			const FString* Value = Labels.Find(TEXT("고정값"));
			if (Value == nullptr)
			{
				UE_LOG(LogDataAssetImport, Error, TEXT("고정값이 없음: %s"), *LabelText);
				return false;
			}
			OutOperand.mConst = FCString::Atof(**Value);
		}
		break;

	case EPassiveOperandKind::Attribute:
		{
			const FString* Value = Labels.Find(TEXT("속성"));
			if (Value == nullptr || ParseAttribute(*Value, OutOperand.mAttribute) == false)
			{
				UE_LOG(LogDataAssetImport, Error, TEXT("속성이 없거나 잘못됨: %s"), *LabelText);
				return false;
			}
		}
		break;

	case EPassiveOperandKind::TagCount:
		{
			const FString* Value = Labels.Find(TEXT("태그"));
			OutOperand.mTag = (Value != nullptr) ? FGameplayTag::RequestGameplayTag(FName(**Value), false) : FGameplayTag();
			if (OutOperand.mTag.IsValid() == false)
			{
				UE_LOG(LogDataAssetImport, Error, TEXT("태그가 없거나 잘못됨: %s"), *LabelText);
				return false;
			}
		}
		break;

	case EPassiveOperandKind::Captured:
		{
			const FString* Value = Labels.Find(TEXT("캡처 키"));
			if (Value == nullptr)
			{
				Value = Labels.Find(TEXT("캡쳐 키"));
			}
			if (Value == nullptr)
			{
				UE_LOG(LogDataAssetImport, Error, TEXT("캡처 키가 없음: %s"), *LabelText);
				return false;
			}
			OutOperand.mCaptureKey = FName(**Value);
		}
		break;

	default:
		break;
	}
	return true;
}

bool DataAssetImportUtils::ParseCompareOp(const FString& Text, EPassiveCompareOp& OutOp)
{
	// 연산자 기호 -> enum
	static const TMap<FString, EPassiveCompareOp> OpByName = {
		{ TEXT("<"), EPassiveCompareOp::Less },
		{ TEXT("<="), EPassiveCompareOp::LessEqual },
		{ TEXT("=="), EPassiveCompareOp::Equal },
		{ TEXT("!="), EPassiveCompareOp::NotEqual },
		{ TEXT(">="), EPassiveCompareOp::GreaterEqual },
		{ TEXT(">"), EPassiveCompareOp::Greater },
		{ TEXT("배수"), EPassiveCompareOp::ModuloZero },
	};
	const EPassiveCompareOp* Op = OpByName.Find(Text.TrimStartAndEnd());
	if (Op == nullptr)
	{
		UE_LOG(LogDataAssetImport, Error, TEXT("알 수 없는 연산자: %s"), *Text);
		return false;
	}
	OutOp = *Op;
	return true;
}

UClass* DataAssetImportUtils::FindEffectClass(const FString& Text)
{
	// 노션의 TE_X를 C++ 클래스 이름 TacticalEffect_X로 바꿔 찾음 (리플렉션 이름엔 U 접두어 없음)
	FString Name = Text.TrimStartAndEnd();
	if (Name.StartsWith(TEXT("TE_")))
	{
		Name = TEXT("TacticalEffect_") + Name.RightChop(3);
	}
	UClass* Found = FindFirstObject<UClass>(*Name, EFindFirstObjectOptions::ExactClass);
	if (Found == nullptr || Found->IsChildOf(UTacticalEffect::StaticClass()) == false || Found->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogDataAssetImport, Error, TEXT("이펙트 클래스를 찾을 수 없음: %s"), *Text);
		return nullptr;
	}
	return Found;
}

bool DataAssetImportUtils::ParseAttribute(const FString& Text, FTacticalAttribute& OutAttribute)
{
	// 예) "CombatTargetAttributeSet.HP"면 클래스와 속성으로 나누고, "HP"만 있으면 클래스는 비움
	FString ClassName;
	FString PropertyName;
	if (Text.TrimStartAndEnd().Split(TEXT("."), &ClassName, &PropertyName) == false)
	{
		PropertyName = Text.TrimStartAndEnd();
	}

	// 등록된 속성 전체에서 이름이 맞는 것을 찾음. 클래스가 없으면 속성명만 맞는 게 하나여야 함
	// 노션에 UCombatTargetAttributeSet처럼 U를 붙여 적어도 되게, 리플렉션 이름과 U를 붙인 이름 둘 다 허용
	TArray<FProperty*> Properties;
	FTacticalAttribute::GetAllAttributeProperties(Properties);
	TArray<FProperty*> Matched;
	for (FProperty* Property : Properties)
	{
		const FString OwnerName = GetNameSafe(Property->GetOwnerStruct());
		const bool bClassMatched = ClassName.IsEmpty() || ClassName == OwnerName || ClassName == TEXT("U") + OwnerName;
		if (bClassMatched && Property->GetName() == PropertyName)
		{
			Matched.Add(Property);
		}
	}
	if (Matched.Num() != 1)
	{
		UE_LOG(LogDataAssetImport, Error, TEXT("속성을 찾을 수 없거나 여러 개 일치: %s (%d개)"), *Text, Matched.Num());
		return false;
	}
	OutAttribute.SetUProperty(Matched[0]);
	return true;
}

FString DataAssetImportUtils::ToPascalCase(const FString& Text)
{
	// 공백을 단어 경계로 보고 단어 첫 글자만 대문자로. 영문자와 숫자 외는 버림
	FString Result;
	bool bWordStart = true;
	for (const TCHAR Ch : Text)
	{
		if (FChar::IsWhitespace(Ch))
		{
			bWordStart = true;
		}
		else if (FChar::IsAlnum(Ch))
		{
			Result.AppendChar(bWordStart ? FChar::ToUpper(Ch) : Ch);
			bWordStart = false;
		}
	}
	return Result;
}

UObject* DataAssetImportUtils::FindOrCreateAsset(UClass* AssetClass, const FString& PackagePath, const FString& AssetName, bool& bOutCreated)
{
	bOutCreated = false;
	const FString PackageName = PackagePath / AssetName;

	// 같은 이름의 .uasset이 있으면 로드해서 그 안의 오브젝트를 돌려줌
	if (UPackage* Existing = LoadPackage(nullptr, *PackageName, LOAD_NoWarn | LOAD_Quiet))
	{
		UObject* Asset = FindObject<UObject>(Existing, *AssetName);
		if (Asset != nullptr && Asset->IsA(AssetClass))
		{
			return Asset;
		}
		UE_LOG(LogDataAssetImport, Error, TEXT("같은 이름의 에셋이 다른 타입임: %s"), *PackageName);
		return nullptr;
	}

	// 없으면 패키지와 오브젝트를 새로 만들고 에셋 레지스트리에 알림
	UPackage* Package = CreatePackage(*PackageName);
	UObject* Asset = NewObject<UObject>(Package, AssetClass, FName(*AssetName), RF_Public | RF_Standalone);
	FAssetRegistryModule::AssetCreated(Asset);
	bOutCreated = true;
	return Asset;
}

bool DataAssetImportUtils::SaveAsset(UObject* Asset)
{
	// 패키지 이름으로 .uasset 경로를 구해 저장
	// UE 이름 규칙에 따라 '/Game/'이 'Content/'로 바뀜
	UPackage* Package = Asset->GetOutermost();
	const FString FileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	Package->MarkPackageDirty();

	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	const bool bSaved = UPackage::SavePackage(Package, Asset, *FileName, Args);
	if (bSaved == false)
	{
		UE_LOG(LogDataAssetImport, Error, TEXT("저장 실패: %s"), *FileName);
	}
	return bSaved;
}

bool DataAssetImportUtils::ValidateAsset(const UObject* Asset)
{
	// IsDataValid가 모은 에러와 경고를 그대로 로그로 옮김
	FDataValidationContext Context;
	Asset->IsDataValid(Context);
	for (const FDataValidationContext::FIssue& Issue : Context.GetIssues())
	{
		if (Issue.Severity == EMessageSeverity::Error)
		{
			UE_LOG(LogDataAssetImport, Error, TEXT("[%s] %s"), *Asset->GetName(), *Issue.Message.ToString());
		}
		else
		{
			UE_LOG(LogDataAssetImport, Warning, TEXT("[%s] %s"), *Asset->GetName(), *Issue.Message.ToString());
		}
	}
	return Context.GetNumErrors() == 0;
}
