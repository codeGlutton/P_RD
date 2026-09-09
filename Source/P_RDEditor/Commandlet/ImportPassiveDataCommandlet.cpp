/*****************************************************************//**
 * @file   ImportPassiveDataCommandlet.cpp
 * @brief  노션 패시브 CSV로 패시브 DA를 만드는 커맨드릿 구현
 * @author 이문환
 * @date   2026-09-08
 *********************************************************************/

#include "Commandlet/ImportPassiveDataCommandlet.h"

#include "Commandlet/DataAssetImportUtils.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"

namespace ImportPassiveDataPrivate
{
	// 노션 CSV 헤더 이름
	// 괄호와 공백은 제거되며 문서에서 컬럼 이름을 바꾸면 여기도 수정 필요
	namespace Column
	{
		const TCHAR* Title = TEXT("제목");
		const TCHAR* Id = TEXT("ID");
		const TCHAR* Exclude = TEXT("생성 제외");
		const TCHAR* ActivateTiming = TEXT("발동 시점");
		const TCHAR* DeactivateTiming = TEXT("해제 시점");
		const TCHAR* CounterResetTiming = TEXT("카운터 초기화 시점");
		const TCHAR* CaptureTiming = TEXT("캡쳐 시점");
		const TCHAR* EffectTarget = TEXT("선택");
		const TCHAR* Quantifier = TEXT("타겟 논리 연산");
		const TCHAR* EffectClass = TEXT("이펙트 클래스");
		const TCHAR* MagnitudeKind = TEXT("종류");
		const TCHAR* MagnitudeText = TEXT("수치");
		const TCHAR* ConditionOp = TEXT("조건 - 연산자");
		const TCHAR* ConditionLhsKind = TEXT("조건 - 좌변 - 종류");
		const TCHAR* ConditionLhsText = TEXT("조건 - 좌변");
		const TCHAR* ConditionRhsKind = TEXT("조건 - 우변 - 종류");
		const TCHAR* ConditionRhsText = TEXT("조건 - 우변");
		const TCHAR* CaptureKind = TEXT("캡쳐 피연산자 - 종류");
		const TCHAR* CaptureKey = TEXT("캡쳐 피연산자 - Key");
		const TCHAR* CaptureText = TEXT("텍스트");
	}

	// 출력 폴더와 에셋 이름 접두어
	const TCHAR* PackagePath = TEXT("/Game/BP/DataAsset/Passive");
	const TCHAR* AssetPrefix = TEXT("DA_Passive_");
}

