/*****************************************************************//**
 * @file   SkillForcedMoveSyncTests.cpp
 * @brief  밀당 페이즈 동기화 유닛테스트
 * @details
 *  밀치기/당기기가 있는 페이즈는 피격자 이동이 끝난 뒤에 종료되고, 광역은 대상 하나씩 순차 출발하는지 검증.
 *  배리어 구독자가 없으면 동기 완주(시뮬레이션), 테스트가 배리어를 잡고 있으면 라이브처럼 단계별 진행.
 * @author 이문환
 * @date   2026-09-20
 *********************************************************************/

#include "P_RDTests.h"
#include "Misc/AutomationTest.h"

#include "Actor/BoardActor/GimmickTestsHelper.h"                        // UMockOverlapGimmickModel, UMockGimmickVictimUnitModel
#include "Component/BoardMovementComponent/BoardMovementTestsHelper.h"  // UMockUnitMovementComponentModel

#include "GameplayTagType.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Animation/BoardActorAnimType.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Push.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Stun.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "TAS/Effect/Cooldown/TacticalEffect_Cooldown.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
	// @brief 테스트용 월드 획득 (PIE/Game 우선)
	UWorld* GetAnyGameWorldForForcedMoveTests()
	{
		if (GEngine == nullptr)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World() != nullptr)
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	/* 스킬 데이터 */

	// @brief 스킬 공통 골격 (단타, 자기 타일 조준, 전 팀 타격). 영향 범위는 호출자가 지정
	UStaticSkillData* MakeSkillDataBase(UObject* Outer, EEffectPattern EffectPattern, int32 EffectArea)
	{
		UStaticSkillData* SkillData = NewObject<UStaticSkillData>(Outer);
		// 스킬 시전 시 쿨다운 스펙을 만들므로 라운드 쿨다운 지정 (미지정 시 스펙이 무효)
		SkillData->mCooldownEffectClass = UTacticalEffect_RoundCooldown::StaticClass();
		SkillData->mSkillAnimationSet.mApplyMotionTags.Add(AnimationTags::Animation_Montage_Skill_Melee_Punch);
		SkillData->mSkillAnimationSet.mAutoRotateTowardTarget = false;
		SkillData->mTargetPattern = ETargetPattern::TargetOnly;
		SkillData->mEffectPattern = EffectPattern;
		SkillData->mEffectArea = EffectArea;

		FSkillPhaseLayer Phase;
		Phase.mTeamAttitudeFilter = StaticCast<int32>(ETeamAttitudeFilter::All);
		SkillData->mSkillPhaseLayers.Add(Phase);
		return SkillData;
	}

	// @brief 밀치기 레이어 (TagGain = 밀칠 칸 수)
	TInstancedStruct<FSkillEffectLayer> MakePushLayer(int32 PushDistance)
	{
		TInstancedStruct<FSkillEffectLayer> Layer;
		Layer.InitializeAs<FSkillEffectLayer_Push>();
		Layer.GetMutable<FSkillEffectLayer_Push>().mTagGain = PushDistance;
		return Layer;
	}

	// @brief 기절 레이어 (밀당이 아닌 이펙트 대표)
	TInstancedStruct<FSkillEffectLayer> MakeStunLayer()
	{
		TInstancedStruct<FSkillEffectLayer> Layer;
		Layer.InitializeAs<FSkillEffectLayer_Stun>();
		Layer.GetMutable<FSkillEffectLayer_Stun>().mTagGain = 1;
		return Layer;
	}

	// @brief 시전자 스킬: 자기 타일 기준 사각형 2칸 범위 밀치기 (사각형은 차단 레이어와 무관하게 범위 안을 전부 포함하므로 일직선 두 명도 같이 맞음)
	UStaticSkillData* MakeAreaPushSkillData(UObject* Outer, int32 PushDistance)
	{
		UStaticSkillData* SkillData = MakeSkillDataBase(Outer, EEffectPattern::Square, 2);
		SkillData->mSkillPhaseLayers[0].mSkillEffectLayers.Add(MakePushLayer(PushDistance));
		return SkillData;
	}

	// @brief 발판 스킬: 밟은 자리 한 칸, 발판 방향으로 밀치기
	UStaticSkillData* MakeTrapPushSkillData(UObject* Outer, int32 PushDistance)
	{
		UStaticSkillData* SkillData = MakeSkillDataBase(Outer, EEffectPattern::Single, 0);
		SkillData->mSkillPhaseLayers[0].mSkillEffectLayers.Add(MakePushLayer(PushDistance));
		return SkillData;
	}

	/* 픽스처 */

	// @brief 타일맵 주입 기믹 생성/배치. 시전자 또는 발판으로 사용
	UMockOverlapGimmickModel* MakeGimmick(UWorld* World, UTileMapModel* TileMap, const FTileTransform& Transform, UStaticSkillData* SkillData, int32 TriggerCount)
	{
		UMockOverlapGimmickModel* Gimmick = NewObject<UMockOverlapGimmickModel>(World);
		Gimmick->Initialize();
		Gimmick->BeginPlay();
		Gimmick->SetTileMap(TileMap);
		Gimmick->GetSkillComponentModel()->SetSkillFrom(TArray<TSoftObjectPtr<UStaticSkillData>>{ SkillData });
		Gimmick->SetRemainingTriggerCount(TriggerCount);
		TileMap->PlaceActor(Transform, Gimmick);
		return Gimmick;
	}

	// @brief 피격 유닛 생성/배치 (타일맵 주입 이동 컴포넌트 연결)
	UMockGimmickVictimUnitModel* MakeVictim(UWorld* World, UTileMapModel* TileMap, const FTileTransform& Transform)
	{
		UMockGimmickVictimUnitModel* Unit = NewObject<UMockGimmickVictimUnitModel>(World);
		Unit->Initialize();
		Unit->BeginPlay();
		UMockUnitMovementComponentModel* Movement = NewObject<UMockUnitMovementComponentModel>(Unit);
		Movement->SetTileMap(TileMap);
		Unit->SetBoardMovementComponentModel(Movement);
		TileMap->PlaceActor(Transform, Unit);
		return Unit;
	}

	// @brief 공용 픽스처: 8x8 타일맵 + (2,2) 시전자. 스킬은 ForcedActivateSkill로 직접 시전 (오버랩 경유 안 함)
	struct FSyncFixture
	{
		UTileMapModel* TileMap = nullptr;
		UMockOverlapGimmickModel* Caster = nullptr;
		USkillComponentModel* SkillComp = nullptr;

		// @brief 시전자 자기 타일 조준으로 시전 (사각형 범위가 주변 2칸을 덮음)
		void Cast() const
		{
			SkillComp->ForcedActivateSkill(TileMap, 0, Caster->GetTileTransform().mIndex);
		}
	};

	FSyncFixture MakeSyncFixture(UWorld* World, UStaticSkillData* SkillData)
	{
		FSyncFixture Fixture;
		Fixture.TileMap = NewObject<UTileMapModel>(World);
		Fixture.TileMap->SetDimensions(8, 8);
		// 시전자는 오버랩으로 발동하지 않도록 수명 0
		Fixture.Caster = MakeGimmick(World, Fixture.TileMap, FTileTransform(FTileIndex(2, 2), ETileActorDirection::Forward), SkillData, 0);
		Fixture.SkillComp = Fixture.Caster->GetSkillComponentModel();
		return Fixture;
	}

	/* 관측 */

	// @brief 스킬 페이즈/종료 이벤트와 유닛 이동 종료 순서 기록
	struct FEventRecorder
	{
		TArray<FString> mEvents;
		int32 mPhaseEndCount = 0;
		int32 mSkillEndCount = 0;

		void BindSkill(USkillComponentModel* SkillComp)
		{
			SkillComp->OnEndPhaseLayerUI.AddLambda([this](int32) {
				++mPhaseEndCount;
				mEvents.Add(TEXT("PhaseEnd"));
				});
			SkillComp->OnEndSkillUI.AddLambda([this](const FActiveSkillContext&, const UStaticSkillData*) {
				++mSkillEndCount;
				mEvents.Add(TEXT("SkillEnd"));
				});
		}

		void BindMove(UBoardActorModel* Unit, const FString& Name)
		{
			Unit->OnEndMovePath.AddLambda([this, Name](const FTileTransform&, const FTransform&) {
				mEvents.Add(Name + TEXT(":MoveEnd"));
				});
		}

		FString Joined() const
		{
			return FString::Join(mEvents, TEXT(" > "));
		}
	};

	/* 라이브 흉내 */

	// @brief 유닛의 스텝 배리어를 잡아 두고 하나씩 풀어 이동을 단계별로 진행
	struct FStepHolder
	{
		TArray<TSharedPtr<FPresentationBarrier>> mHeld;

		void Bind(UBoardActorModel* Unit)
		{
			Unit->OnStartMoveStep.AddLambda([this](const FTileTransform&, const FTransform&, TSharedPtr<FPresentationBarrier> Barrier, float, EBoardMoveMode) {
				mHeld.Add(Barrier);
				});
		}

		bool IsHolding() const
		{
			return mHeld.IsEmpty() == false;
		}

		// @brief 가장 오래된 스텝 하나 풀기. 푸는 도중 다음 스텝이 들어와 배열이 바뀌어도 안전하도록 먼저 빼낸 뒤 Reset
		bool ReleaseNext()
		{
			if (mHeld.IsEmpty() == true)
			{
				return false;
			}
			TSharedPtr<FPresentationBarrier> Barrier = mHeld[0];
			mHeld.RemoveAt(0);
			Barrier.Reset();
			return true;
		}

		// @brief 이 유닛의 이동이 끝날 때까지 전부 풀기
		void ReleaseAll()
		{
			while (ReleaseNext() == true)
			{
			}
		}
	};

	// @brief 시전자 애니메이션 배리어를 잡아 두고, 노티파이 발화와 연출 종료를 테스트가 제어
	struct FMotionHolder
	{
		TSharedPtr<FPresentationBarrier> mHeld;
		FBoardActorAnimationContext mContext;

		void Bind(UBoardActorModel* Caster)
		{
			Caster->OnPlayAnimationUI.AddLambda([this](TSharedPtr<FPresentationBarrier> Barrier, const FBoardActorAnimationContext& Context) {
				mHeld = Barrier;
				mContext = Context;
				});
		}

		// @brief HitLogic 노티파이 발화 -> TriggerPhaseLayer
		void FireHitLogic()
		{
			mContext.mMontageEvents.FindChecked(AnimationTags::Animation_Event_Skill_HitLogic).OnTriggerAnimationEvent.Broadcast(mContext, nullptr, nullptr);
		}

		// @brief 연출 종료 -> 스킬 종료 배리어 해제
		void Release()
		{
			mHeld.Reset();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkillForcedMoveSyncSimTests,
	"P_RD.SRPG.SkillComponent.ForcedMoveSync.Sim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 시뮬레이션(배리어 구독자 없음) 동기 완주 검증
 *  1) 밀치기 1명: 도착 위치, 이벤트 순서 이동끝 -> 페이즈끝 -> 스킬끝
 *  2) 밀리다 발판 밟고 재밀림: 페이즈 끝 시점 위치 == 최종 위치 (함정 연쇄가 페이즈 안에 포함)
 *  3) 면역 대상: 안 밀리고 페이즈/스킬 정상 종료
 */
bool FSkillForcedMoveSyncSimTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForForcedMoveTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	/* Case1: 기본 */
	AddInfo(TEXT("=== Case1: (3,2) 대상 +x로 2칸 밀림 -> (5,2). 순서 이동끝 > 페이즈끝 > 스킬끝 ==="));
	{
		FSyncFixture Fixture = MakeSyncFixture(World, MakeAreaPushSkillData(World, 2));
		UMockGimmickVictimUnitModel* UnitA = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(3, 2), ETileActorDirection::Forward));

		FEventRecorder Recorder;
		Recorder.BindSkill(Fixture.SkillComp);
		Recorder.BindMove(UnitA, TEXT("A"));

		Fixture.Cast();

		TestTrue(TEXT("[Case1] A (5,2) 도착"), UnitA->GetTileTransform().mIndex == FTileIndex(5, 2));
		TestEqual(TEXT("[Case1] 이벤트 순서"), Recorder.Joined(), FString(TEXT("A:MoveEnd > PhaseEnd > SkillEnd")));
		TestFalse(TEXT("[Case1] 스킬 종료 상태"), Fixture.SkillComp->IsAnySkillActivated());
	}

	/* Case2: 함정 연쇄 */
	AddInfo(TEXT("=== Case2: 경로 위 (4,2) 발판(+y 1칸) -> (4,3) 도착. 페이즈 끝 시점 위치 == 최종 위치 ==="));
	{
		FSyncFixture Fixture = MakeSyncFixture(World, MakeAreaPushSkillData(World, 2));
		UMockGimmickVictimUnitModel* UnitA = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(3, 2), ETileActorDirection::Forward));
		// 발판이 Right(+y)를 바라봄. 밀리던 A가 (4,2)를 밟으면 +y로 1칸 재밀림
		UMockOverlapGimmickModel* Trap = MakeGimmick(World, Fixture.TileMap, FTileTransform(FTileIndex(4, 2), ETileActorDirection::Right), MakeTrapPushSkillData(World, 1), 1);

		FEventRecorder Recorder;
		Recorder.BindSkill(Fixture.SkillComp);

		// 페이즈 끝 시점의 A 위치 기록
		FTileIndex IndexAtPhaseEnd = FTileIndex::Invalid;
		Fixture.SkillComp->OnEndPhaseLayerUI.AddLambda([&](int32) {
			IndexAtPhaseEnd = UnitA->GetTileTransform().mIndex;
			});

		Fixture.Cast();

		TestTrue(TEXT("[Case2] A 발판 방향으로 재밀린 (4,3) 도착"), UnitA->GetTileTransform().mIndex == FTileIndex(4, 3));
		TestTrue(TEXT("[Case2] 페이즈 끝 시점 위치 == 최종 위치"), IndexAtPhaseEnd == FTileIndex(4, 3));
		TestEqual(TEXT("[Case2] 발판 수명 차감"), Trap->GetRemainingTriggerCount(), 0);
		TestEqual(TEXT("[Case2] 시전자 페이즈 종료 1회"), Recorder.mPhaseEndCount, 1);
		TestEqual(TEXT("[Case2] 시전자 스킬 종료 1회"), Recorder.mSkillEndCount, 1);
	}

	/* Case3: 면역 */
	AddInfo(TEXT("=== Case3: 강제이동 면역 대상 -> 제자리, 페이즈/스킬 정상 종료 ==="));
	{
		FSyncFixture Fixture = MakeSyncFixture(World, MakeAreaPushSkillData(World, 2));
		UMockGimmickVictimUnitModel* UnitA = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(3, 2), ETileActorDirection::Forward));
		UnitA->GetAttributeComponentModel()->AddLooseGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_ForcedMovementImmunity);

		FEventRecorder Recorder;
		Recorder.BindSkill(Fixture.SkillComp);

		Fixture.Cast();

		TestTrue(TEXT("[Case3] A 제자리 (3,2)"), UnitA->GetTileTransform().mIndex == FTileIndex(3, 2));
		TestEqual(TEXT("[Case3] 페이즈 종료 1회"), Recorder.mPhaseEndCount, 1);
		TestEqual(TEXT("[Case3] 스킬 종료 1회"), Recorder.mSkillEndCount, 1);
		TestFalse(TEXT("[Case3] 스킬 종료 상태"), Fixture.SkillComp->IsAnySkillActivated());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkillForcedMoveSyncLiveTests,
	"P_RD.SRPG.SkillComponent.ForcedMoveSync.Live",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 라이브 흉내(스텝/모션 배리어를 테스트가 보유) 단계별 진행 검증
 *  1) 스텝을 하나씩 풀 때마다 위치가 바뀌고, 다 밀려야 페이즈 종료, 모션이 풀려야 스킬 종료
 *  2) 모션이 먼저 끝나도 스킬은 안 끝나고, 밀림이 끝난 뒤 페이즈 종료 -> 스킬 종료 (보류 -> 재개)
 *  3) 밀당 대기 중 두 번째 노티파이는 무시
 */
bool FSkillForcedMoveSyncLiveTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForForcedMoveTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	/* Case1: 페이즈가 밀림을 기다림 */
	AddInfo(TEXT("=== Case1: 스텝 1 -> (4,2), 스텝 2 -> (5,2) 페이즈 종료, 모션 해제 -> 스킬 종료 ==="));
	{
		FSyncFixture Fixture = MakeSyncFixture(World, MakeAreaPushSkillData(World, 2));
		UMockGimmickVictimUnitModel* UnitA = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(3, 2), ETileActorDirection::Forward));

		FEventRecorder Recorder;
		Recorder.BindSkill(Fixture.SkillComp);
		FStepHolder StepsA;
		StepsA.Bind(UnitA);
		FMotionHolder Motion;
		Motion.Bind(Fixture.Caster);

		Fixture.Cast();
		TestTrue(TEXT("[Case1] 시전 후 모션 배리어 보유"), Motion.mHeld.IsValid());
		TestEqual(TEXT("[Case1] 노티파이 전 페이즈 종료 0"), Recorder.mPhaseEndCount, 0);

		Motion.FireHitLogic();
		TestTrue(TEXT("[Case1] 노티파이 직후 A 논리 위치 (4,2)"), UnitA->GetTileTransform().mIndex == FTileIndex(4, 2));
		TestEqual(TEXT("[Case1] 노티파이 직후 페이즈 종료 0"), Recorder.mPhaseEndCount, 0);

		StepsA.ReleaseNext();
		TestTrue(TEXT("[Case1] 스텝 1 해제 후 A (5,2)"), UnitA->GetTileTransform().mIndex == FTileIndex(5, 2));
		TestEqual(TEXT("[Case1] 스텝 1 해제 후 페이즈 종료 0"), Recorder.mPhaseEndCount, 0);

		StepsA.ReleaseNext();
		TestEqual(TEXT("[Case1] 스텝 2 해제 후 페이즈 종료 1"), Recorder.mPhaseEndCount, 1);
		TestEqual(TEXT("[Case1] 모션 해제 전 스킬 종료 0"), Recorder.mSkillEndCount, 0);

		Motion.Release();
		TestEqual(TEXT("[Case1] 모션 해제 후 스킬 종료 1"), Recorder.mSkillEndCount, 1);
		TestFalse(TEXT("[Case1] 스킬 종료 상태"), Fixture.SkillComp->IsAnySkillActivated());
	}

	/* Case2: 연출이 먼저 끝남 */
	AddInfo(TEXT("=== Case2: 노티파이 후 모션 먼저 해제 -> 스킬 종료 0. 스텝 다 풀면 페이즈 종료 > 스킬 종료 ==="));
	{
		FSyncFixture Fixture = MakeSyncFixture(World, MakeAreaPushSkillData(World, 2));
		UMockGimmickVictimUnitModel* UnitA = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(3, 2), ETileActorDirection::Forward));

		FEventRecorder Recorder;
		Recorder.BindSkill(Fixture.SkillComp);
		FStepHolder StepsA;
		StepsA.Bind(UnitA);
		FMotionHolder Motion;
		Motion.Bind(Fixture.Caster);

		Fixture.Cast();
		Motion.FireHitLogic();
		Motion.Release();

		TestEqual(TEXT("[Case2] 모션 해제 후에도 스킬 종료 0 (밀림 대기로 보류)"), Recorder.mSkillEndCount, 0);
		TestTrue(TEXT("[Case2] 스킬 활성 유지"), Fixture.SkillComp->IsAnySkillActivated());

		StepsA.ReleaseAll();

		TestTrue(TEXT("[Case2] A (5,2) 도착"), UnitA->GetTileTransform().mIndex == FTileIndex(5, 2));
		TestEqual(TEXT("[Case2] 이벤트 순서"), Recorder.Joined(), FString(TEXT("PhaseEnd > SkillEnd")));
		TestFalse(TEXT("[Case2] 스킬 종료 상태"), Fixture.SkillComp->IsAnySkillActivated());
	}

	/* Case3: 대기 중 노티파이 무시 */
	AddInfo(TEXT("=== Case3: 스텝 보유 중 노티파이 2회 -> 두 번째 무시, 페이즈 종료 1회, 스킬 정상 종료 ==="));
	{
		FSyncFixture Fixture = MakeSyncFixture(World, MakeAreaPushSkillData(World, 2));
		UMockGimmickVictimUnitModel* UnitA = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(3, 2), ETileActorDirection::Forward));

		FEventRecorder Recorder;
		Recorder.BindSkill(Fixture.SkillComp);
		FStepHolder StepsA;
		StepsA.Bind(UnitA);
		FMotionHolder Motion;
		Motion.Bind(Fixture.Caster);

		Fixture.Cast();
		Motion.FireHitLogic();
		Motion.FireHitLogic();

		TestEqual(TEXT("[Case3] 두 번째 노티파이 후에도 보유 스텝 1개 (새 이동 없음)"), StepsA.mHeld.Num(), 1);

		StepsA.ReleaseAll();
		Motion.Release();

		TestTrue(TEXT("[Case3] A (5,2) 도착"), UnitA->GetTileTransform().mIndex == FTileIndex(5, 2));
		TestEqual(TEXT("[Case3] 페이즈 종료 1회"), Recorder.mPhaseEndCount, 1);
		TestEqual(TEXT("[Case3] 스킬 종료 1회"), Recorder.mSkillEndCount, 1);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkillForcedMoveSyncSequentialTests,
	"P_RD.SRPG.SkillComponent.ForcedMoveSync.Sequential",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 광역 밀치기 순차 출발 검증
 *  1) 노티파이 직후 한 명만 출발. 그 유닛이 다 밀린 뒤 다음 유닛 출발. 둘 다 끝나야 페이즈 종료
 *  2) 일직선 두 명: 시뮬레이션 최종 위치 == 라이브 흉내 최종 위치
 */
bool FSkillForcedMoveSyncSequentialTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForForcedMoveTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	/* Case1: 순차 출발 */
	AddInfo(TEXT("=== Case1: A (3,2), C (2,3) -> 한 명씩 출발, 둘 다 끝나야 페이즈 종료 ==="));
	{
		FSyncFixture Fixture = MakeSyncFixture(World, MakeAreaPushSkillData(World, 2));
		UMockGimmickVictimUnitModel* UnitA = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(3, 2), ETileActorDirection::Forward));
		UMockGimmickVictimUnitModel* UnitC = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(2, 3), ETileActorDirection::Forward));

		FEventRecorder Recorder;
		Recorder.BindSkill(Fixture.SkillComp);
		FStepHolder StepsA;
		StepsA.Bind(UnitA);
		FStepHolder StepsC;
		StepsC.Bind(UnitC);
		FMotionHolder Motion;
		Motion.Bind(Fixture.Caster);

		Fixture.Cast();
		Motion.FireHitLogic();

		// 어느 쪽이 먼저인지는 대상 목록 순서에 달렸으므로, 출발한 쪽을 First로 잡음
		const bool IsAFirst = StepsA.IsHolding();
		FStepHolder& First = IsAFirst ? StepsA : StepsC;
		FStepHolder& Second = IsAFirst ? StepsC : StepsA;
		TestTrue(TEXT("[Case1] 노티파이 직후 한 명은 출발"), First.IsHolding());
		TestFalse(TEXT("[Case1] 노티파이 직후 다른 한 명은 대기"), Second.IsHolding());

		First.ReleaseAll();
		TestTrue(TEXT("[Case1] 첫 유닛 완료 후 다음 유닛 출발"), Second.IsHolding());
		TestEqual(TEXT("[Case1] 첫 유닛 완료 시점 페이즈 종료 0"), Recorder.mPhaseEndCount, 0);

		Second.ReleaseAll();
		TestEqual(TEXT("[Case1] 둘 다 완료 후 페이즈 종료 1"), Recorder.mPhaseEndCount, 1);

		Motion.Release();
		TestTrue(TEXT("[Case1] A (5,2) 도착"), UnitA->GetTileTransform().mIndex == FTileIndex(5, 2));
		TestTrue(TEXT("[Case1] C (2,5) 도착"), UnitC->GetTileTransform().mIndex == FTileIndex(2, 5));
		TestEqual(TEXT("[Case1] 스킬 종료 1회"), Recorder.mSkillEndCount, 1);
	}

	/* Case2: 시뮬레이션 == 라이브 */
	AddInfo(TEXT("=== Case2: A (3,2), B (4,2) 일직선 -> 시뮬레이션과 라이브 흉내의 최종 위치 동일 ==="));
	{
		// 시뮬레이션 (구독자 없음)
		FTileIndex SimA;
		FTileIndex SimB;
		{
			FSyncFixture Fixture = MakeSyncFixture(World, MakeAreaPushSkillData(World, 2));
			UMockGimmickVictimUnitModel* UnitA = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(3, 2), ETileActorDirection::Forward));
			UMockGimmickVictimUnitModel* UnitB = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(4, 2), ETileActorDirection::Forward));

			Fixture.Cast();

			SimA = UnitA->GetTileTransform().mIndex;
			SimB = UnitB->GetTileTransform().mIndex;
			TestFalse(TEXT("[Case2] 시뮬레이션 스킬 종료"), Fixture.SkillComp->IsAnySkillActivated());
		}

		// 라이브 흉내 (스텝 보유 후 들어오는 순서대로 해제)
		FTileIndex LiveA;
		FTileIndex LiveB;
		{
			FSyncFixture Fixture = MakeSyncFixture(World, MakeAreaPushSkillData(World, 2));
			UMockGimmickVictimUnitModel* UnitA = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(3, 2), ETileActorDirection::Forward));
			UMockGimmickVictimUnitModel* UnitB = MakeVictim(World, Fixture.TileMap, FTileTransform(FTileIndex(4, 2), ETileActorDirection::Forward));

			FStepHolder StepsA;
			StepsA.Bind(UnitA);
			FStepHolder StepsB;
			StepsB.Bind(UnitB);
			FMotionHolder Motion;
			Motion.Bind(Fixture.Caster);

			Fixture.Cast();
			Motion.FireHitLogic();

			// 한 번에 한 명만 움직이므로 잡힌 스텝을 찾아 풀기를 반복
			while (StepsA.ReleaseNext() == true || StepsB.ReleaseNext() == true)
			{
			}
			Motion.Release();

			LiveA = UnitA->GetTileTransform().mIndex;
			LiveB = UnitB->GetTileTransform().mIndex;
			TestFalse(TEXT("[Case2] 라이브 스킬 종료"), Fixture.SkillComp->IsAnySkillActivated());
		}

		TestTrue(TEXT("[Case2] A 최종 위치 동일"), SimA == LiveA);
		TestTrue(TEXT("[Case2] B 최종 위치 동일"), SimB == LiveB);
		AddInfo(FString::Printf(TEXT("[Case2] A (%d,%d) B (%d,%d)"), SimA.mX, SimA.mY, SimB.mX, SimB.mY));
	}

	return true;
}

