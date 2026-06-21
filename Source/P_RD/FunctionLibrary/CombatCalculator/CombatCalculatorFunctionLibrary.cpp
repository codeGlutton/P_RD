// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatCalculatorFunctionLibrary.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"

bool UCombatCalculatorFunctionLibrary::CalculateSkillResult(UBoardActorModel* CasterTile, const UStaticSkillData* SkillData, const TArray<TObjectPtr<UBoardActorModel>>& Tiles, FSkillCommitResult& Out_Result)
{
    FSkillCommitResult SkillCommitResult;

    // 1. 이펙트 적용 결과 생성
    FEffectCommitResult EffectCommitResult;

    for (UBoardActorModel* TileActor : Tiles)
    {
        // 2. 타일 적용 결과 생성 (예: X=1, Y=2 타일)
        FTileActorCommitResult TileCommitResult;

        // 타일 인덱스 설정 (프로젝트 내 FTileIndex 구조체 형태에 맞게 사용)
        TileCommitResult.mTileActor = TileActor;

        // 유닛이 있다면
        if (true)
        {
            // 3. 유닛 적용 결과 생성 (데미지 적용)
            FUnitCommitResult UnitCommitResult;

            // GameplayTag는 프로젝트에서 사용하는 실제 데미지 태그로 교체 필요
            FGameplayTag DamageTag = EffectTags::GameplayEffect_Skill_Effect_Damage;
            UnitCommitResult.mEffect.Add(DamageTag, 50.0f); // 데미지 수치 50 적용

            TileCommitResult.mUnitCommitResult = UnitCommitResult;
        }
        EffectCommitResult.mTileCommitResult.Add(TileCommitResult);
    }

    // 4. 계층 구조에 데이터 조립
    SkillCommitResult.mEffectCommitResult.Add(EffectCommitResult);

    Out_Result = SkillCommitResult;

    return true;
}