UImportPassiveDataCommandlet::UImportPassiveDataCommandlet()
{
	// 에디터 없이 돌아가지만 에셋 저장에 에디터 기능이 필요
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UImportPassiveDataCommandlet::Main(const FString& Params)
{
	using namespace ImportPassiveDataPrivate;
	// -csv 는 필수, -ids 는 선택
	FString CsvPath;
	if (FParse::Value(*Params, TEXT("csv="), CsvPath) == false)
	{
		UE_LOG(LogDataAssetImport, Error, TEXT("사용법: -run=ImportPassiveData -csv=<경로> [-ids=P001,P015]"));
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

	// 필수 컬럼이 하나라도 없으면 이름을 찍고 중단. 생성 제외 체크박스는 없어도 됨
	const TCHAR* RequiredColumns[] = {
		Column::Title, Column::Id,
		Column::ActivateTiming, Column::DeactivateTiming, Column::CounterResetTiming, Column::CaptureTiming,
		Column::EffectTarget, Column::Quantifier, Column::EffectClass, Column::MagnitudeKind, Column::MagnitudeText,
		Column::ConditionOp, Column::ConditionLhsKind, Column::ConditionLhsText, Column::ConditionRhsKind, Column::ConditionRhsText,
		Column::CaptureKind, Column::CaptureKey, Column::CaptureText,
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

		// DA를 만들거나 로드해서 채우고, 검증한 뒤 저장
		bool bCreated = false;
		UStaticPassiveData* Data = DataAssetImportUtils::FindOrCreateAsset<UStaticPassiveData>(PackagePath, AssetPrefix + Id, bCreated);
		if (Data == nullptr || FillPassiveData(Table, Row, Data) == false)
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

	UE_LOG(LogDataAssetImport, Display, TEXT("패시브 DA 임포트 끝: 생성 %d, 갱신 %d, 제외 %d, 실패 %d"), NumCreated, NumUpdated, NumSkipped, NumFailed);
	return (NumFailed == 0) ? 0 : 1;
}

bool UImportPassiveDataCommandlet::FillPassiveData(const FImportCsvTable& Table, int32 RowIndex, UStaticPassiveData* Data) const
{
	using namespace ImportPassiveDataPrivate;
	const FString Id = Table.Get(RowIndex, Column::Id).TrimStartAndEnd();

	// 설명: 제목에서 앞의 ID를 뗌
	// 예) "P001 전투 시작 시, ..." -> "전투 시작 시, ..."
	FString Title = Table.Get(RowIndex, Column::Title).TrimStartAndEnd();
	if (Title.StartsWith(Id))
	{
		Title = Title.RightChop(Id.Len()).TrimStart();
	}
	Data->mDescription = FText::FromString(Title);

	// 시점 4종. 비어 있으면 빈 태그
	bool bOk = true;
	bOk &= DataAssetImportUtils::ParseTimingTag(Table.Get(RowIndex, Column::ActivateTiming), Data->mActivateTimingTag);
	bOk &= DataAssetImportUtils::ParseTimingTag(Table.Get(RowIndex, Column::DeactivateTiming), Data->mDeactivateTimingTag);
	bOk &= DataAssetImportUtils::ParseTimingTag(Table.Get(RowIndex, Column::CounterResetTiming), Data->mCounterResetTimingTag);
	bOk &= DataAssetImportUtils::ParseTimingTag(Table.Get(RowIndex, Column::CaptureTiming), Data->mCaptureTimingTag);

	// 효과 대상(자신/대상)과 수량 조건(Any/All)
	Data->mEffectTarget = (Table.Get(RowIndex, Column::EffectTarget).TrimStartAndEnd() == TEXT("대상")) ? EPassiveEffectTarget::Targets : EPassiveEffectTarget::Self;
	Data->mTargetQuantifier = (Table.Get(RowIndex, Column::Quantifier).TrimStartAndEnd() == TEXT("All")) ? EPassiveTargetQuantifier::All : EPassiveTargetQuantifier::Any;

	// 효과: 이펙트 클래스는 여러 개 추가 가능. 단, 수치는 하나만 있으므로 동일한 수치가 들어감
	Data->mEffects.Reset();
	FPassiveOperand Magnitude;
	bOk &= DataAssetImportUtils::ParseOperand(Table.Get(RowIndex, Column::MagnitudeKind), Table.Get(RowIndex, Column::MagnitudeText), Magnitude);
	TArray<FString> EffectNames;
	Table.Get(RowIndex, Column::EffectClass).ParseIntoArray(EffectNames, TEXT(","));
	for (const FString& EffectName : EffectNames)
	{
		UClass* EffectClass = DataAssetImportUtils::FindEffectClass(EffectName);
		if (EffectClass == nullptr)
		{
			bOk = false;
			continue;
		}
		FPassiveEffectEntry Entry;
		Entry.mEffectClass = EffectClass;
		Entry.mMagnitude = Magnitude;
		Data->mEffects.Add(Entry);
	}

	// 조건: 연산자 칸이 비어 있으면 조건 없음. 있으면 좌변/연산자/우변 하나
	Data->mConditions.Reset();
	const FString OpText = Table.Get(RowIndex, Column::ConditionOp).TrimStartAndEnd();
	if (OpText.IsEmpty() == false)
	{
		FPassiveCondition Condition;
		bOk &= DataAssetImportUtils::ParseOperand(Table.Get(RowIndex, Column::ConditionLhsKind), Table.Get(RowIndex, Column::ConditionLhsText), Condition.mLhs);
		bOk &= DataAssetImportUtils::ParseCompareOp(OpText, Condition.mOp);
		bOk &= DataAssetImportUtils::ParseOperand(Table.Get(RowIndex, Column::ConditionRhsKind), Table.Get(RowIndex, Column::ConditionRhsText), Condition.mRhs);
		Data->mConditions.Add(Condition);
	}

	// 캡처: 종류 칸이 비어 있으면 캡처 없음. 있으면 키 + 피연산자 하나
	Data->mCaptureOperands.Reset();
	const FString CaptureKind = Table.Get(RowIndex, Column::CaptureKind).TrimStartAndEnd();
	if (CaptureKind.IsEmpty() == false)
	{
		FPassiveCaptureEntry Capture;
		Capture.mKey = FName(*Table.Get(RowIndex, Column::CaptureKey).TrimStartAndEnd());
		bOk &= DataAssetImportUtils::ParseOperand(CaptureKind, Table.Get(RowIndex, Column::CaptureText), Capture.mOperand);
		Data->mCaptureOperands.Add(Capture);
	}

	// 캡처한 타겟에게 발동: 문서에 이 옵션 컬럼이 없으므로 시점을 조합해서 판단
	// 발동이 OnEndUsingSkill 이고 캡처가 OnStartApplyingEffect 이면 대상을 캡처했다가 스킬 끝에 판정하므로 켬
	const FGameplayTag OnEndUsingSkill = FGameplayTag::RequestGameplayTag(FName(TEXT("GameplayAbility.Passive.OnEndUsingSkill")));
	const FGameplayTag OnStartApplyingEffect = FGameplayTag::RequestGameplayTag(FName(TEXT("GameplayAbility.Passive.OnStartApplyingEffect")));
	Data->mActivateOnCapturedTargets = (Data->mActivateTimingTag == OnEndUsingSkill && Data->mCaptureTimingTag == OnStartApplyingEffect);

	return bOk;
}
