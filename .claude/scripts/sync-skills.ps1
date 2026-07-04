<#
.SYNOPSIS
    Junction the Ck plugin skill libraries into a consuming superproject's .claude/skills/.

.DESCRIPTION
    Claude Code auto-discovers skills at the session project root (.claude/skills/<name>/SKILL.md)
    but does NOT walk into git submodules' own .claude/skills directories. Ck skills live in the
    plugin repo that owns them (CkFoundation, CkGameplayDebugger, CkTests) so they version with the
    code. This script bridges the gap: it creates NTFS directory junctions from the superproject's
    .claude/skills/<name> to each plugin's .claude/skills/<name>.

    Idempotent and safe to re-run after every submodule update:
      - correct junction already present  -> skipped
      - junction pointing elsewhere       -> re-pointed
      - REAL directory with the same name -> warned and left alone (never clobbers your content)
      - -Prune removes junctions whose targets no longer exist

    See README.md next to this script for verified Claude Code discovery behavior and caveats
    (notably: creating .claude/skills for the FIRST time requires restarting the Claude Code
    session before skills appear).

.PARAMETER SuperprojectRoot
    Root of the consuming project (the directory Claude Code sessions open). Default: current dir.

.PARAMETER PluginSkillDirs
    Explicit list of source skill directories. Default: auto-discover
    <SuperprojectRoot>/Plugins/*/.claude/skills.

.PARAMETER Prune
    Also remove junctions in <SuperprojectRoot>/.claude/skills whose target directory is gone.

.PARAMETER DryRun
    Print what would happen without changing anything.

.EXAMPLE
    pwsh Plugins/CkFoundation/.claude/scripts/sync-skills.ps1
    # from the superproject root — discovers all plugin skill dirs and junctions them in.

.EXAMPLE
    pwsh sync-skills.ps1 -SuperprojectRoot D:/Repos/BusterBlock -Prune
#>
[CmdletBinding()]
param(
    [string]   $SuperprojectRoot = (Get-Location).Path,
    [string[]] $PluginSkillDirs  = @(),
    [switch]   $Prune,
    [switch]   $DryRun
)

$ErrorActionPreference = 'Stop'

function Test-IsJunction([string] $Path)
{
    $Item = Get-Item -LiteralPath $Path -Force
    return [bool]($Item.Attributes -band [IO.FileAttributes]::ReparsePoint)
}

$SuperprojectRoot = (Resolve-Path -LiteralPath $SuperprojectRoot).Path
$SkillsRoot       = Join-Path $SuperprojectRoot '.claude/skills'

# `pwsh -File` flattens array arguments into one comma/semicolon-joined string - split them back
$PluginSkillDirs = @($PluginSkillDirs | ForEach-Object { $_ -split '[,;]' } | Where-Object { $_ })

if (-not (Test-Path -LiteralPath $SkillsRoot))
{
    if ($DryRun)
    { Write-Host "[dry-run] would create $SkillsRoot" }
    else
    {
        New-Item -ItemType Directory -Force -Path $SkillsRoot | Out-Null
        Write-Warning ".claude/skills did not exist and was just created. Claude Code only watches skill directories that existed at session start - RESTART any open session rooted at $SuperprojectRoot before expecting the skills to load."
    }
}

if ($PluginSkillDirs.Count -eq 0)
{
    $PluginSkillDirs = Get-ChildItem -Path (Join-Path $SuperprojectRoot 'Plugins') -Directory -ErrorAction SilentlyContinue |
        ForEach-Object { Join-Path $_.FullName '.claude/skills' } |
        Where-Object { Test-Path -LiteralPath $_ }
}

if ($PluginSkillDirs.Count -eq 0)
{
    Write-Warning "No plugin skill directories found under $SuperprojectRoot/Plugins/*/.claude/skills - nothing to sync."
    return
}

$Results = [System.Collections.Generic.List[object]]::new()
$Seen    = @{}

foreach ($SourceRoot in $PluginSkillDirs)
{
    $SourceRootResolved = (Resolve-Path -LiteralPath $SourceRoot).Path
    $SkillDirs = Get-ChildItem -LiteralPath $SourceRootResolved -Directory |
        Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'SKILL.md') }

    foreach ($Skill in $SkillDirs)
    {
        $Name     = $Skill.Name
        $LinkPath = Join-Path $SkillsRoot $Name

        if ($Seen.ContainsKey($Name))
        {
            $Results.Add([pscustomobject]@{ Skill = $Name; Action = 'COLLISION-SKIPPED'; Detail = "already provided by $($Seen[$Name])" })
            continue
        }
        $Seen[$Name] = $SourceRootResolved

        if (Test-Path -LiteralPath $LinkPath)
        {
            if (-not (Test-IsJunction $LinkPath))
            {
                $Results.Add([pscustomobject]@{ Skill = $Name; Action = 'REAL-DIR-SKIPPED'; Detail = 'a non-junction directory exists here; refusing to clobber' })
                continue
            }

            $CurrentTarget = (Get-Item -LiteralPath $LinkPath -Force).Target
            if ($CurrentTarget -eq $Skill.FullName)
            {
                $Results.Add([pscustomobject]@{ Skill = $Name; Action = 'ok'; Detail = 'junction already correct' })
                continue
            }

            if ($DryRun)
            { $Results.Add([pscustomobject]@{ Skill = $Name; Action = '[dry-run] re-point'; Detail = "$CurrentTarget -> $($Skill.FullName)" }) }
            else
            {
                # junctions are removed with Remove-Item on the link itself; the target is untouched
                (Get-Item -LiteralPath $LinkPath -Force).Delete()
                New-Item -ItemType Junction -Path $LinkPath -Target $Skill.FullName | Out-Null
                $Results.Add([pscustomobject]@{ Skill = $Name; Action = 're-pointed'; Detail = "-> $($Skill.FullName)" })
            }
            continue
        }

        if ($DryRun)
        { $Results.Add([pscustomobject]@{ Skill = $Name; Action = '[dry-run] create'; Detail = "-> $($Skill.FullName)" }) }
        else
        {
            New-Item -ItemType Junction -Path $LinkPath -Target $Skill.FullName | Out-Null
            $Results.Add([pscustomobject]@{ Skill = $Name; Action = 'created'; Detail = "-> $($Skill.FullName)" })
        }
    }
}

if ($Prune -and (Test-Path -LiteralPath $SkillsRoot))
{
    foreach ($Entry in Get-ChildItem -LiteralPath $SkillsRoot -Directory -Force)
    {
        if (-not (Test-IsJunction $Entry.FullName)) { continue }
        $Target = $Entry.Target
        if ($Target -and (Test-Path -LiteralPath $Target)) { continue }

        if ($DryRun)
        { $Results.Add([pscustomobject]@{ Skill = $Entry.Name; Action = '[dry-run] prune'; Detail = "dead target $Target" }) }
        else
        {
            $Entry.Delete()
            $Results.Add([pscustomobject]@{ Skill = $Entry.Name; Action = 'pruned'; Detail = "dead target $Target" })
        }
    }
}

$Results | Sort-Object Skill | Format-Table -AutoSize
$Created = @($Results | Where-Object { $_.Action -in 'created', 're-pointed' }).Count
Write-Host "sync-skills: $($Results.Count) entries processed, $Created changed."
