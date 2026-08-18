# Import-MdiIcon.ps1
#
# Vendors one Material Design Icon into Resources/Icons/Mdi/ at the PINNED version:
# validates the name against the pinned index, downloads from the pinned tag, applies the
# white-fill recolour (see Mdi/NOTICE.md), and writes the SVG. It does NOT touch the manifest —
# add the semantic mapping to Resources/Icons/CkIcons_Manifest.json yourself, then run
# Generate-CkIcons.ps1.
#
# Usage:
#   pwsh ./Import-MdiIcon.ps1 -Name cube-outline

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$Name
)

$ErrorActionPreference = 'Stop'

$PluginRoot  = (Resolve-Path (Join-Path $PSScriptRoot '..' '..')).Path
$MdiDir      = Join-Path $PluginRoot 'Resources/Icons/Mdi'
$VersionPath = Join-Path $MdiDir 'VERSION.json'
$NamesPath   = Join-Path $MdiDir 'mdi-names.txt'

$Version = Get-Content $VersionPath -Raw | ConvertFrom-Json
$Tag = $Version.tag
if ([string]::IsNullOrWhiteSpace($Tag)) { throw "No pinned tag in $VersionPath" }

$KnownMdiNames = [System.Collections.Generic.HashSet[string]]::new([string[]](Get-Content $NamesPath))
if (-not $KnownMdiNames.Contains($Name))
{ throw "Unknown MDI icon name '$Name' — not present in the pinned $Tag index ($NamesPath). Check spelling against https://pictogrammers.com/library/mdi/" }

$Url = "https://raw.githubusercontent.com/Templarian/MaterialDesign-SVG/$Tag/svg/$Name.svg"
$Svg = (Invoke-WebRequest -Uri $Url -UseBasicParsing).Content
if ($Svg -is [byte[]]) { $Svg = [System.Text.Encoding]::UTF8.GetString($Svg) }

if ($Svg -notmatch '<path ') { throw "Downloaded content for '$Name' does not look like an MDI SVG (no <path>): $Url" }

# The recolour recorded in Mdi/NOTICE.md — bare paths default to black, which cannot be tinted.
$Recoloured = $Svg -creplace '<path ', '<path fill="#FFFFFF" '

foreach ($Forbidden in @('<style', '<text', '<filter', 'class='))
{
    if ($Recoloured.Contains($Forbidden))
    { throw "Downloaded SVG contains '$Forbidden', which UE's SVG rasterizer does not support: $Url" }
}

$OutPath = Join-Path $MdiDir "$Name.svg"
[System.IO.File]::WriteAllText($OutPath, $Recoloured, [System.Text.UTF8Encoding]::new($false))

Write-Host "Vendored $Name ($Tag) -> $OutPath"
Write-Host "Next: add a semantic mapping to Resources/Icons/CkIcons_Manifest.json and run Generate-CkIcons.ps1"
