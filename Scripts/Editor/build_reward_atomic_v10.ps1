param(
    [string]$ProjectRoot = 'D:\UnrealProjects\P_RD_develop_20260816',
    [string]$GeneratedModalBackground = 'C:\Users\2009e\.codex\generated_images\01a00649-56ee-7693-b057-d255eb583395\exec-e5e9207e-7daa-4e3b-9f40-5ac62bf53276.png',
    [string]$ArchiveRoot = 'F:\코덱스이미지생성폴더'
)

$ErrorActionPreference = 'Stop'
$Magick = 'C:\Program Files\ImageMagick-7.1.2-Q16-HDRI\magick.exe'
$V8Parts = Join-Path $ProjectRoot 'Saved\DesignAssets\RewardWireframeV8_20260816\GeneratedPartsV2'
$V8Outer = Join-Path $ProjectRoot 'Saved\DesignAssets\RewardWireframeV8_20260816\GeneratedPartsV1\reward_v8_outer_shell_no_bottom_crystal_1536x864.png'
$OutputRoot = Join-Path $ProjectRoot 'Saved\DesignAssets\RewardAtomicV10_20260817\GeneratedPartsV1'
New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
New-Item -ItemType Directory -Force -Path $ArchiveRoot | Out-Null

function Invoke-Magick([string[]]$Arguments) {
    & $Magick @Arguments
    if ($LASTEXITCODE -ne 0) { throw "ImageMagick failed: $($Arguments -join ' ')" }
}

function New-FillBackground([string]$Source, [string]$Name, [string]$Crop, [string]$Size) {
    $Target = Join-Path $OutputRoot $Name
    Invoke-Magick @($Source, '-crop', $Crop, '+repage', '-filter', 'Lanczos', '-resize', "$Size!", '-strip', $Target)
}

function New-SplitLayer([string]$Source, [string]$Name, [int]$Width, [int]$Height,
    [string]$InnerRect, [int]$Radius, [bool]$KeepInside) {
    $Mask = Join-Path $OutputRoot ("mask_" + $Name)
    $Target = Join-Path $OutputRoot $Name
    if ($KeepInside) {
        Invoke-Magick @('-size', "${Width}x${Height}", 'xc:black', '-fill', 'white',
            '-draw', "roundrectangle $InnerRect $Radius,$Radius", $Mask)
    } else {
        Invoke-Magick @('-size', "${Width}x${Height}", 'xc:white', '-fill', 'black',
            '-draw', "roundrectangle $InnerRect $Radius,$Radius", $Mask)
    }
    # DstIn multiplies the source's existing alpha by the mask. CopyOpacity/alpha-off
    # replaced the original alpha and produced the opaque black rectangles seen in UE.
    Invoke-Magick @($Source, $Mask, '-compose', 'DstIn', '-composite', '-strip', $Target)
    Remove-Item -LiteralPath $Mask
}

function New-Crop([string]$Source, [string]$Name, [string]$Crop) {
    Invoke-Magick @($Source, '-crop', $Crop, '+repage', '-strip', (Join-Path $OutputRoot $Name))
}

function Get-UniqueArchivePath([string]$BaseName) {
    $Candidate = Join-Path $ArchiveRoot $BaseName
    if (-not (Test-Path -LiteralPath $Candidate)) { return $Candidate }
    $Stem = [IO.Path]::GetFileNameWithoutExtension($BaseName)
    $Ext = [IO.Path]::GetExtension($BaseName)
    for ($Index = 2; ; ++$Index) {
        $Candidate = Join-Path $ArchiveRoot ("{0}_v{1}{2}" -f $Stem, $Index, $Ext)
        if (-not (Test-Path -LiteralPath $Candidate)) { return $Candidate }
    }
}

$Header = Join-Path $V8Parts 'reward_v8_header_720x141_v2.png'
$Body = Join-Path $V8Parts 'reward_v8_content_panel_1376x518_v2_clean_edgefixed.png'
$Step = Join-Path $V8Parts 'reward_v8_step_badge_380x62_v2.png'
$Cta = Join-Path $V8Parts 'reward_v8_primary_button_404x70_v2.png'
$Exp = Join-Path $V8Parts 'reward_v8_exp_row_1054x132_v2_clean_edgefixed.png'
$Gold = Join-Path $V8Parts 'reward_v8_gold_panel_800x228_v2_clean_edgefixed.png'
$Card = Join-Path $V8Parts 'reward_v8_artifact_card_240x389_v2.png'

Invoke-Magick @($GeneratedModalBackground, '-filter', 'Lanczos', '-resize', '1536x864!',
    '-strip', (Join-Path $OutputRoot 'reward_v10_modal_background_1536x864.png'))
New-SplitLayer $V8Outer 'reward_v10_modal_outer_frame_1536x864.png' 1536 864 '70,70 1466,794' 32 $false

