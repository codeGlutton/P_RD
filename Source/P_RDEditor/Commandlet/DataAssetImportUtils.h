/*****************************************************************//**
 * @file   DataAssetImportUtils.h
 * @brief  노션 CSV로 DA를 만들 때 쓰는 공통 유틸 (CSV 파서, 텍스트 해석, DA 생성/저장/검증)
 * @author 이문환
 * @date   2026-09-08
 *********************************************************************/

#pragma once

#include "RDEditorMinimal.h"
#include "GameplayTagContainer.h"
#include "TAS/AttributeSet/TacticalAttributeSet.h"
#include "TAS/Passive/PassiveCondition.h"

class UTacticalEffect;

/**
 * @brief CSV 표 하나
 *
 * @details
 * 노션 CSV는 컬럼이 가나다순, 행은 뒤섞여 나옴.
 * 컬럼은 헤더 이름으로 찾고, 행은 커맨드릿이 ID 컬럼을 보고 고름.
 */
struct FImportCsvTable
{
	// 헤더 이름 -> 컬럼 인덱스
	TMap<FString, int32> mColumnIndex;

	// 행마다 셀 배열 (헤더 순서)
	TArray<TArray<FString>> mRows;

	// 헤더 이름으로 셀 조회. 컬럼이 없거나 행이 짧으면 빈 문자열
	FString Get(int32 RowIndex, const FString& Column) const;

	// 컬럼 존재 여부
	bool HasColumn(const FString& Column) const { return mColumnIndex.Contains(NormalizeColumn(Column)); }

	// 헤더 이름 비교용 정규화: 괄호 안과 공백을 뺌
	// 예) "발동 시점(Activate Timing)" -> "발동시점"
	static FString NormalizeColumn(const FString& Name);
};

/**
 * @brief DA 임포트 공통 함수
 */
namespace DataAssetImportUtils
{
	/**
	 * @brief CSV 파일 읽기. 따옴표 안의 쉼표와 줄바꿈을 셀 값으로 처리. 확장자가 .tsv면 탭 구분
	 * @return 헤더를 읽었으면 true
	 */
	bool LoadCsv(IN const FString& FilePath, OUT FImportCsvTable& OutTable);

	/**
	 * @brief 라벨 텍스트 해석. "출처 : 자신 / 속성 : X / 배수 : 1.0" 같은 여러 줄을 라벨별 값으로 나눔
	 *
	 * @details
	 * 줄마다 "라벨 : 값". 콜론이 빠진 "배수 1.0"은 첫 공백 기준으로 나눔.
	 * 라벨 없이 숫자만 있으면 "고정값" 라벨로 넣음.
	 */
	TMap<FString, FString> ParseLabels(IN const FString& Text);

	/**
	 * @brief 체크박스가 체크되어 있는 지 여부
	 * @details 문서 종류가 다양하므로 유연한 판정 필요
	 */
	bool IsChecked(IN const FString& Text);

	/**
	 * @brief 시점 이름 -> 패시브 타이밍 태그 ("OnStartRoom" -> GameplayAbility.Passive.OnStartRoom). 빈 문자열이면 빈 태그
	 */
	bool ParseTimingTag(IN const FString& Text, OUT FGameplayTag& OutTag);

	/**
	 * @brief 종류 이름 + 라벨 텍스트 -> 피연산자 ("속성값" + "출처 : 자신 / 속성 : … / 배수 : 1.0")
	 */
	bool ParseOperand(IN const FString& KindText, IN const FString& LabelText, OUT FPassiveOperand& OutOperand);

	/**
	 * @brief 연산자 기호 -> 비교 연산자 ("<=", "배수" 등)
	 */
	bool ParseCompareOp(IN const FString& Text, OUT EPassiveCompareOp& OutOp);

	/**
	 * @brief 이펙트 이름 -> 클래스 ("TE_Defense" -> UTacticalEffect_Defense)
	 */
	UClass* FindEffectClass(IN const FString& Text);

	/**
	 * @brief 속성 이름 -> 속성 ("CombatTargetAttributeSet.HP" 또는 "HP"). 클래스 없이 이름만 있으면 전체 속성셋에서 하나만 맞을 때 성공
	 */
	bool ParseAttribute(IN const FString& Text, OUT FTacticalAttribute& OutAttribute);

	/**
	 * @brief 영문 이름을 에셋 이름에 쓸 파스칼 표기로 변환. 단어 첫 글자 대문자, 영문자와 숫자 외는 버림
	 * 예) "Guardian's Shield" -> "GuardiansShield"
	 */
	FString ToPascalCase(IN const FString& Text);

	/**
	 * @brief DA를 새로 만들거나, 같은 이름의 .uasset이 있으면 참조가 깨지지 않게 기존 파일 로드
	 * @param bOutCreated 새로 만들었으면 true, 기존 것을 로드했으면 false
	 * @return 실패하면 nullptr
	 */
	UObject* FindOrCreateAsset(IN UClass* AssetClass, IN const FString& PackagePath, IN const FString& AssetName, OUT bool& bOutCreated);

	// 타입 지정 버전
	template<typename T>
	T* FindOrCreateAsset(IN const FString& PackagePath, IN const FString& AssetName, OUT bool& bOutCreated)
	{
		return Cast<T>(FindOrCreateAsset(T::StaticClass(), PackagePath, AssetName, bOutCreated));
	}

	/**
	 * @brief DA를 디스크에 저장
	 */
	bool SaveAsset(IN UObject* Asset);

	/**
	 * @brief IsDataValid를 돌려 에러와 경고를 로그로 출력
	 * @return 에러가 없으면 true
	 */
	bool ValidateAsset(IN const UObject* Asset);
}

DECLARE_LOG_CATEGORY_EXTERN(LogDataAssetImport, Log, All)
