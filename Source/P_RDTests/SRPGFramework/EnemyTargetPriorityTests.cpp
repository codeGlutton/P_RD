#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "SRPGFramework/EnemyTargetPriorityTestsHelper.h"
#include "SRPGFramework/SRPGEnemyTurnPlanner.h"
#include "SRPGFramework/SRPGSkillAction.h"
#include "Actor/TileMap/TileMapModel.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "GameplayTagType.h"
#include "SRPGFramework/SRPGMoveAction.h"
#include "Simulation/RoomInstance.h"
#include "Misc/DataValidation.h"
#include "Setting/GameTeamType.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace
{
struct FTargetFixture
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    URoomInstance* Room = NewObject<URoomInstance>(World);
    UTileMapModel* Map = NewObject<UTileMapModel>(Room);
    UMockEnemyUnitModel* Enemy = NewObject<UMockEnemyUnitModel>(Room);
    UStaticEnemyUnitSpawnData* Data = NewObject<UStaticEnemyUnitSpawnData>(World);
    UStaticUnitSkillData* Skill = NewObject<UStaticUnitSkillData>(World);
    TArray<UUnitModel*> Players;

    FTargetFixture()
    {
        Map->SetDimensions(12, 5);
        Room->mAliveWorldModels.Add(100, Map);
        Room->mAliveWorldModels.Add(101, Enemy);
        Enemy->Initialize();
        Enemy->BeginPlay();
        Enemy->SetStaticSpawnData(Data);
        Skill->mJobType = EUnitJobType::Common;
        Skill->mSkillType = ESkillType::Attack;
        Skill->mRequiredActionPoint = 1;
        Skill->mAimPattern = EAimPattern::Square;
        Skill->mAimRange = 10;
        Skill->mCanAimBoardActor = true;
        Skill->mEffectPattern = EEffectPattern::Single;
        Skill->mSkillPhaseLayers.AddDefaulted();
        Data->mSkillDatas.Add(Skill);
        SetPolicy(0, EEnemyTargetPriority::Nearest);
        Map->PlaceActor(FTileTransform(FTileIndex(1, 2)), Enemy);
        Enemy->GetAttributeComponentModel()->ApplyModToAttribute(
            UUnitAttributeSet::GetActionPointAttribute(), ETacticalModOp::Override, 1.f);
        AddPlayer(FTileIndex(2, 2), 50, 100, 1);
        AddPlayer(FTileIndex(4, 2), 20, 40, 2);
        AddPlayer(FTileIndex(6, 2), 30, 300, 5);
    }
    void SetPolicy(int32 SkillIndex, EEnemyTargetPriority Priority)
    {
        Data->mTargetPolicyOverrides[SkillIndex].mIsOverrided = true;
        Data->mTargetPolicyOverrides[SkillIndex].mTargetPolicy.mPriority = Priority;
        // Exercise the actual DA -> model initialization, not a test-only field setter.
        Enemy->UEnemyUnitModel::PostInitializeComponentModels();
    }
    void AddPlayer(FTileIndex Tile, float HP, float MaxHP, float Attack)
    {
        auto* Player = NewObject<UMockTargetPolicyPlayerUnitModel>(Room);
        Player->Initialize();
        auto* Attributes = Player->GetAttributeComponentModel();
        Attributes->AddAttributeSet<UPlayerUnitAttributeSet>();
        Attributes->ApplyModToAttribute(UCombatTargetAttributeSet::GetMaxHPAttribute(), ETacticalModOp::Override, MaxHP);
        Attributes->ApplyModToAttribute(UCombatTargetAttributeSet::GetHPAttribute(), ETacticalModOp::Override, HP);
        Attributes->ApplyModToAttribute(UCombatTargetAttributeSet::GetAttackFactorAttribute(), ETacticalModOp::Override, Attack);
        Room->mAliveWorldModels.Add(Players.Num(), Player);
        Players.Add(Player);
        Map->PlaceActor(FTileTransform(Tile), Player);
    }
    TArray<TInstancedStruct<FSRPGCommand>> Plan(int32 Seed = 12345)
    {
        const FRandomStream Stream(Seed);
        return USRPGEnemyTurnPlanner::PlanTurn(Enemy, Players, Map, Stream);
    }
    FTileIndex Target(int32 Seed = 12345)
    {
        for (const auto& Command : Plan(Seed))
            if (Command.Get().GetCommandType() == ESRPGCommandType::SkillCast)
                return Command.Get<FSRPGSkillCastCommand>().mTargetIndex;
        return FTileIndex::Invalid;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyTargetPriorityTest, "P_RD.SRPG.EnemyTargetPriority.Ranking",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyTargetPriorityTest::RunTest(const FString&)
{
    FTargetFixture F;
    TestTrue(TEXT("Existing DAs default to nearest"), F.Enemy->GetTargetPolicyOverride(0).mTargetPolicy.mPriority == EEnemyTargetPriority::Nearest);
    TestTrue(TEXT("Nearest"), F.Target() == FTileIndex(2, 2));
    F.SetPolicy(0, EEnemyTargetPriority::LowestHP);
    TestTrue(TEXT("DA policy reaches model"), F.Enemy->GetTargetPolicyOverride(0).mTargetPolicy.mPriority == EEnemyTargetPriority::LowestHP);
    TestTrue(TEXT("Lowest HP can beat a nearer target"), F.Target() == FTileIndex(4, 2));
    F.SetPolicy(0, EEnemyTargetPriority::LowestHPPercent);
    TestTrue(TEXT("HP ratio differs from absolute HP"), F.Target() == FTileIndex(6, 2));
    F.SetPolicy(0, EEnemyTargetPriority::HighestMaxHP);
    TestTrue(TEXT("Highest max HP"), F.Target() == FTileIndex(6, 2));
    F.SetPolicy(0, EEnemyTargetPriority::Farthest);
    TestTrue(TEXT("Farthest reachable path"), F.Target() == FTileIndex(6, 2));
    F.SetPolicy(0, EEnemyTargetPriority::HighestMaxHP);
    F.Players[0]->GetAttributeComponentModel()->ApplyModToAttribute(
        UCombatTargetAttributeSet::GetMaxHPAttribute(), ETacticalModOp::Override, 300.f);
    TestTrue(TEXT("Equal stats prefer shortest path"), F.Target() == FTileIndex(2, 2));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyTargetCandidateTest, "P_RD.SRPG.EnemyTargetPriority.LegalCandidates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyTargetCandidateTest::RunTest(const FString&)
{
    FTargetFixture F;
    F.SetPolicy(0, EEnemyTargetPriority::LowestHP);
    F.Skill->mAimRange = 1;
    TestTrue(TEXT("Cannot chase low HP and miss an affordable nearby attack"), F.Target() == FTileIndex(2, 2));
    F.Skill->mAimRange = 10;
    F.Players[1]->GetAttributeComponentModel()->AddLooseGameplayTag(EffectTags::GameplayEffect_ActorState_Dead);
    TestTrue(TEXT("Dead target is excluded"), F.Target() == FTileIndex(6, 2));
    F.Players.Add(nullptr);
    TestTrue(TEXT("Null candidate does not alter target indices"), F.Target() == FTileIndex(6, 2));
    F.Players.Empty();
    const auto Commands = F.Plan();
    TestTrue(TEXT("No targets safely ends the turn"), Commands.Num() == 1 && Commands[0].Get().GetCommandType() == ESRPGCommandType::TurnEnd);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyTargetRandomTest, "P_RD.SRPG.EnemyTargetPriority.SeededRandom",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyTargetRandomTest::RunTest(const FString&)
{
    FTargetFixture F;
    F.SetPolicy(0, EEnemyTargetPriority::Random);
    TSet<int32> SelectedColumns;
    for (int32 Seed = 0; Seed < 24; ++Seed)
    {
        const FTileIndex A = F.Target(Seed);
        TestTrue(TEXT("Same board and seed produce same target"), A == F.Target(Seed));
        TestTrue(TEXT("Random selects a legal candidate"), A == FTileIndex(2,2) || A == FTileIndex(4,2) || A == FTileIndex(6,2));
        SelectedColumns.Add(A.mX);
    }
    TestTrue(TEXT("Random policy is not fixed to nearest"), SelectedColumns.Num() > 1);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyTargetChaseTest, "P_RD.SRPG.EnemyTargetPriority.Chase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyTargetChaseTest::RunTest(const FString&)
{
    FTargetFixture F;
    F.Skill->mAimRange = 1;
    F.SetPolicy(0, EEnemyTargetPriority::LowestHP);
    TestTrue(TEXT("Default takes affordable attack"), F.Target() == FTileIndex(2,2));
    F.Data->mTargetPolicyOverrides[0].mTargetPolicy.mUnreachableBehavior = EEnemyUnreachableTargetBehavior::ChasePreferred;
    F.SetPolicy(0, EEnemyTargetPriority::LowestHP);
    for (const EMoveTendency Tendency : {EMoveTendency::MoveClose, EMoveTendency::HoldRange, EMoveTendency::MoveAway})
    {
        F.Enemy->SetMoveTendency(Tendency);
        const auto Commands = F.Plan();
        TestTrue(TEXT("Chase doesn't attack available bystander"), F.Target() == FTileIndex::Invalid);
        bool Moved = false;
        for (const auto& Command : Commands)
        {
            if (Command.Get().GetCommandType() == ESRPGCommandType::MoveCast)
            {
                const auto& Path = Command.Get<FSRPGMoveCommand>().mPathTileIndexes;
                Moved = Path.Num() > 1;
            }
        }
        TestTrue(TEXT("All movement tendencies approach unreachable preferred target"), Moved);
        TestTrue(TEXT("Chase still ends turn"), Commands.Last().Get().GetCommandType() == ESRPGCommandType::TurnEnd);
    }
    F.Skill->mAimRange = 10;
    TestTrue(TEXT("Preferred target attacked when reachable"), F.Target() == FTileIndex(4,2));
    F.Skill->mAimRange = 1;
    F.Enemy->GetAttributeComponentModel()->AddLooseGameplayTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root);
    TestTrue(TEXT("Rooted chase safely waits"), F.Plan().Num() == 1);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyTargetSkillOverrideTest, "P_RD.SRPG.EnemyTargetPriority.SkillOverrideAndStatus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyTargetSkillOverrideTest::RunTest(const FString&)
{
    FTargetFixture F;
    const FGameplayTag Status = EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Root;
    F.Players[0]->GetAttributeComponentModel()->AddLooseGameplayTag(Status);
    F.Skill->mTargetPolicy.mPriority = EEnemyTargetPriority::WithStatus;
    F.Skill->mTargetPolicy.mStatusTag = Status;
    TestTrue(TEXT("Status combo targets affected unit"), F.Target() == FTileIndex(2,2));
    F.Skill->mTargetPolicy.mStatusTag = FGameplayTag();
    TestTrue(TEXT("Unconfigured status falls back safely"), F.Target() == FTileIndex(2,2));
    FDataValidationContext Context;
    TestTrue(TEXT("Missing status tag rejects skill config"), F.Skill->IsDataValid(Context) == EDataValidationResult::Invalid);
    TestTrue(TEXT("Specific missing-tag validation error"), Context.GetIssues().ContainsByPredicate([](const FDataValidationContext::FIssue& Issue) { return Issue.Message.ToString().Contains(TEXT("Status Tag")); }));
    F.Skill->mTargetPolicy.mPriority = EEnemyTargetPriority::LowestHP;
    F.Skill->mTargetPolicy.mUnreachableBehavior = EEnemyUnreachableTargetBehavior::ChasePreferred;
    F.Skill->mAimRange = 1;
    TestTrue(TEXT("Skill override also controls chase"), F.Target() == FTileIndex::Invalid);
    F.Skill->mSkillType = ESkillType::Spell;
    TestTrue(TEXT("Self spell ignores target policy and still casts"), F.Target() != FTileIndex::Invalid);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyTargetAreaTest, "P_RD.SRPG.EnemyTargetPriority.AreaCoverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyTargetAreaTest::RunTest(const FString&)
{
    FTargetFixture F;
    F.SetPolicy(0, EEnemyTargetPriority::MostTargets);
    F.Skill->mEffectPattern = EEffectPattern::Square;
    F.Skill->mEffectArea = 1;
    F.Skill->mAimBlockerMask = 0;
    F.Skill->mEffectBlockerMask = 0;
    // (5,2) is empty, but an area cast there hits (4,2), (6,2), (5,3).
    F.AddPlayer(FTileIndex(5,3), 80, 100, 1);
    const auto Aim = F.Target();
    const auto Tiles = F.Map->GetEffectTiles(Aim, F.Skill->mEffectPattern, 1, ETileLayerFlag::None);
    TestTrue(TEXT("Best area covers three enemies including from empty aim tile"),
        Tiles.Contains(FTileIndex(4,2)) && Tiles.Contains(FTileIndex(6,2)) && Tiles.Contains(FTileIndex(5,3)));
    F.Skill->mCanAimBoardActor = false;
    TestTrue(TEXT("Area attack works when occupied aim tiles are forbidden"), F.Target() != FTileIndex::Invalid);
    // Repeated phases count each living target once; friendly-only phases cannot inflate coverage.
    const FSkillPhaseLayer ExtraPhase = F.Skill->mSkillPhaseLayers[0];
    F.Skill->mSkillPhaseLayers.Add(ExtraPhase);
    TestTrue(TEXT("Multi hit doesn't alter distinct-target scoring"), F.Target() == Aim);
    for (auto& Phase : F.Skill->mSkillPhaseLayers) Phase.mTeamAttitudeFilter = static_cast<int32>(ETeamAttitudeFilter::Friendly);
    TestTrue(TEXT("Friendly-only effect never scores hostile targets"), F.Target() == FTileIndex::Invalid);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyTargetCloneTest, "P_RD.SRPG.EnemyTargetPriority.RoomClone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyTargetCloneTest::RunTest(const FString&)
{
    FTargetFixture F;
    for (auto Priority : {EEnemyTargetPriority::Random, EEnemyTargetPriority::LowestHP, EEnemyTargetPriority::MostTargets})
    {
        F.SetPolicy(0, Priority);
        // Same UObject duplication boundary used by SimulationSubsystem, including cross references.
        URoomInstance* Copy = CastChecked<URoomInstance>(StaticDuplicateObject(F.Room, F.World));
        auto* Enemy = CastChecked<UMockEnemyUnitModel>(Copy->mAliveWorldModels[101]);
        auto* Map = CastChecked<UTileMapModel>(Copy->mAliveWorldModels[100]);
        TArray<UUnitModel*> Players;
        for (int32 Index = 0; Index < F.Players.Num(); ++Index)
            Players.Add(CastChecked<UUnitModel>(Copy->mAliveWorldModels[Index]));
        TestTrue(TEXT("Duplicated player has same team identity"), Players[0]->GetGenericTeamId() == F.Players[0]->GetGenericTeamId());
        TestTrue(TEXT("Tile map refers to cloned player"), Map->GetActorsOnTile(Players[0]->GetTileTransform().mIndex).Contains(Players[0]));
        TestTrue(TEXT("Room duplication preserves DA policy"), Enemy != F.Enemy && Enemy->GetTargetPolicyOverride(0).mTargetPolicy.mPriority == Priority);
        FRandomStream Live(923), Preview(923);
        const auto A = USRPGEnemyTurnPlanner::PlanTurn(F.Enemy, F.Players, F.Map, Live);
        const auto B = USRPGEnemyTurnPlanner::PlanTurn(Enemy, Players, Map, Preview);
        TestEqual(TEXT("Duplicated room has same command count"), A.Num(), B.Num());
        for (int32 Index = 0; Index < FMath::Min(A.Num(), B.Num()); ++Index)
        {
            TestTrue(TEXT("Duplicated room has same command type"), A[Index].Get().GetCommandType() == B[Index].Get().GetCommandType());
            if (A[Index].Get().GetCommandType() == ESRPGCommandType::SkillCast && B[Index].Get().GetCommandType() == ESRPGCommandType::SkillCast)
                TestTrue(TEXT("Duplicated room has same target"), A[Index].Get<FSRPGSkillCastCommand>().mTargetIndex == B[Index].Get<FSRPGSkillCastCommand>().mTargetIndex);
            if (A[Index].Get().GetCommandType() == ESRPGCommandType::MoveCast && B[Index].Get().GetCommandType() == ESRPGCommandType::MoveCast)
                TestTrue(TEXT("Duplicated room has same move"), A[Index].Get<FSRPGMoveCommand>().mPathTileIndexes == B[Index].Get<FSRPGMoveCommand>().mPathTileIndexes);
        }
        TestEqual(TEXT("Preview and live consume equal random draws"), Live.GetCurrentSeed(), Preview.GetCurrentSeed());
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyRealDATest, "P_RD.SRPG.EnemyTargetPriority.AuthoredMonsterDA",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnemyRealDATest::RunTest(const FString&)
{
    for (const TCHAR* Path : {
        TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_SpiderUnit.DA_SpiderUnit"),
        TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage1/DA_EagleUnit.DA_EagleUnit"),
        TEXT("/Game/BP/DataAsset/Unit/EnemyUnit/Stage2/DA_SkeletonBirdUnit.DA_SkeletonBirdUnit")})
    {
        FTargetFixture F;
        auto* Original = LoadObject<UStaticEnemyUnitSpawnData>(nullptr, Path);
        if (!TestNotNull(TEXT("Authored monster DA loads"), Original)) continue;
        F.Data = DuplicateObject<UStaticEnemyUnitSpawnData>(Original, F.World);
        F.Enemy->SetStaticSpawnData(F.Data);
        F.SetPolicy(0, EEnemyTargetPriority::LowestHP);
        F.Enemy->GetAttributeComponentModel()->ApplyModToAttribute(UUnitAttributeSet::GetActionPointAttribute(), ETacticalModOp::Override, 12.f);
        TestTrue(*FString::Printf(TEXT("%s attacks lowest HP using authored skills/range/cost"), Path), F.Target() == FTileIndex(4,2));
        TestTrue(TEXT("Original asset is unchanged"), Original->mTargetPolicyOverrides[0].mTargetPolicy.mPriority == EEnemyTargetPriority::Nearest);
    }
    return !HasAnyErrors();
}
#endif
