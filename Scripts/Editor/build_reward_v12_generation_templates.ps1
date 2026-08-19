param(
    [string]$ProjectRoot = 'D:\UnrealProjects\P_RD_develop_20260816'
)

$ErrorActionPreference = 'Stop'
$Magick = 'C:\Program Files\ImageMagick-7.1.2-Q16-HDRI\magick.exe'
$Out = Join-Path $ProjectRoot 'Saved\DesignAssets\RewardAtomicV12\GenerationTemplates'
New-Item -ItemType Directory -Force -Path $Out | Out-Null

function Invoke-Magick([string[]]$Arguments) {
    & $Magick @Arguments
    if ($LASTEXITCODE -ne 0) { throw "ImageMagick failed: $($Arguments -join ' ')" }
}

function New-RoundedTemplate([string]$Name, [int]$W, [int]$H, [int]$Margin, [int]$Radius) {
    Invoke-Magick @('-size', "${W}x${H}", 'xc:none', '-fill', '#808080',
        '-draw', "roundrectangle $Margin,$Margin $($W-$Margin-1),$($H-$Margin-1) $Radius,$Radius",
        (Join-Path $Out $Name))
}

New-RoundedTemplate 'template_exp_row_plate_1054x132.png' 1054 132 2 18
New-RoundedTemplate 'template_exp_level_window_96x58.png' 96 58 2 12
New-RoundedTemplate 'template_exp_progress_track_516x22.png' 516 22 1 10
New-RoundedTemplate 'template_gold_panel_plate_800x228.png' 800 228 2 24
New-RoundedTemplate 'template_gold_amount_window_300x160.png' 300 160 2 20
New-RoundedTemplate 'template_card_name_plate_240x70.png' 240 70 2 14

Invoke-Magick @('-size', '116x116', 'xc:none', '-fill', '#808080', '-draw', 'circle 58,58 56,58',
    (Join-Path $Out 'template_exp_portrait_ring_116x116.png'))
Invoke-Magick @('-size', '168x168', 'xc:none', '-fill', '#808080', '-draw', 'circle 84,84 82,84',
    (Join-Path $Out 'template_gold_coin_ring_168x168.png'))

Invoke-Magick @('-size', '120x64', 'xc:none', '-fill', '#808080',
    '-draw', "path 'M 12,4 L 108,4 L 116,14 L 108,50 L 60,62 L 12,50 L 4,14 Z'",
    (Join-Path $Out 'template_exp_xp_badge_120x64.png'))

Invoke-Magick @('-size', '240x230', 'xc:none', '-stroke', '#808080', '-strokewidth', '14', '-fill', 'none',
    '-draw', 'roundrectangle 8,8 231,221 18,18',
    (Join-Path $Out 'template_card_frame_240x230.png'))

Invoke-Magick @('-size', '240x300', 'xc:none', '-stroke', '#808080', '-strokewidth', '8', '-fill', 'none',
    '-draw', 'roundrectangle 8,8 231,291 22,22',
    (Join-Path $Out 'template_card_selected_overlay_240x300.png'))

Write-Host "Reward V12 generation templates: $Out"
