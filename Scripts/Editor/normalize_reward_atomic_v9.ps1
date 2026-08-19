param(
    [string]$GeneratedRoot = 'C:\Users\2009e\.codex\generated_images\01a00649-56ee-7693-b057-d255eb583395',
    [string]$OutputRoot = 'D:\UnrealProjects\P_RD_develop_20260816\Saved\DesignAssets\RewardAtomicV9_20260817\GeneratedPartsV1',
    [string]$ArchiveRoot = 'F:\코덱스이미지생성폴더'
)

$ErrorActionPreference = 'Stop'
$Magick = 'C:\Program Files\ImageMagick-7.1.2-Q16-HDRI\magick.exe'
if (-not (Test-Path -LiteralPath $Magick)) { throw "ImageMagick not found: $Magick" }
New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
New-Item -ItemType Directory -Force -Path $ArchiveRoot | Out-Null

function Get-UniqueArchivePath([string]$BaseName) {
    $Candidate = Join-Path $ArchiveRoot $BaseName
    if (-not (Test-Path -LiteralPath $Candidate)) { return $Candidate }
    $Stem = [System.IO.Path]::GetFileNameWithoutExtension($BaseName)
    $Extension = [System.IO.Path]::GetExtension($BaseName)
    for ($Index = 2; ; ++$Index) {
        $Candidate = Join-Path $ArchiveRoot ("{0}_v{1}{2}" -f $Stem, $Index, $Extension)
        if (-not (Test-Path -LiteralPath $Candidate)) { return $Candidate }
    }
}

$Assets = @(
    @{ Raw='exec-f9477e37-73db-414f-80ad-f971666370af.png'; Name='reward_atomic_modal_background_1536x864_v1.png'; Size='1536x864'; Mode='Opaque' },
    @{ Raw='exec-851a493f-9afd-409e-a436-3249e412f86f.png'; Name='reward_atomic_modal_outer_frame_1536x864_v1.png'; Size='1536x864'; Mode='Alpha' },
    @{ Raw='exec-ce74a031-ba40-47a9-ad9b-3b5a6acb6ac3.png'; Name='reward_atomic_body_background_1376x518_v1.png'; Size='1376x518'; Mode='Opaque' },
    @{ Raw='exec-0ec19a94-1b9f-4f1b-8f60-5b05efc0475e.png'; Name='reward_atomic_body_frame_1376x518_v1.png'; Size='1376x518'; Mode='Alpha' },
    @{ Raw='exec-6230fa1f-3182-4ab8-9018-99fb3a5c7e3b.png'; Name='reward_atomic_header_background_720x141_v1.png'; Size='720x141'; Mode='Crop'; Geometry='1716x500+0+208' },
    @{ Raw='exec-7c3983d7-87b7-4afa-8f45-167eae00b343.png'; Name='reward_atomic_header_frame_720x141_v1.png'; Size='720x141'; Mode='Alpha' },
    @{ Raw='exec-52230949-95d5-49b5-8c70-e58d27f27629.png'; Name='reward_atomic_step_background_380x62_v1.png'; Size='380x62'; Mode='DarkKey' },
    @{ Raw='exec-37538995-2259-4a5f-afc4-2f2b03e66f1a.png'; Name='reward_atomic_step_frame_380x62_v1.png'; Size='380x62'; Mode='Alpha' },
    @{ Raw='exec-a8c22d34-9e03-4bc5-995a-0c86378fc584.png'; Name='reward_atomic_cta_background_404x70_v1.png'; Size='404x70'; Mode='DarkKey' },
    @{ Raw='exec-c221f011-cf19-4f06-8d90-be8c578d7503.png'; Name='reward_atomic_cta_frame_404x70_v1.png'; Size='404x70'; Mode='Alpha' },
    @{ Raw='exec-7694ef58-b37f-4904-9b27-abc8fa11ae49.png'; Name='reward_atomic_exp_rail_background_1054x132_v1.png'; Size='1054x132'; Mode='Opaque' },
    @{ Raw='exec-51f07e8f-076e-4e86-b42a-cab4311eedba.png'; Name='reward_atomic_exp_rail_frame_1054x132_v1.png'; Size='1054x132'; Mode='Alpha' },
    @{ Raw='exec-fe9f0b8d-686a-4c57-8bd2-8c007e513d3f.png'; Name='reward_atomic_exp_portrait_socket_190x132_v1.png'; Size='190x132'; Mode='Alpha' },
    @{ Raw='exec-90300eba-f5f4-4ce2-9e49-b8493df81289.png'; Name='reward_atomic_exp_level_plate_100x76_v1.png'; Size='100x76'; Mode='Alpha' },
    @{ Raw='exec-d8780515-1dcb-497d-b93e-99a72e31313d.png'; Name='reward_atomic_exp_progress_track_520x60_v1.png'; Size='520x60'; Mode='CheckerKey' },
    @{ Raw='exec-c1a4e058-e2d4-4bc6-955e-dbb4b6517e76.png'; Name='reward_atomic_exp_xp_badge_130x92_v1.png'; Size='130x92'; Mode='Alpha' },
    @{ Raw='exec-d5668f65-bab5-4cdc-aeba-e892d65baea8.png'; Name='reward_atomic_gold_rail_background_800x228_v1.png'; Size='800x228'; Mode='Opaque' },
    @{ Raw='exec-1695c7db-2431-4245-aef0-9e9a3114d6e1.png'; Name='reward_atomic_gold_rail_frame_800x228_v1.png'; Size='800x228'; Mode='Alpha' },
    @{ Raw='exec-21479b85-9199-48f5-9957-9022f7f6a24b.png'; Name='reward_atomic_gold_coin_socket_180x180_v1.png'; Size='180x180'; Mode='Alpha' },
    @{ Raw='exec-c75e1f4b-b355-4dc3-9864-04d12f2b6727.png'; Name='reward_atomic_gold_label_plate_300x150_v1.png'; Size='300x150'; Mode='Alpha' },
    @{ Raw='exec-509f6c2e-a9bc-41dc-8fba-54f9c056709c.png'; Name='reward_atomic_gold_amount_plate_280x150_v1.png'; Size='280x150'; Mode='Alpha' },
    @{ Raw='exec-63cf5dc1-6306-4f84-baf6-80899bbd2e09.png'; Name='reward_atomic_card_background_240x389_v1.png'; Size='240x389'; Mode='Crop'; Geometry='760x1350+112+123' },
    @{ Raw='exec-45578563-93ec-49c2-9da9-89803b3c43f7.png'; Name='reward_atomic_card_outer_frame_240x389_v1.png'; Size='240x389'; Mode='Alpha' },
    @{ Raw='exec-079078a4-a937-4154-8036-4c794ab58dd8.png'; Name='reward_atomic_card_art_socket_180x180_v1.png'; Size='180x180'; Mode='Alpha' },
    @{ Raw='exec-6c9b1a68-8887-4b81-a76e-a13d85f4cead.png'; Name='reward_atomic_card_info_panel_180x54_v1.png'; Size='180x54'; Mode='Alpha' },
    @{ Raw='exec-44dc2239-16e6-4ee0-b627-0a515c0b0ea1.png'; Name='reward_atomic_card_name_plate_196x58_v1.png'; Size='196x58'; Mode='Alpha' },
    @{ Raw='exec-4b3ef83c-7a3a-4eb6-adc2-18fd0fec0dd3.png'; Name='reward_atomic_card_selection_overlay_240x389_v1.png'; Size='240x389'; Mode='Alpha' }
)

