// Fill out your copyright notice in the Description page of Project Settings.


#include "CommandLogFunctionLibrary.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "SRPGFramework/SRPGFrameworkType.h"


bool UCommandLogFunctionLibrary::CalculateSkillCommandLog(
    FTileMapCloneData CloneTileMap,
    const FCommandLogFunctionContext& Context,
    FCommandLog& Out_Result)
{

    float TileSize = CloneTileMap.mTiles.Num() + 1;
    UE_LOG(LogTemp, Warning, TEXT("타일 크기 : %f"), TileSize);

    // 캐스터와 스킬 데이터를 가지고 기본 효과 값을 계산한다.
    FEffectPacket EffectPacket = CalculateDefaultSkillEffectValue(CloneTileMap.mTileActorDatas[Context.mSourceActorID], Context.mSkillData);

    // 스킬 패시브를 돌려서 기본 효과 값을 갱신한다.
    CalculateSkillPassive(CloneTileMap.mTileActorDatas[Context.mSourceActorID], EffectPacket);

    // 모션 레이어
    for (TPair<FGameplayTag, float> Effect : EffectPacket.mEffectValue)
    {
        UE_LOG(LogTemp, Warning, TEXT("모션 재생 중"));

        // 모션 패시브를 돌려서 효과 값 갱신
        CalculateMotionPassive(CloneTileMap.mTileActorDatas[Context.mSourceActorID], Effect);

        // 기본 효과 적용 커맨드 로그
        FTileLog SkillDefaultCommitCommandLog;
        SkillDefaultCommitCommandLog.mEventTimig = EEffectEventTiming::EffectDefaultCommit;
        // 타겟 스코프

        // 타겟 레이어
        for (const FTileIndex TargetTile : Context.mTargetTiles)
        {
            int32 TileIndex = TargetTile.mY * FMath::Sqrt(TileSize) + TargetTile.mX;
            UE_LOG(LogTemp, Warning, TEXT("타일 : %d"), TileIndex);


            // 타겟 필터 돌리기

            // 타격, 피격 패시브를 돌려서 효과 값 갱신
            CalculateAttackPassive(CloneTileMap.mTileActorDatas[Context.mSourceActorID], TileIndex, Effect);
            CalculateHitPassive(CloneTileMap.mTileActorDatas[Context.mSourceActorID], TileIndex, Effect);

            // 스냅샷을 기반으로 타일 로그 생성
            FEventLog EventLog;

            // 이벤트가 존재할 때만 기록한다.
            if (CreateEventLog(CloneTileMap, TileIndex, Effect, EventLog))
            {
                SkillDefaultCommitCommandLog.mEventLog.Add(TileIndex, EventLog);

                // 현재 이벤트를 기반으로 스냅샷 변경
                // 즉시 반영하여 다음 타겟이 적절한 스냅샷을 가지고 연산할 수 있게 변경한다.
                ChangeTileMapCloneDataFromEvent(CloneTileMap, EventLog);

                // 공격 후 패시브 지연을 위한 공격 후 패시브 버퍼
                // 피격 후 패시브 지연을 위한 피격 후 패시브 버퍼
            }

        }

        // 커맨드 로그 삽입
        Out_Result.mTileLog.Push(SkillDefaultCommitCommandLog);

        // 공격 후 각각의 패시브 발동, 커맨드 로그 출력
        // 피격 후 각각의 패시브 발동, 커맨드 로그 출력

        // 모션 후 패시브 발동, 커맨드 로그 출력
    }

    // 스킬 사용 후 패시브 발동, 커맨드 로그 출력

    return true;
}