#if WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkillForcedMoveSyncDataValidationTests,
	"P_RD.SRPG.SkillComponent.ForcedMoveSync.DataValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * @brief 밀당 배치 순서 검증 경고
 *  1) 마지막 페이즈가 아닌 곳의 Push -> 경고
 *  2) 같은 페이즈에서 Push 뒤에 다른 이펙트 -> 경고
 *  3) 마지막 페이즈의 마지막 이펙트 -> 경고 없음
 */
bool FSkillForcedMoveSyncDataValidationTests::RunTest(const FString& Parameters)
{
	UWorld* World = GetAnyGameWorldForForcedMoveTests();
	if (World == nullptr)
	{
		World = GWorld;
	}
	if (TestNotNull(TEXT("유효한 UWorld"), World) == false)
	{
		return false;
	}

	// @brief 검증 실행 후 경고 수
	auto CountWarnings = [](const UStaticSkillData* SkillData) -> int32
	{
		FDataValidationContext Context;
		SkillData->IsDataValid(Context);
		return Context.GetNumWarnings();
	};

	/* Case1: 마지막 페이즈 아닌 곳 */
	AddInfo(TEXT("=== Case1: 페이즈 2개, Push가 0번 페이즈 -> 경고 ==="));
	{
		UStaticSkillData* SkillData = MakeSkillDataBase(World, EEffectPattern::Single, 0);
		SkillData->mSkillPhaseLayers[0].mSkillEffectLayers.Add(MakePushLayer(1));
		SkillData->mSkillPhaseLayers.AddDefaulted();
		SkillData->mSkillPhaseLayers[1].mSkillEffectLayers.Add(MakeStunLayer());

		TestTrue(TEXT("[Case1] 경고 1개 이상"), CountWarnings(SkillData) >= 1);
	}

	/* Case2: Push 뒤에 다른 이펙트 */
	AddInfo(TEXT("=== Case2: 페이즈 1개, Push 뒤에 Stun -> 경고 ==="));
	{
		UStaticSkillData* SkillData = MakeSkillDataBase(World, EEffectPattern::Single, 0);
		SkillData->mSkillPhaseLayers[0].mSkillEffectLayers.Add(MakePushLayer(1));
		SkillData->mSkillPhaseLayers[0].mSkillEffectLayers.Add(MakeStunLayer());

		TestTrue(TEXT("[Case2] 경고 1개 이상"), CountWarnings(SkillData) >= 1);
	}

	/* Case3: 규칙 준수 */
	AddInfo(TEXT("=== Case3: 페이즈 2개, 마지막 페이즈의 마지막이 Push -> 경고 없음 ==="));
	{
		UStaticSkillData* SkillData = MakeSkillDataBase(World, EEffectPattern::Single, 0);
		SkillData->mSkillPhaseLayers[0].mSkillEffectLayers.Add(MakeStunLayer());
		SkillData->mSkillPhaseLayers.AddDefaulted();
		SkillData->mSkillPhaseLayers[1].mSkillEffectLayers.Add(MakeStunLayer());
		SkillData->mSkillPhaseLayers[1].mSkillEffectLayers.Add(MakePushLayer(1));

		TestEqual(TEXT("[Case3] 경고 0개"), CountWarnings(SkillData), 0);
	}

	return true;
}

#endif
