/*****************************************************************//**
 * @file   PassiveConditionTests.cpp
 * @brief  패시브 조건 평가기 테스트
 * @details
 * 월드 없이 스냅샷과 런타임 상태만 구성해서 피연산자 계산과 조건 판정을 검증.
 *  - Compare: 연산자 7종
 *  - Resolve: Attribute(배수) / TagCount(하위 태그 합) / Counter / Distance / Captured / MovedDistance / 스냅샷 없음
 *  - EvaluateAll: 빈 배열 / AND
 * @author 이문환
 * @date   2026-08-30
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "TAS/Passive/PassiveCondition.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData_Generic.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "AttributeSet/CombatTargetAttributeSet.h"
#include "GameplayTagType.h"

namespace
{
	// HP / MaxHP / 타일 위치를 가진 스냅샷 생성
	UBoardCombatTargetSnapshotData* MakeSnapshot(float HP, float MaxHP, const FTileIndex& Tile)
	{
		UBoardCombatTargetSnapshotData* Snapshot = NewObject<UBoardCombatTargetSnapshotData>();
		Snapshot->mAttributes.Add(UCombatTargetAttributeSet::GetHPAttribute(), HP);
		Snapshot->mAttributes.Add(UCombatTargetAttributeSet::GetMaxHPAttribute(), MaxHP);
		Snapshot->mTileTransform.mIndex = Tile;
		return Snapshot;
	}

	// 피연산자 생성 헬퍼
	FPassiveOperand ConstOp(float Value)
	{
		FPassiveOperand Op;
		Op.mKind = EPassiveOperandKind::Const;
		Op.mConst = Value;
		return Op;
	}

	FPassiveOperand AttrOp(EPassiveOperandSource Source, const FTacticalAttribute& Attribute, float Multiplier = 1.f)
	{
		FPassiveOperand Op;
		Op.mKind = EPassiveOperandKind::Attribute;
		Op.mSource = Source;
		Op.mAttribute = Attribute;
		Op.mMultiplier = Multiplier;
		return Op;
	}

	FPassiveOperand KindOp(EPassiveOperandKind Kind, EPassiveOperandSource Source = EPassiveOperandSource::Self)
	{
		FPassiveOperand Op;
		Op.mKind = Kind;
		Op.mSource = Source;
		return Op;
	}
}

// 연산자 7종이 기대대로 비교하는지 검증
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveConditionCompareTests,
	"P_RD.TAS.Passive.Condition.Compare",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveConditionCompareTests::RunTest(const FString& Parameters)
{
	// 대소 비교
	TestTrue(TEXT("3 < 5"), FPassiveCondition::Compare(3.f, EPassiveCompareOp::Less, 5.f));
	TestFalse(TEXT("5 < 5"), FPassiveCondition::Compare(5.f, EPassiveCompareOp::Less, 5.f));
	TestTrue(TEXT("5 <= 5"), FPassiveCondition::Compare(5.f, EPassiveCompareOp::LessEqual, 5.f));
	TestTrue(TEXT("5 >= 5"), FPassiveCondition::Compare(5.f, EPassiveCompareOp::GreaterEqual, 5.f));
	TestTrue(TEXT("6 > 5"), FPassiveCondition::Compare(6.f, EPassiveCompareOp::Greater, 5.f));
	TestFalse(TEXT("5 > 5"), FPassiveCondition::Compare(5.f, EPassiveCompareOp::Greater, 5.f));

	// 근사 비교: float 계산 오차를 같은 값으로 취급
	TestTrue(TEXT("50 == 100*0.5"), FPassiveCondition::Compare(50.f, EPassiveCompareOp::Equal, 100.f * 0.5f));
	TestTrue(TEXT("50 != 51"), FPassiveCondition::Compare(50.f, EPassiveCompareOp::NotEqual, 51.f));

	// 배수 판정: 6은 3의 배수, 7은 아님, 0으로는 나눌 수 없어 거짓
	TestTrue(TEXT("6 % 3 == 0"), FPassiveCondition::Compare(6.f, EPassiveCompareOp::ModuloZero, 3.f));
	TestFalse(TEXT("7 % 3 != 0"), FPassiveCondition::Compare(7.f, EPassiveCompareOp::ModuloZero, 3.f));
	TestFalse(TEXT("x % 0 → 거짓"), FPassiveCondition::Compare(6.f, EPassiveCompareOp::ModuloZero, 0.f));

	return true;
}

// 피연산자 계산: 속성(배수) / 태그 하위 합 / 카운터 / 스냅샷 없음
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveConditionResolveTests,
	"P_RD.TAS.Passive.Condition.Resolve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveConditionResolveTests::RunTest(const FString& Parameters)
{
	// 소유자: HP 30 / MaxHP 100, 디버프 약화 1 + 취약 2
	UBoardCombatTargetSnapshotData* Owner = MakeSnapshot(30.f, 100.f, FTileIndex(0, 0));
	Owner->mEffectCounts.Add(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Weakness, 1);
	Owner->mEffectCounts.Add(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Vulnerability, 2);
	Owner->mEffectCounts.Add(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Vigor, 1);

	FPassiveActivateContext Ctx;
	Ctx.mOwnerSnapshot = Owner;

	FDynamicPassiveData_Generic State;
	State.mCounter = 6;

	float Value = 0.f;

	// 속성값 + 배수: MaxHP 100 x 0.5 = 50
	const FPassiveOperand HalfMaxHP = AttrOp(EPassiveOperandSource::Self, UCombatTargetAttributeSet::GetMaxHPAttribute(), 0.5f);
	TestTrue(TEXT("MaxHP 계산 성공"), HalfMaxHP.Resolve(Ctx, INDEX_NONE, State, Value));
	TestEqual(TEXT("MaxHP x 0.5 == 50"), Value, 50.f);

	// 조건식: HP(30) <= MaxHP x 0.5(50)
	FPassiveCondition HalfHP;
	HalfHP.mLhs = AttrOp(EPassiveOperandSource::Self, UCombatTargetAttributeSet::GetHPAttribute());
	HalfHP.mOp = EPassiveCompareOp::LessEqual;
	HalfHP.mRhs = HalfMaxHP;
	TestTrue(TEXT("HP 30 <= 50 통과"), HalfHP.Evaluate(Ctx, INDEX_NONE, State));

	// 태그 개수: 부모 태그(Debuff)로 하위 합 3, Buff는 제외
	FPassiveOperand DebuffCount = KindOp(EPassiveOperandKind::TagCount);
	DebuffCount.mTag = EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff;
	TestTrue(TEXT("TagCount 계산 성공"), DebuffCount.Resolve(Ctx, INDEX_NONE, State, Value));
	TestEqual(TEXT("디버프 합 == 3"), Value, 3.f);

	// 카운터 + 배수 판정: 6 % 3 == 0
	FPassiveCondition Every3rd;
	Every3rd.mLhs = KindOp(EPassiveOperandKind::Counter);
	Every3rd.mOp = EPassiveCompareOp::ModuloZero;
	Every3rd.mRhs = ConstOp(3.f);
	TestTrue(TEXT("카운터 6, 3의 배수 통과"), Every3rd.Evaluate(Ctx, INDEX_NONE, State));

	// 스냅샷 없음: 속성 조건은 탈락, 고정값끼리는 통과
	FPassiveActivateContext EmptyCtx;
	TestFalse(TEXT("스냅샷 없으면 속성 조건 탈락"), HalfHP.Evaluate(EmptyCtx, INDEX_NONE, State));
	FPassiveCondition ConstOnly;
	ConstOnly.mLhs = ConstOp(1.f);
	ConstOnly.mOp = EPassiveCompareOp::Equal;
	ConstOnly.mRhs = ConstOp(1.f);
	TestTrue(TEXT("고정값 조건은 스냅샷 없어도 통과"), ConstOnly.Evaluate(EmptyCtx, INDEX_NONE, State));

	return true;
}

// 위치/캡처 기반 피연산자: 거리 / 캡처값 / 이동 거리 / 미캡처
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveConditionSpatialTests,
	"P_RD.TAS.Passive.Condition.Spatial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveConditionSpatialTests::RunTest(const FString& Parameters)
{
	// 소유자 (0,0), 타겟 0 (2,3), 타겟 1 (1,0)
	FPassiveActivateContext Ctx;
	Ctx.mOwnerSnapshot = MakeSnapshot(100.f, 100.f, FTileIndex(0, 0));
	Ctx.mTargetSnapshots.Add(MakeSnapshot(100.f, 100.f, FTileIndex(2, 3)));
	Ctx.mTargetSnapshots.Add(MakeSnapshot(100.f, 100.f, FTileIndex(1, 0)));

	FDynamicPassiveData_Generic State;
	float Value = 0.f;

	// 거리: 타겟 0은 2+3=5, 타겟 1은 1 (맨해튼 방식)
	const FPassiveOperand Distance = KindOp(EPassiveOperandKind::Distance);
	TestTrue(TEXT("거리 계산 성공"), Distance.Resolve(Ctx, 0, State, Value));
	TestEqual(TEXT("타겟 0 거리 == 5"), Value, 5.f);
	TestTrue(TEXT("거리 계산 성공"), Distance.Resolve(Ctx, 1, State, Value));
	TestEqual(TEXT("타겟 1 거리 == 1"), Value, 1.f);
	TestFalse(TEXT("범위 밖 타겟 인덱스는 실패"), Distance.Resolve(Ctx, 2, State, Value));

	// 이동 거리: 캡처 전엔 실패, 캡처 후 소유자가 (0,0)→현재 (0,0)이면 0, 타겟 0은 (0,3)→(2,3)이면 2
	FPassiveOperand SelfMoved = KindOp(EPassiveOperandKind::MovedDistance, EPassiveOperandSource::Self);
	TestFalse(TEXT("미캡처면 이동 거리 실패"), SelfMoved.Resolve(Ctx, INDEX_NONE, State, Value));
	State.mCapturedSelfTile = FTileIndex(0, 0);
	State.mCapturedTargetTiles = { FTileIndex(0, 3), FTileIndex(1, 0) };
	TestTrue(TEXT("소유자 이동 거리 계산 성공"), SelfMoved.Resolve(Ctx, INDEX_NONE, State, Value));
	TestEqual(TEXT("소유자 이동 거리 == 0"), Value, 0.f);
	const FPassiveOperand TargetMoved = KindOp(EPassiveOperandKind::MovedDistance, EPassiveOperandSource::Target);
	TestTrue(TEXT("타겟 이동 거리 계산 성공"), TargetMoved.Resolve(Ctx, 0, State, Value));
	TestEqual(TEXT("타겟 0 이동 거리 == 2"), Value, 2.f);

	// 캡처값: 키 "HP"에 소유자 80, 타겟별 [60, 40]. 없는 키는 실패
	FPassiveCaptureSlot Slot;
	Slot.mSelf = 80.f;
	Slot.mTargets = { 60.f, 40.f };
	State.mCaptures.Add(TEXT("HP"), Slot);
	FPassiveOperand CapturedSelf = KindOp(EPassiveOperandKind::Captured, EPassiveOperandSource::Self);
	CapturedSelf.mCaptureKey = TEXT("HP");
	TestTrue(TEXT("소유자 캡처값 계산 성공"), CapturedSelf.Resolve(Ctx, INDEX_NONE, State, Value));
	TestEqual(TEXT("소유자 캡처값 == 80"), Value, 80.f);
	FPassiveOperand CapturedTarget = KindOp(EPassiveOperandKind::Captured, EPassiveOperandSource::Target);
	CapturedTarget.mCaptureKey = TEXT("HP");
	TestTrue(TEXT("타겟 캡처값 계산 성공"), CapturedTarget.Resolve(Ctx, 1, State, Value));
	TestEqual(TEXT("타겟 1 캡처값 == 40"), Value, 40.f);
	CapturedSelf.mCaptureKey = TEXT("None");
	TestFalse(TEXT("없는 캡처 키는 실패"), CapturedSelf.Resolve(Ctx, INDEX_NONE, State, Value));

	return true;
}

// 조건 배열 판정: 빈 배열 / 전부 통과 / 하나 탈락
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPassiveConditionEvaluateAllTests,
	"P_RD.TAS.Passive.Condition.EvaluateAll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FPassiveConditionEvaluateAllTests::RunTest(const FString& Parameters)
{
	// 소유자 HP 30 / MaxHP 100, 카운터 6
	FPassiveActivateContext Ctx;
	Ctx.mOwnerSnapshot = MakeSnapshot(30.f, 100.f, FTileIndex(0, 0));
	FDynamicPassiveData_Generic State;
	State.mCounter = 6;

	// 빈 배열은 통과
	TArray<FPassiveCondition> Conditions;
	TestTrue(TEXT("빈 배열 통과"), PassiveConditionUtils::EvaluateAll(Conditions, Ctx, INDEX_NONE, State));

	// 조건 1: HP <= MaxHP x 0.5 (통과)
	FPassiveCondition HalfHP;
	HalfHP.mLhs = AttrOp(EPassiveOperandSource::Self, UCombatTargetAttributeSet::GetHPAttribute());
	HalfHP.mOp = EPassiveCompareOp::LessEqual;
	HalfHP.mRhs = AttrOp(EPassiveOperandSource::Self, UCombatTargetAttributeSet::GetMaxHPAttribute(), 0.5f);
	Conditions.Add(HalfHP);

	// 조건 2: 카운터 % 3 == 0 (통과)
	FPassiveCondition Every3rd;
	Every3rd.mLhs = KindOp(EPassiveOperandKind::Counter);
	Every3rd.mOp = EPassiveCompareOp::ModuloZero;
	Every3rd.mRhs = ConstOp(3.f);
	Conditions.Add(Every3rd);
	TestTrue(TEXT("두 조건 모두 통과"), PassiveConditionUtils::EvaluateAll(Conditions, Ctx, INDEX_NONE, State));

	// 카운터를 7로 바꾸면 조건 2 탈락 → 전체 탈락
	State.mCounter = 7;
	TestFalse(TEXT("하나 탈락하면 전체 탈락"), PassiveConditionUtils::EvaluateAll(Conditions, Ctx, INDEX_NONE, State));

	return true;
}