FEffectPacket UCommandLogFunctionLibrary::CalculateDefaultSkillEffectValue(const FTileActorCloneData& Caster, const UStaticSkillData* SkillData)
{
    checkf(IsValid(SkillData), TEXT("스킬이 존재하지 않음"));

    FEffectPacket Packet;

    for (const FSkillAnimLayer& AnimLayer : SkillData->mSkillAnimLayers)
    {
        //for (const FSkillEffectLayer& EffectLayer : AnimLayer.mSkillEffectLayers)
        //{
        //    float Value = EffectLayer.mGameplayEffectDefaultValue + EffectLayer.mGameplayEffectRatioValue * Caster.mDiceDots;
        //    TPair TagValuePair = TPair<FGameplayTag, float>(EffectTags::GameplayEffect_Skill_Effect_Damage, Value);
        //    Packet.mEffectValue.Add(TagValuePair);
        //}
    }

    return Packet;
}

void UCommandLogFunctionLibrary::CalculateSkillPassive(const FTileActorCloneData& Caster, FEffectPacket& EffectPacket)
{
    // 패시브 필요
}

void UCommandLogFunctionLibrary::CalculateMotionPassive(const FTileActorCloneData& Caster, TPair<FGameplayTag, float>& Effect)
{
    // 패시브 필요
}

void UCommandLogFunctionLibrary::CalculateAttackPassive(const FTileActorCloneData& Caster, int32 TargetTileIndex, TPair<FGameplayTag, float>& Effect)
{
    // 패시브 필요
}

void UCommandLogFunctionLibrary::CalculateHitPassive(const FTileActorCloneData& Caster, int32 TargetTileIndex, TPair<FGameplayTag, float>& Effect)
{
    // 패시브 필요
}

bool UCommandLogFunctionLibrary::CreateEventLog(FTileMapCloneData& TileMapCloneData, int32 TargetTileIndex, TPair<FGameplayTag, float>& Effect, FEventLog& EventLog)
{
    const FTileCloneData& TileCloneData = TileMapCloneData.mTiles[TargetTileIndex];

    bool IsEvent = false;

    for (int32 ActorID : TileCloneData.mActorIDs)
    {
        checkf(ActorID != 0, TEXT("존재하지 않는 대상의 객체가 있음"));
        //UE_LOG(LogTemp, Warning, TEXT("타일에게 효과 적용 중 : %d"), TargetTileIndex);
        //UE_LOG(LogTemp, Warning, TEXT("ActorID : %d"), ActorID);

        IsEvent = true;

        const FTileActorCloneData& ActorCloneData = TileMapCloneData.mTileActorDatas[ActorID];

        // 대충 오버레이
        if (ActorCloneData.mActorType == 0)
        {
            // 장판
            // 현재 스냅샷에 장판이 있는지 확인
            // 효과가 장판을 건드리는 것이라면 이벤트 로그를 채운다.
        }
        // 유닛
        else if (ActorCloneData.mActorType == 1)
        {
            // 유닛
            // 현재 스냅샷에 유닛이 있는지 확인
            // 유닛이 있다면 유닛 이벤트 로그를 채운다.
            EventLog.mUnitEventLog.mTargetUnitID = ActorID;       // TargetTile -> 유닛 ID 필요
            EventLog.mUnitEventLog.mGameplayTag = Effect.Key;
            EventLog.mUnitEventLog.mValue = Effect.Value;
        }
        // 장애물
        else if (ActorCloneData.mActorType == 2)
        {
            // 장애물
            // 현재 스냅샷에 장애물이 있는지 확인
            // 효과가 장애물을 건드리는 것이라면 이벤트 로그를 채운다.
        }
    }


    return IsEvent;
}

void UCommandLogFunctionLibrary::ChangeTileMapCloneDataFromEvent(FTileMapCloneData& TileMapCloneData, const FEventLog& EventLog)
{
    // 유닛 이벤트가 존재할 때만
    if (EventLog.mUnitEventLog.IsValid())
    {
        if (EventLog.mUnitEventLog.mGameplayTag == EffectTags::GameplayEffect_Skill_Effect_Damage)
        {
            TileMapCloneData.mTileActorDatas.Find(EventLog.mUnitEventLog.mTargetUnitID)->HP -= EventLog.mUnitEventLog.mValue;
        }
    }
}