foreach ($Asset in $Assets) {
    $Source = Join-Path $GeneratedRoot $Asset.Raw
    $Target = Join-Path $OutputRoot $Asset.Name
    if (-not (Test-Path -LiteralPath $Source)) { throw "Missing generated source: $Source" }

    switch ($Asset.Mode) {
        'Opaque' {
            & $Magick $Source -filter Lanczos -resize "$($Asset.Size)!" -strip $Target
        }
        'Alpha' {
            & $Magick $Source -alpha on -trim +repage -filter Lanczos -resize "$($Asset.Size)!" -strip $Target
        }
        'Crop' {
            & $Magick $Source -crop $Asset.Geometry +repage -filter Lanczos -resize "$($Asset.Size)!" -strip $Target
        }
        'DarkKey' {
            & $Magick $Source -alpha on -channel A -fx '((r<0.055)&&(g<0.055)&&(b<0.055))?0:a' +channel -trim +repage -filter Lanczos -resize "$($Asset.Size)!" -channel A -threshold 3% +channel -strip $Target
        }
        'CheckerKey' {
            & $Magick $Source -alpha on -channel A -fx '((r>0.80)&&(g>0.80)&&(b>0.80)&&(abs(r-g)<0.045)&&(abs(g-b)<0.045))?0:a' +channel -trim +repage -filter Lanczos -resize "$($Asset.Size)!" -channel A -threshold 3% +channel -strip $Target
        }
        default { throw "Unknown mode: $($Asset.Mode)" }
    }

    $ArchiveName = "P_RD_reward_atomic_v9_$($Asset.Name)"
    Copy-Item -LiteralPath $Target -Destination (Get-UniqueArchivePath $ArchiveName)
}

$ContactSheet = Join-Path $OutputRoot 'reward_atomic_v9_contact_sheet.png'
$Labels = foreach ($Asset in $Assets) { Join-Path $OutputRoot $Asset.Name }
& $Magick montage @Labels -thumbnail '300x160>' -background '#17130f' -fill white -stroke black -strokewidth 1 -label '%f' -tile '3x' -geometry '320x205+10+10' $ContactSheet
Copy-Item -LiteralPath $ContactSheet -Destination (
    Get-UniqueArchivePath 'P_RD_reward_atomic_v9_contact_sheet_20260817.png')

$Manifest = Join-Path $OutputRoot 'atomic-assets.tsv'
"name`twidth`theight`tchannels`opaque" | Set-Content -LiteralPath $Manifest -Encoding UTF8
foreach ($Asset in $Assets) {
    $Target = Join-Path $OutputRoot $Asset.Name
    $Line = & $Magick identify -format "%f`t%w`t%h`t%[channels]`t%[opaque]" $Target
    Add-Content -LiteralPath $Manifest -Value $Line -Encoding UTF8
}

Write-Host "Normalized $($Assets.Count) atomic assets into $OutputRoot"
Write-Host "Contact sheet: $ContactSheet"
