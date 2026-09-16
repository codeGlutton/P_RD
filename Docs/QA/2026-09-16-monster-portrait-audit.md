# 몬스터 초상화 점검 — 2026-09-16

기준: develop 08058e20 + 진행 중 UI 수정, Content SVN r446. 추가 초상화 자산은 SVN r447.

## 결과

27종의 mIcon / mPortrait 54개 연결을 새 Unreal 프로세스에서 로드해 모두 Texture2D와 유효한 크기를 확인했다.
해골새(독수리 재사용), 붉은 거미(일반 거미 재사용), 폭발 슬라임(일반 슬라임 재사용)의 전용 투명 PNG를 built-in image_gen으로 생성하고 두 UI 필드에 연결했다.
일반 거미는 기존 전용 초상화가 있으며 이번에는 변경하지 않았다.
감사 스크립트의 변경 전후 비교에서 3개 DA의 mIcon/mPortrait 6개 필드만 변경됐다. 모델, 스킬, 능력치와 컷인 필드는 변경하지 않았다.

## 자산 및 검증 범위

- 텍스처: Content/SVN/OutSideAsset/AICreation/UI/Portraits/Variants_20260916
- 원본 및 정확한 생성 프롬프트: SourceArt/UI/MonsterPortraits_20260916 (SVN SourceArt/UI에 보관)
- 로컬 원본 복사: F:/코덱스이미지생성폴더/20260916-monster-portrait-*.png
- UI 설정: 최대 512px, UI 압축, sRGB, mipmap 없음. 생성 원본 RGBA 알파 0–255 보존.
- 독립 프로세스 검증: D:/Builds/P_RD/MonsterPortraits_20260916/verified-after.json
- 변경 비교: D:/Builds/P_RD/MonsterPortraits_20260916/binding-diff.json
- 이번 작업에서 Android 패키징, 폰 설치, Play 배포는 수행하지 않았다.

## 별도 발견

해골새와 붉은 거미의 mShortCut도 각각 독수리·일반 거미 컷인을 재사용한다. 이번 요청의 초상화 범위와 분리했으며, 정사각 초상화를 가로 컷인에 억지로 늘려 넣지 않았다.

## 전체 목록

| 몬스터 DA | 아이콘 텍스처 | 초상화 텍스처 |
|---|---|---|
| DA_EagleUnit | KK_Face_Enemy_Eagle_HeadV2 | KK_Face_Enemy_Eagle_ActionV3 |
| DA_GolemUnit | KK_Face_Enemy_Golem_HeadV2 | KK_Face_Enemy_Golem_ActionV3 |
| DA_LeshyUnit | KK_Face_Enemy_Leshy_HeadV2 | KK_Face_Enemy_Leshy_ActionV3 |
| DA_MushroomUnit | KK_Face_Enemy_Mushroom_HeadV2 | KK_Face_Enemy_Mushroom_ActionV3 |
| DA_SlimeUnit | KK_Face_Enemy_Slime_HeadV2 | KK_Face_Enemy_Slime_HeadV2 |
| DA_SpiderUnit | KK_Face_Enemy_Spider_HeadV2 | KK_Face_Enemy_Spider_ActionV3 |
| DA_StumpUnit | T_Portrait_Stump | T_Portrait_Stump |
| DA_WerewolfUnit | KK_Face_Enemy_Werewolf_HeadV2 | KK_Face_Enemy_Werewolf_ActionV3 |
| DA_BatUnit | T_Portrait_Bat | T_Portrait_Bat |
| DA_HatchetBirdUnit | T_Portrait_HatchetBird | T_Portrait_HatchetBird |
| DA_MimicUnit | T_Portrait_Mimic | T_Portrait_Mimic |
| DA_PumpkinUnit | T_Portrait_Pumpkin | T_Portrait_Pumpkin |
| DA_RatUnit | T_Rat_Head | T_Rat_Action |
| DA_Red_SpiderUnit | T_Portrait_RedSpider_v1 | T_Portrait_RedSpider_v1 |
| DA_SkeletonBirdUnit | T_Portrait_SkeletonBird_v1 | T_Portrait_SkeletonBird_v1 |
| DA_SkeletonGolem | KK_Face_Enemy_SkeletonGolem_HeadV2 | KK_Face_Enemy_SkeletonGolem_ActionV3 |
| DA_SkeletonMeleeUnit | KK_Face_Enemy_SkeletonMinionMelee_HeadV2 | KK_Face_Enemy_SkeletonMinionMelee_ActionV3 |
| DA_SkeletonRangedUnit | KK_Face_Enemy_SkeletonMinionRanged_HeadV2 | KK_Face_Enemy_SkeletonMinionRanged_ActionV3 |
| DA_Slime_ExplosionUnit | T_Portrait_SlimeExplosion_v1 | T_Portrait_SlimeExplosion_v1 |
| DA_DemonUnit | T_Portrait_Demon | T_Portrait_Demon |
| DA_GhoulUnit | T_Portrait_Ghoul | T_Portrait_Ghoul |
| DA_NecromancerUnit | KK_Face_Enemy_Necromancer_HeadV2 | KK_Face_Enemy_Necromancer_ActionV3 |
| DA_ReaperUnit | T_Portrait_Reaper | T_Portrait_Reaper |
| DA_Skeleton_MageUnit | T_Portrait_Skeleton_Mage | T_Portrait_Skeleton_Mage |
| DA_Skeleton_RogueUnit | T_Portrait_Skeleton_Rogue | T_Portrait_Skeleton_Rogue |
| DA_Skeleton_WarriorUnit | T_Portrait_Skeleton_Warrior | T_Portrait_Skeleton_Warrior |
| DA_ZombieUnit | T_Portrait_Zombie | T_Portrait_Zombie |