New-SplitLayer $Body 'reward_v10_body_background_1376x518.png' 1376 518 '68,46 1308,472' 42 $true
New-SplitLayer $Body 'reward_v10_body_frame_1376x518.png' 1376 518 '68,46 1308,472' 42 $false
New-SplitLayer $Header 'reward_v10_header_background_720x141.png' 720 141 '48,25 672,116' 24 $true
New-SplitLayer $Header 'reward_v10_header_frame_720x141.png' 720 141 '48,25 672,116' 24 $false
New-SplitLayer $Step 'reward_v10_step_background_380x62.png' 380 62 '30,10 350,52' 14 $true
New-SplitLayer $Step 'reward_v10_step_frame_380x62.png' 380 62 '30,10 350,52' 14 $false
New-SplitLayer $Cta 'reward_v10_cta_background_404x70.png' 404 70 '35,12 369,58' 14 $true
New-SplitLayer $Cta 'reward_v10_cta_frame_404x70.png' 404 70 '35,12 369,58' 14 $false

# Non-overlapping semantic strips: recombination is pixel-identical to the V8 master.
New-Crop $Exp 'reward_v10_exp_portrait_segment_235x132.png' '235x132+0+0'
New-Crop $Exp 'reward_v10_exp_level_segment_110x132.png' '110x132+235+0'
New-Crop $Exp 'reward_v10_exp_progress_segment_525x132.png' '525x132+345+0'
New-Crop $Exp 'reward_v10_exp_xp_segment_140x132.png' '140x132+870+0'
New-Crop $Exp 'reward_v10_exp_tail_segment_44x132.png' '44x132+1010+0'

New-Crop $Gold 'reward_v10_gold_coin_segment_220x228.png' '220x228+0+0'
New-Crop $Gold 'reward_v10_gold_label_segment_280x228.png' '280x228+220+0'
New-Crop $Gold 'reward_v10_gold_amount_segment_300x228.png' '300x228+500+0'

New-FillBackground $Card 'reward_v10_card_background_240x389.png' '150x86+45+205' '240x389'
New-Crop $Card 'reward_v10_card_art_segment_240x230.png' '240x230+0+0'
New-Crop $Card 'reward_v10_card_info_segment_240x70.png' '240x70+0+230'
New-Crop $Card 'reward_v10_card_name_segment_240x70.png' '240x70+0+300'
New-Crop $Card 'reward_v10_card_footer_segment_240x19.png' '240x19+0+370'

$ExpPieces = @(
    'reward_v10_exp_portrait_segment_235x132.png',
    'reward_v10_exp_level_segment_110x132.png',
    'reward_v10_exp_progress_segment_525x132.png',
    'reward_v10_exp_xp_segment_140x132.png',
    'reward_v10_exp_tail_segment_44x132.png') | ForEach-Object { Join-Path $OutputRoot $_ }
$GoldPieces = @(
    'reward_v10_gold_coin_segment_220x228.png',
    'reward_v10_gold_label_segment_280x228.png',
    'reward_v10_gold_amount_segment_300x228.png') | ForEach-Object { Join-Path $OutputRoot $_ }
$CardPieces = @(
    'reward_v10_card_art_segment_240x230.png',
    'reward_v10_card_info_segment_240x70.png',
    'reward_v10_card_name_segment_240x70.png',
    'reward_v10_card_footer_segment_240x19.png') | ForEach-Object { Join-Path $OutputRoot $_ }

$ExpPreview = Join-Path $OutputRoot 'reward_v10_recombined_exp_preview.png'
$GoldPreview = Join-Path $OutputRoot 'reward_v10_recombined_gold_preview.png'
$CardPreview = Join-Path $OutputRoot 'reward_v10_recombined_card_preview.png'
Invoke-Magick (@($ExpPieces) + @('+append', $ExpPreview))
Invoke-Magick (@($GoldPieces) + @('+append', $GoldPreview))
Invoke-Magick (@($CardPieces) + @('-append', $CardPreview))

foreach ($Comparison in @(
    @{ Master=$Exp; Preview=$ExpPreview; Name='EXP' },
    @{ Master=$Gold; Preview=$GoldPreview; Name='GOLD' },
    @{ Master=$Card; Preview=$CardPreview; Name='CARD' })) {
    $MetricOutput = & $Magick compare -metric AE $Comparison.Master $Comparison.Preview null: 2>&1
    $MetricText = ($MetricOutput | Out-String).Trim()
    $Metric = [int]([regex]::Match($MetricText, '^\d+').Value)
    if ($Metric -ne 0) { throw "$($Comparison.Name) recombination mismatch: $Metric pixels" }
    Write-Host "$($Comparison.Name) recombination mismatch pixels: 0"
}

$FinalPngs = Get-ChildItem -LiteralPath $OutputRoot -Filter 'reward_v10_*.png'
$ContactSheet = Join-Path $OutputRoot 'reward_v10_atomic_contact_sheet.png'
Invoke-Magick (@('montage') + @($FinalPngs.FullName) + @('-thumbnail', '320x170>',
    '-background', '#17130f', '-tile', '3x', '-geometry', '340x200+10+10', $ContactSheet))

foreach ($Png in (Get-ChildItem -LiteralPath $OutputRoot -Filter '*.png')) {
    Copy-Item -LiteralPath $Png.FullName -Destination (
        Get-UniqueArchivePath ("P_RD_" + $Png.Name))
}

Write-Host "Built V10 from one visual master per component: $OutputRoot"
