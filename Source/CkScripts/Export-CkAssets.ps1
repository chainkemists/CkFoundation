<#
.SYNOPSIS
    Exports Unreal assets (DataAsset/Blueprint/BT/EQS/StateTree/enum/struct/etc.) to sibling
    JSON + text files via the CkAssetExporter commandlet, without the caller needing to know
    engine paths.

.DESCRIPTION
    Self-contained PowerShell 7 wrapper around `-run=CkAssetExporter`. Resolves the project's
    engine root from its .uproject EngineAssociation GUID (registry lookup — does NOT dot-source
    any other project script), then either:

      - runs a one-shot `UnrealEditor-Cmd.exe -run=CkAssetExporter ...` commandlet invocation, or
      - routes the request through an already-running CkAssetExporter server (see -KeepAlive)
        for fast iterating calls without paying editor-boot cost each time.

    After any run, reads the exporter's JSON output and prints a plain per-asset summary
    (path / ok / error) an agent can parse, then exits nonzero on any failure.

.PARAMETER Project
    Path to the .uproject file, or a directory containing one. Defaults to walking upward from
    the current directory until a *.uproject is found.

.PARAMETER Assets
    Semicolon-separated asset paths to export. Each entry may be an object path
    (/Game/X/Y.Y), a package path (/Game/X/Y), or a Windows disk path under Content\.

.PARAMETER Dir
    A /Game/... content directory to sweep.

.PARAMETER Classes
    Comma-separated class name filter, e.g. "DataAsset,Blueprint". Applies to -Dir sweeps and
    -List discovery.

.PARAMETER List
    List discoverable assets instead of exporting them.

.PARAMETER DumpGraph
    Dump the asset dependency graph (every asset under -Dir, default /Game: class, disk path,
    hard/soft package deps, hazard flags) to Saved/CkAssetExporter/graph.json. Migration-closure
    planning input; mutually exclusive with -List.

.PARAMETER Out
    Output directory override (defaults to the exporter's own sibling-file placement).

.PARAMETER SkipFresh
    Skip assets whose sibling export already appears up to date.

.PARAMETER Force
    Force re-export even if the sibling export looks up to date. With -StopServer: permit killing
    a BUSY server whose quit is queued behind an in-flight request (default is to leave it to
    finish -- killing destroys another session's work).

.PARAMETER KeepAlive
    If no CkAssetExporter server is currently live for this project, start one and wait for it
    to become ready (up to 10 minutes). Then, if any export/list arguments were also given on
    this invocation, submit them to the now-live server. Use this as the FIRST call of an
    iterating session; subsequent calls in the same session auto-route through the live server
    even without repeating -KeepAlive (see -StopServer to shut it down when done).

.PARAMETER StopServer
    Stop a live CkAssetExporter server: submit a quit request, wait up to 30s for the process to
    exit, force-stop it if it doesn't, and clean up server.json.

.PARAMETER Status
    Print the current server.json contents (if any), whether its pid is alive, and whether this
    project's editor/server appears to be holding its log open. Does not touch the engine or run
    anything else.

.PARAMETER TimeoutSec
    Seconds to wait for a result when routing a request through a live server. Default 120.

.EXAMPLE
    ./Export-CkAssets.ps1 -Assets "/Game/Data/DT_Foo.DT_Foo"
    One-shot export of a single asset.

.EXAMPLE
    ./Export-CkAssets.ps1 -Dir "/Game/BusterBlock/Data" -Classes "DataAsset,Blueprint"
    One-shot folder sweep filtered to two classes.

.EXAMPLE
    ./Export-CkAssets.ps1 -Dir "/Game/BusterBlock/Data" -List
    Discover what would be exported without exporting anything.

.EXAMPLE
    ./Export-CkAssets.ps1 -KeepAlive -Assets "/Game/Data/DT_Foo.DT_Foo"
    First call of an iterating session: boots (or reuses) a server, then exports through it.
    Follow-up calls in the same session omit -KeepAlive and still auto-route through the server.

.EXAMPLE
    ./Export-CkAssets.ps1 -StopServer
    Shut down the live server at the end of the session.

.EXAMPLE
    ./Export-CkAssets.ps1 -Status
    Check whether a server is live and whether the project's editor log is locked.
#>

#Requires -Version 7.0

[CmdletBinding()]
param(
    [string]$Project,
    [string]$Assets,
    [string]$Dir,
    [string]$Classes,
    [switch]$List,
    [switch]$DumpGraph,
    [string]$Out,
    [switch]$SkipFresh,
    [switch]$Force,
    [switch]$KeepAlive,
    [switch]$StopServer,
    [switch]$Status,
    [int]$TimeoutSec = 120
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Captured here, at script scope, because $PSBoundParameters is per-function-call: inside
# Invoke-Main (below) it would resolve to Invoke-Main's OWN (empty) bound parameters, not the
# script's -Assets/-Dir/-Classes/-Out/etc. Referencing the bare name there would silently make
# every ContainsKey() check false and drop every passthrough argument.
$script:TopLevelBoundParameters = $PSBoundParameters

# Internal constants (not exposed as flags — see CLAUDE.md "don't invent extra flags").
$script:OneShotTimeoutSec    = 900   # 15 minutes
$script:ServerBootTimeoutSec = 600   # 10 minutes
$script:StopServerTimeoutSec = 30

# ----------------------------------------------------------------------------------------------
# Small helpers
# ----------------------------------------------------------------------------------------------

function Fail([string]$Message) {
    [Console]::Error.WriteLine($Message)
    exit 1
}

# Safe optional-property read: Set-StrictMode -Version Latest throws on `$obj.MissingProp`
# instead of returning $null, and the exporter's JSON schemas (esp. LastRun.json / the
# server's per-request result) are produced by C++ written in parallel with this script —
# treat every field as "expected but not guaranteed" rather than trust exact shape/casing.
function Get-PropertyValue($Obj, [string[]]$Names) {
    if ($null -eq $Obj) { return $null }
    foreach ($name in $Names) {
        if ($Obj.PSObject.Properties.Name -contains $name) {
            return $Obj.$name
        }
    }
    return $null
}

function ConvertTo-QuotedArg([string]$Arg) {
    if ($Arg -match '[\s"]') {
        return '"' + ($Arg -replace '"', '\"') + '"'
    }
    return $Arg
}

# ----------------------------------------------------------------------------------------------
# Project / engine resolution (self-contained — do not dot-source other project scripts; this
# file must also work standalone in a sibling project, per the header contract).
# ----------------------------------------------------------------------------------------------

function Find-UProjectUpward([string]$StartDir) {
    $dir = $StartDir
    while ($dir) {
        $found = Get-ChildItem -LiteralPath $dir -Filter '*.uproject' -File -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) { return $found.FullName }
        $parent = Split-Path -Parent $dir
        if (-not $parent -or $parent -eq $dir) { return $null }
        $dir = $parent
    }
    return $null
}

function Resolve-UProjectPath([string]$ProjectParam) {
    if ($ProjectParam) {
        if (-not (Test-Path -LiteralPath $ProjectParam)) {
            Fail "-Project path '$ProjectParam' does not exist."
        }
        $item = Get-Item -LiteralPath $ProjectParam
        if ($item.PSIsContainer) {
            $found = Get-ChildItem -LiteralPath $ProjectParam -Filter '*.uproject' -File -ErrorAction SilentlyContinue | Select-Object -First 1
            if (-not $found) { Fail "No .uproject file found in directory '$ProjectParam'." }
            return $found.FullName
        }
        if ($item.Extension -ne '.uproject') {
            Fail "-Project '$ProjectParam' is not a .uproject file."
        }
        return (Resolve-Path -LiteralPath $ProjectParam).Path
    }

    $found = Find-UProjectUpward (Get-Location).Path
    if (-not $found) {
        Fail "Could not locate a .uproject by walking upward from '$((Get-Location).Path)'. Pass -Project explicitly."
    }
    return $found
}

function Resolve-EngineRoot([string]$UProjectPath) {
    try {
        $manifest = Get-Content -LiteralPath $UProjectPath -Raw | ConvertFrom-Json
    } catch {
        Fail "Failed to parse '$UProjectPath': $_"
    }

    $assoc = Get-PropertyValue $manifest @('EngineAssociation')
    if (-not $assoc) { Fail "EngineAssociation is empty in '$UProjectPath'." }

    $enginePath = $null

    if ($assoc -match '^\{[0-9A-Fa-f-]+\}$') {
        $regRoots = @(
            'HKCU:\Software\Epic Games\Unreal Engine\Builds',
            'HKLM:\SOFTWARE\Epic Games\Unreal Engine\Builds'
        )
        foreach ($regRoot in $regRoots) {
            if (-not (Test-Path -LiteralPath $regRoot)) { continue }
            $entry = Get-ItemProperty -LiteralPath $regRoot -ErrorAction SilentlyContinue
            if ($entry -and $entry.PSObject.Properties.Name -contains $assoc) {
                $enginePath = $entry.$assoc
                break
            }
        }
        if (-not $enginePath) {
            Fail ("EngineAssociation '$assoc' (from '$UProjectPath') was not found in HKCU or HKLM " +
                  "'Software\Epic Games\Unreal Engine\Builds'. Register the engine via the .uproject's " +
                  "right-click 'Switch Unreal Engine version', or run GenerateProjectFiles.")
        }
    } elseif ([System.IO.Path]::IsPathRooted($assoc)) {
        $enginePath = $assoc
    } else {
        $enginePath = Join-Path (Split-Path -Parent $UProjectPath) $assoc
    }

    if (-not (Test-Path -LiteralPath $enginePath)) {
        Fail "Resolved engine path does not exist: '$enginePath' (from EngineAssociation '$assoc')."
    }

    return (Resolve-Path -LiteralPath $enginePath).Path
}

function Resolve-EditorCmd([string]$UProjectPath) {
    # Ck-family source builds compile a PROJECT editor target (e.g. BusterBlockEditor-Cmd.exe in
    # the project's own Binaries) and may ship NO stock UnrealEditor binaries in the engine at
    # all. Prefer the project's editor-cmd (Development, then DebugGame) — which also avoids the
    # engine/registry lookup entirely — and only fall back to the engine's UnrealEditor-Cmd.exe.
    $projectDir  = Split-Path -Parent $UProjectPath
    $projectName = [System.IO.Path]::GetFileNameWithoutExtension($UProjectPath)

    $candidates = @(
        (Join-Path $projectDir "Binaries\Win64\${projectName}Editor-Cmd.exe"),
        (Join-Path $projectDir "Binaries\Win64\${projectName}Editor-Win64-DebugGame-Cmd.exe")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) { return $candidate }
    }

    $engineRoot = Resolve-EngineRoot $UProjectPath
    $editorCmd = Join-Path $engineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    if (-not (Test-Path -LiteralPath $editorCmd)) {
        Fail ("No editor-cmd binary found. Tried: $($candidates -join '; '); $editorCmd. " +
              'Build the project editor first (e.g. via UnrealToolbox --build).')
    }
    return $editorCmd
}

# ----------------------------------------------------------------------------------------------
# server.json / live-server helpers
# ----------------------------------------------------------------------------------------------

# Returns the parsed server.json contents if a server is live (pid alive), else $null.
# A stale server.json (pid dead, or unparseable) is deleted and treated as absent, so the
# caller falls through to a one-shot run or a fresh -KeepAlive boot.
function Get-LiveServerInfo([string]$ServerJsonPath) {
    if (-not (Test-Path -LiteralPath $ServerJsonPath)) { return $null }

    try {
        $info = Get-Content -LiteralPath $ServerJsonPath -Raw | ConvertFrom-Json
    } catch {
        Remove-Item -LiteralPath $ServerJsonPath -Force -ErrorAction SilentlyContinue
        return $null
    }

    $pid_ = Get-PropertyValue $info @('pid')
    if (-not $pid_) {
        Remove-Item -LiteralPath $ServerJsonPath -Force -ErrorAction SilentlyContinue
        return $null
    }

    # Guard against pid reuse: the pid must not only be alive, it must be an Unreal editor
    # process. A recycled pid belonging to some unrelated program would otherwise make us
    # submit requests nobody will ever answer (120s timeout instead of a clean fall-through).
    # '*Editor*' (not 'UnrealEditor*'): Ck-family projects run PROJECT editor targets, e.g. BusterBlockEditor-Cmd.
    $proc = Get-Process -Id $pid_ -ErrorAction SilentlyContinue
    if (-not $proc -or $proc.ProcessName -notlike '*Editor*') {
        Remove-Item -LiteralPath $ServerJsonPath -Force -ErrorAction SilentlyContinue
        return $null
    }

    return $info
}

function New-RequestBody {
    param(
        [string]$Op,
        [System.Collections.IDictionary]$BoundParams,
        [string]$AssetsValue,
        [string]$DirValue,
        [string]$ClassesValue,
        [string]$OutValue,
        [bool]$SkipFreshValue,
        [bool]$ForceValue
    )

    $body = [ordered]@{ op = $Op }
    if ($BoundParams.ContainsKey('Assets')) {
        $body.assets = @($AssetsValue -split ';' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    }
    if ($BoundParams.ContainsKey('Dir')) { $body.dir = $DirValue }
    if ($BoundParams.ContainsKey('Classes')) {
        $body.classes = @($ClassesValue -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    }
    if ($BoundParams.ContainsKey('Out')) { $body.out = $OutValue }
    if ($SkipFreshValue) { $body.skipFresh = $true }
    if ($ForceValue) { $body.force = $true }
    return $body
}

# Writes <requestsDir>\<guid>.json, polls <resultsDir>\<guid>.json until it appears (or times
# out), and returns the parsed result. Request/result files are left on disk as an audit trail.
function Submit-ExporterRequest {
    param(
        [Parameter(Mandatory)] $ServerInfo,
        [Parameter(Mandatory)] [System.Collections.IDictionary]$RequestBody,
        [int]$TimeoutSec = 120
    )

    $requestsDir = Get-PropertyValue $ServerInfo @('requestsDir')
    $resultsDir  = Get-PropertyValue $ServerInfo @('resultsDir')
    if (-not $requestsDir -or -not (Test-Path -LiteralPath $requestsDir)) {
        Fail "Live server's requestsDir '$requestsDir' is missing or unreadable."
    }
    if (-not $resultsDir -or -not (Test-Path -LiteralPath $resultsDir)) {
        Fail "Live server's resultsDir '$resultsDir' is missing or unreadable."
    }

    $guid        = [Guid]::NewGuid().ToString()
    $requestPath = Join-Path $requestsDir "$guid.json"
    $resultPath  = Join-Path $resultsDir  "$guid.json"

    ($RequestBody | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $requestPath -Encoding utf8NoBOM

    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        if (Test-Path -LiteralPath $resultPath) {
            Start-Sleep -Milliseconds 200   # let the writer finish flushing before we read
            try {
                return (Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json)
            } catch {
                # Result file may still be mid-write; retry on the next poll tick.
            }
        }
        Start-Sleep -Milliseconds 500
    }

    Fail ("Timed out after ${TimeoutSec}s waiting for server result '$resultPath' " +
          "(request '$guid.json' left in '$requestsDir' for inspection).")
}

function Start-ExporterServer {
    # NOTE: deliberately no Write-Output in this function -- its return value (the server-info
    # object) is captured by the caller (`$serverInfo = Start-ExporterServer ...`), and a
    # function's captured output is the concatenation of EVERY unsuppressed pipeline object it
    # emits, not just its `return` value. Mixing a progress message here would silently turn
    # $serverInfo into a 2-element array (string + object) at the call site. The caller prints
    # its own "starting server" line before invoking this.
    param([string]$EditorCmd, [string]$UProjectPath, [string]$ServerJsonPath, [int]$TimeoutSec)

    # -ExportServer, NOT -Server: "-Server" is a reserved engine switch (dedicated-server mode) that hijacks the
    # launch into a ticking headless session before the commandlet ever runs.
    $serverArgs = @($UProjectPath, '-run=CkAssetExporter', '-ExportServer', '-unattended', '-nosplash', '-nullrhi')
    $quotedArgs = $serverArgs | ForEach-Object { ConvertTo-QuotedArg $_ }

    Start-Process -FilePath $EditorCmd -ArgumentList $quotedArgs -WindowStyle Hidden | Out-Null

    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        $info = Get-LiveServerInfo $ServerJsonPath
        if ($info) { return $info }
        Start-Sleep -Seconds 2
    }

    Fail "CkAssetExporter server did not become ready within ${TimeoutSec}s (no live '$ServerJsonPath' appeared)."
}

function Invoke-StopServer {
    param([string]$ServerJsonPath, [int]$TimeoutSec, [bool]$ForceKill = $false)

    $serverInfo = Get-LiveServerInfo $ServerJsonPath
    if (-not $serverInfo) {
        Write-Output 'No live CkAssetExporter server found (server.json absent, unparseable, or stale pid). Nothing to stop.'
        return
    }

    $pid_        = Get-PropertyValue $serverInfo @('pid')
    $requestsDir = Get-PropertyValue $serverInfo @('requestsDir')

    if ($requestsDir -and (Test-Path -LiteralPath $requestsDir)) {
        $guid        = [Guid]::NewGuid().ToString()
        $requestPath = Join-Path $requestsDir "$guid.json"
        ([ordered]@{ op = 'quit' } | ConvertTo-Json) | Set-Content -LiteralPath $requestPath -Encoding utf8NoBOM
        Write-Output "Submitted quit request '$guid.json' to pid $pid_."
    } else {
        Write-Output "Server's requestsDir is missing/unreadable; skipping graceful quit request, going straight to process stop."
    }

    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    $exited = $false
    while ((Get-Date) -lt $deadline) {
        if (-not (Get-Process -Id $pid_ -ErrorAction SilentlyContinue)) { $exited = $true; break }
        Start-Sleep -Milliseconds 500
    }

    if ($exited) {
        Write-Output "Server pid $pid_ exited cleanly."
    } else {
        # Re-read the status file: a server that is BUSY (mid-request — possibly another session's
        # long sweep) has the quit QUEUED and will honor it when the current op completes. Killing
        # it would destroy in-flight work, so that requires an explicit -Force.
        $freshInfo = $null
        if (Test-Path -LiteralPath $ServerJsonPath) {
            try { $freshInfo = Get-Content -LiteralPath $ServerJsonPath -Raw | ConvertFrom-Json } catch {}
        }
        $busy      = [bool](Get-PropertyValue $freshInfo @('busy', 'Busy'))
        $currentOp = Get-PropertyValue $freshInfo @('currentOp', 'CurrentOp')

        if ($busy -and -not $ForceKill) {
            $opNote = if ($currentOp) { " (processing '$currentOp')" } else { '' }
            Write-Output "Server pid $pid_ is BUSY$opNote -- the quit request is queued and will be honored when the current op completes. NOT force-killing in-flight work; re-run '-StopServer -Force' to kill anyway."
            return
        }

        Write-Output "Server pid $pid_ did not exit within ${TimeoutSec}s after the quit request; force-stopping."
        Stop-Process -Id $pid_ -Force -ErrorAction SilentlyContinue
    }

    if (Test-Path -LiteralPath $ServerJsonPath) {
        Write-Output 'server.json still present after shutdown; removing.'
        Remove-Item -LiteralPath $ServerJsonPath -Force -ErrorAction SilentlyContinue
    }
}

# ----------------------------------------------------------------------------------------------
# Status probe (mirrors the log-lock idea in CkAuto/Check-UnrealNotRunning.ps1's
# Test-EditorRunning — an active editor/server holds Saved/Logs/*.log under an exclusive lock)
# ----------------------------------------------------------------------------------------------

function Test-ProjectLogLocked([string]$ProjectDir) {
    $logsDir = Join-Path $ProjectDir 'Saved\Logs'
    if (-not (Test-Path -LiteralPath $logsDir)) { return $false }

    $logs = Get-ChildItem -LiteralPath $logsDir -Filter '*.log' -File -ErrorAction SilentlyContinue
    foreach ($log in $logs) {
        try {
            $fs = [System.IO.File]::Open($log.FullName, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
            $fs.Close()
        } catch {
            return $true
        }
    }
    return $false
}

function Show-ExporterStatus {
    param([string]$ProjectDir, [string]$ServerJsonPath)

    if (Test-Path -LiteralPath $ServerJsonPath) {
        try {
            $info = Get-Content -LiteralPath $ServerJsonPath -Raw | ConvertFrom-Json
            Write-Output "server.json: $ServerJsonPath"
            Write-Output "  pid:         $(Get-PropertyValue $info @('pid'))"
            Write-Output "  startedAt:   $(Get-PropertyValue $info @('startedAt'))"
            Write-Output "  project:     $(Get-PropertyValue $info @('project'))"
            Write-Output "  requestsDir: $(Get-PropertyValue $info @('requestsDir'))"
            Write-Output "  resultsDir:  $(Get-PropertyValue $info @('resultsDir'))"

            Write-Output "  busy:        $(Get-PropertyValue $info @('busy'))"
            $statusOp = Get-PropertyValue $info @('currentOp')
            if ($statusOp) { Write-Output "  currentOp:   $statusOp" }
            Write-Output "  lastActivity:$(Get-PropertyValue $info @('lastActivityAt'))"

            $pid_ = Get-PropertyValue $info @('pid')
            $proc = if ($pid_) { Get-Process -Id $pid_ -ErrorAction SilentlyContinue } else { $null }
            if ($proc) {
                Write-Output '  pid alive:   true'
            } else {
                Write-Output '  pid alive:   false (STALE -- the next invocation will delete server.json and fall through to a one-shot run)'
            }
        } catch {
            Write-Output "server.json exists at '$ServerJsonPath' but failed to parse: $_"
        }
    } else {
        Write-Output "No server.json at '$ServerJsonPath' -- no CkAssetExporter server is registered."
    }

    if (Test-ProjectLogLocked $ProjectDir) {
        Write-Output 'Editor/server appears OPEN for this project (an active *.log under Saved/Logs is exclusively locked).'
    } else {
        Write-Output 'Editor/server does not appear to be open for this project (no exclusively-locked *.log under Saved/Logs).'
    }
}

# ----------------------------------------------------------------------------------------------
# Per-asset summary printing / exit-code determination
#
# LastRun.json's exact schema is not pinned by the fixed contract beyond "per-asset summary
# (path, ok/fail, error) plus process exit code" -- assumed here to mirror the server result
# schema ({ ok, perAsset:[{asset,ok,output,error}], error }) since both come from the same
# commandlet code path. Field lookups tolerate a couple of naming variants so a minor mismatch
# from the parallel C++ work doesn't hard-crash the wrapper. See SKILL.md / final report for
# this called out explicitly as an assumption to confirm once the commandlet lands.
#
# IMPORTANT (footgun documented once, obeyed everywhere below): a PowerShell function's captured
# output is the concatenation of EVERY unsuppressed pipeline object it emits, not just its final
# `return` value. A function that both Write-Output's a progress line and `return`s a bool, when
# called as `$x = Foo`, hands the caller a 2-element array -- which is always truthy as a whole,
# silently breaking any `if ($x)` check regardless of the real boolean. So: functions below are
# strictly either (a) PURE compute with zero Write-Output, safe to capture/assign/wrap in
# `exit (...)`, or (b) PRINT-ONLY with no return value, meant to be invoked as a bare statement
# (never assigned, never parenthesized) so their lines flow straight to real stdout.
# ----------------------------------------------------------------------------------------------

# (a) PURE
function Get-PerAssetEntries($ManifestObj) {
    if ($null -eq $ManifestObj) { return @() }

    # Primary, contract-given shape: an object with a perAsset/assets array property. Wrapping
    # in @(...) matters here too -- ConvertFrom-Json silently collapses a JSON array with exactly
    # one element down to a scalar (a well-documented cmdlet quirk, not specific to this field),
    # so a single-entry perAsset array would otherwise arrive as a bare object.
    $perAsset = Get-PropertyValue $ManifestObj @('entries', 'Entries', 'perAsset', 'PerAsset', 'assets', 'Assets')
    if ($null -ne $perAsset) { return @($perAsset) }

    # No wrapper property found. Two remaining possibilities, both defensive (the fixed contract
    # doesn't specify either): the root IS a single per-asset entry (detected by an asset/path
    # field), or the root IS the per-asset list itself (an actual array -- note we deliberately do
    # NOT gate on `-is [array]` first, since the single-element collapse above means a one-entry
    # array root would already have failed that check even though it should still count).
    if (Get-PropertyValue $ManifestObj @('asset', 'Asset', 'path', 'Path', 'AssetPath')) {
        return @($ManifestObj)
    }
    if ($ManifestObj -is [System.Collections.IEnumerable] -and $ManifestObj -isnot [string]) {
        return @($ManifestObj)
    }

    return @()
}

# (a) PURE
function Test-AnyAssetFailed([array]$Entries) {
    foreach ($e in $Entries) {
        $okValue = Get-PropertyValue $e @('ok', 'Ok', 'success', 'Success')
        if (-not [bool]$okValue) { return $true }
    }
    return $false
}

# (b) PRINT-ONLY -- list rows carry no ok/error fields ({assetPath, class, diskPath}), so they
# get their own printer instead of Write-PerAssetSummaryLines' ok-judgment (which would render
# every listed asset as [FAIL]).
function Write-ListRowLines([array]$Rows) {
    foreach ($r in $Rows) {
        $assetPath = Get-PropertyValue $r @('assetPath', 'asset', 'path')
        $class     = Get-PropertyValue $r @('class', 'Class', 'assetClass')
        if (-not $assetPath) { $assetPath = '<unknown-asset>' }
        if ($class) {
            Write-Output "$assetPath  [$class]"
        } else {
            Write-Output "$assetPath"
        }
    }
}

# (b) PRINT-ONLY -- call bare, never `$x = Write-PerAssetSummaryLines ...`.
function Write-PerAssetSummaryLines([array]$Entries) {
    foreach ($e in $Entries) {
        $assetPath = Get-PropertyValue $e @('asset', 'Asset', 'path', 'Path', 'AssetPath')
        if (-not $assetPath) { $assetPath = '<unknown-asset>' }

        $okValue = Get-PropertyValue $e @('ok', 'Ok', 'success', 'Success')
        $err = Get-PropertyValue $e @('error', 'Error')

        if ([bool]$okValue) {
            Write-Output "[OK]   $assetPath"
        } elseif ($err) {
            Write-Output "[FAIL] $assetPath -- $err"
        } else {
            Write-Output "[FAIL] $assetPath"
        }
    }
}

# (a) PURE
function Get-OneShotExitCode {
    param($Manifest, [array]$Entries, [int]$ProcessExitCode)

    $manifestOkValue = Get-PropertyValue $Manifest @('ok', 'Ok')
    $manifestOk = if ($null -ne $manifestOkValue) { [bool]$manifestOkValue } else { $true }
    if (Get-PropertyValue $Manifest @('error', 'Error')) { $manifestOk = $false }

    $anyFail = Test-AnyAssetFailed $Entries

    if ($ProcessExitCode -ne 0 -or -not $manifestOk -or $anyFail) { return 1 }
    return 0
}

# (a) PURE
function Get-ServerResultExitCode {
    param($Result, [array]$Entries)

    $resultOkValue = Get-PropertyValue $Result @('ok', 'Ok')
    $resultOk = if ($null -ne $resultOkValue) { [bool]$resultOkValue } else { $true }
    if (Get-PropertyValue $Result @('error', 'Error')) { $resultOk = $false }

    $anyFail = Test-AnyAssetFailed $Entries

    if (-not $resultOk -or $anyFail) { return 1 }
    return 0
}

# (b) PRINT-ONLY + terminal -- prints a server result (export, list, or dumpGraph shape) and
# exits the script. List results carry rows under `assets` with no per-row ok field, so they
# bypass the ok-judgment entirely (see Write-ListRowLines).
function Complete-ServerResultAndExit {
    param($Result, [bool]$IsList, [bool]$IsGraph = $false)

    $resultError = Get-PropertyValue $Result @('error', 'Error')

    if ($IsGraph) {
        $graphPath = Get-PropertyValue $Result @('graphPath', 'GraphPath')
        $count = Get-PropertyValue $Result @('count', 'Count')
        if ($graphPath) { Write-Output "Graph written: $graphPath ($count assets)" }
        if ($resultError) { Write-Output "Result-level error: $resultError" }
        $okValue = Get-PropertyValue $Result @('ok', 'Ok')
        $ok = if ($null -ne $okValue) { [bool]$okValue } else { $true }
        if ($resultError) { $ok = $false }
        exit ($ok ? 0 : 1)
    }

    if ($IsList) {
        $rowsRaw = Get-PropertyValue $Result @('assets', 'Assets')
        $rows = if ($null -ne $rowsRaw) { @($rowsRaw) } else { @() }
        if ($rows.Count -eq 0) { Write-Output '(No assets in list result.)' }
        Write-ListRowLines $rows
        if ($resultError) { Write-Output "Result-level error: $resultError" }
        $okValue = Get-PropertyValue $Result @('ok', 'Ok')
        $ok = if ($null -ne $okValue) { [bool]$okValue } else { $true }
        if ($resultError) { $ok = $false }
        exit ($ok ? 0 : 1)
    }

    $entries = Get-PerAssetEntries $Result
    if ($entries.Count -eq 0) { Write-Output '(No per-asset entries in server result.)' }
    Write-PerAssetSummaryLines $entries
    if ($resultError) { Write-Output "Result-level error: $resultError" }
    exit (Get-ServerResultExitCode -Result $Result -Entries $entries)
}

# ----------------------------------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------------------------------

function Invoke-Main {
    if ($StopServer -and $KeepAlive) {
        Fail '-StopServer and -KeepAlive are contradictory -- pass only one.'
    }
    if ($DumpGraph -and $List) {
        Fail '-DumpGraph and -List are mutually exclusive -- pass only one.'
    }

    $uprojectPath = Resolve-UProjectPath $Project
    $projectDir   = Split-Path -Parent $uprojectPath

    $exporterSavedDir = Join-Path $projectDir 'Saved\CkAssetExporter'
    $serverJsonPath   = Join-Path $exporterSavedDir 'server.json'
    $lastRunJsonPath  = Join-Path $exporterSavedDir 'LastRun.json'

    # -Status / -StopServer never need the engine — resolve and act, then stop.
    if ($Status) {
        Show-ExporterStatus -ProjectDir $projectDir -ServerJsonPath $serverJsonPath
        exit 0
    }

    if ($StopServer) {
        Invoke-StopServer -ServerJsonPath $serverJsonPath -TimeoutSec $script:StopServerTimeoutSec -ForceKill ([bool]$Force)
        exit 0
    }

    # Engine/editor-binary resolution is deliberately LAZY -- only the two branches that might
    # actually launch UnrealEditor-Cmd.exe (starting a fresh -KeepAlive server, or the one-shot
    # fallback) call Resolve-EditorCmd. Routing through an already-live server never touches the
    # registry or requires a registered engine at all.

    $hasRequestArgs = $script:TopLevelBoundParameters.ContainsKey('Assets') -or $script:TopLevelBoundParameters.ContainsKey('Dir') -or
                      $script:TopLevelBoundParameters.ContainsKey('Classes') -or $List -or $DumpGraph -or
                      $script:TopLevelBoundParameters.ContainsKey('Out') -or $SkipFresh -or $Force

    if ($KeepAlive) {
        $serverInfo = Get-LiveServerInfo $serverJsonPath
        if ($serverInfo) {
            $pid_ = Get-PropertyValue $serverInfo @('pid')
            Write-Output "Reusing live CkAssetExporter server (pid $pid_)."
        } else {
            $editorCmd = Resolve-EditorCmd $uprojectPath
            Write-Output "Starting CkAssetExporter server: $editorCmd `"$uprojectPath`" -run=CkAssetExporter -ExportServer ..."
            $serverInfo = Start-ExporterServer -EditorCmd $editorCmd -UProjectPath $uprojectPath -ServerJsonPath $serverJsonPath -TimeoutSec $script:ServerBootTimeoutSec
            $pid_ = Get-PropertyValue $serverInfo @('pid')
            Write-Output "CkAssetExporter server ready (pid $pid_)."
        }

        if (-not $hasRequestArgs) {
            Write-Output 'No export/list arguments were given -- server is ready, nothing further to do.'
            exit 0
        }

        $op = if ($DumpGraph) { 'dumpGraph' } elseif ($List) { 'list' } else { 'export' }
        $requestBody = New-RequestBody -Op $op -BoundParams $script:TopLevelBoundParameters `
            -AssetsValue $Assets -DirValue $Dir -ClassesValue $Classes -OutValue $Out `
            -SkipFreshValue ([bool]$SkipFresh) -ForceValue ([bool]$Force)
        $result = Submit-ExporterRequest -ServerInfo $serverInfo -RequestBody $requestBody -TimeoutSec $TimeoutSec
        Complete-ServerResultAndExit -Result $result -IsList ([bool]$List) -IsGraph ([bool]$DumpGraph)
    }

    # Not -KeepAlive: route through a live server if one already exists, else run one-shot.
    $serverInfo = Get-LiveServerInfo $serverJsonPath
    if ($serverInfo) {
        $pid_ = Get-PropertyValue $serverInfo @('pid')
        Write-Output "Routing through live CkAssetExporter server (pid $pid_)."
        $op = if ($DumpGraph) { 'dumpGraph' } elseif ($List) { 'list' } else { 'export' }
        $requestBody = New-RequestBody -Op $op -BoundParams $script:TopLevelBoundParameters `
            -AssetsValue $Assets -DirValue $Dir -ClassesValue $Classes -OutValue $Out `
            -SkipFreshValue ([bool]$SkipFresh) -ForceValue ([bool]$Force)
        $result = Submit-ExporterRequest -ServerInfo $serverInfo -RequestBody $requestBody -TimeoutSec $TimeoutSec
        Complete-ServerResultAndExit -Result $result -IsList ([bool]$List) -IsGraph ([bool]$DumpGraph)
    }

    # One-shot commandlet run.
    $editorCmd = Resolve-EditorCmd $uprojectPath
    # UE commandlet args are single-token -Key=Value form (FParse::Value scans for "Key=") — a
    # separate "-Key" "Value" pair is invisible to it and the commandlet falls through to usage.
    $passthroughArgs = @()
    if ($script:TopLevelBoundParameters.ContainsKey('Assets'))  { $passthroughArgs += "-Assets=$Assets" }
    if ($script:TopLevelBoundParameters.ContainsKey('Dir'))      { $passthroughArgs += "-Dir=$Dir" }
    if ($script:TopLevelBoundParameters.ContainsKey('Classes'))  { $passthroughArgs += "-Classes=$Classes" }
    if ($List)                                      { $passthroughArgs += '-List' }
    if ($DumpGraph)                                 { $passthroughArgs += '-DumpGraph' }
    if ($script:TopLevelBoundParameters.ContainsKey('Out'))      { $passthroughArgs += "-Out=$Out" }
    if ($SkipFresh)                                 { $passthroughArgs += '-SkipFresh' }
    if ($Force)                                     { $passthroughArgs += '-Force' }

    $coreArgs = @($uprojectPath, '-run=CkAssetExporter') + $passthroughArgs +
                @('-unattended', '-nosplash', '-nullrhi', '-stdout', '-FullStdOutLogOutput')
    $quotedArgs = $coreArgs | ForEach-Object { ConvertTo-QuotedArg $_ }

    Write-Output "Running one-shot CkAssetExporter commandlet: $editorCmd $($coreArgs -join ' ')"
    $proc = Start-Process -FilePath $editorCmd -ArgumentList $quotedArgs -NoNewWindow -PassThru

    if (-not $proc.WaitForExit($script:OneShotTimeoutSec * 1000)) {
        try { $proc.Kill() } catch {}
        Fail "CkAssetExporter one-shot run timed out after $($script:OneShotTimeoutSec)s; process killed."
    }

    Write-Output "Editor process exit code: $($proc.ExitCode)"

    # A manifest/list/graph file OLDER than this process's start is a leftover from a previous
    # run — a crashed commandlet writes nothing, and reporting stale rows as this run's result
    # is worse than admitting the crash.
    $runStartTime = $proc.StartTime

    # -DumpGraph one-shot: the commandlet writes graph.json, not LastRun.json.
    if ($DumpGraph) {
        $graphJsonPath = Join-Path $exporterSavedDir 'graph.json'
        if (-not (Test-Path -LiteralPath $graphJsonPath) -or (Get-Item -LiteralPath $graphJsonPath).LastWriteTime -lt $runStartTime) {
            [Console]::Error.WriteLine("graph.json at '$graphJsonPath' is missing or predates this run -- treating as failure.")
            exit 1
        }
        Write-Output "Graph written: $graphJsonPath"
        exit (($proc.ExitCode -eq 0) ? 0 : 1)
    }

    # -List one-shot: the commandlet writes LastList.json (discovery rows), not LastRun.json --
    # reading the export manifest here would false-fail every discovery run.
    if ($List) {
        $lastListJsonPath = Join-Path $exporterSavedDir 'LastList.json'
        if (-not (Test-Path -LiteralPath $lastListJsonPath) -or (Get-Item -LiteralPath $lastListJsonPath).LastWriteTime -lt $runStartTime) {
            [Console]::Error.WriteLine("LastList.json at '$lastListJsonPath' is missing or predates this run -- treating as failure.")
            exit 1
        }
        try {
            $listObj = Get-Content -LiteralPath $lastListJsonPath -Raw | ConvertFrom-Json
        } catch {
            [Console]::Error.WriteLine("Failed to parse '$lastListJsonPath': $_")
            exit 1
        }
        $rowsRaw = Get-PropertyValue $listObj @('assets', 'Assets')
        $rows = if ($null -ne $rowsRaw) { @($rowsRaw) } else { @() }
        if ($rows.Count -eq 0) { Write-Output '(No assets in LastList.json.)' }
        Write-ListRowLines $rows
        exit (($proc.ExitCode -eq 0) ? 0 : 1)
    }

    if (-not (Test-Path -LiteralPath $lastRunJsonPath)) {
        [Console]::Error.WriteLine("LastRun.json not found at '$lastRunJsonPath' -- treating as failure.")
        exit 1
    }
    if ((Get-Item -LiteralPath $lastRunJsonPath).LastWriteTime -lt $runStartTime) {
        [Console]::Error.WriteLine("LastRun.json at '$lastRunJsonPath' predates this run (process likely crashed before writing it) -- treating as failure.")
        exit 1
    }

    try {
        $manifest = Get-Content -LiteralPath $lastRunJsonPath -Raw | ConvertFrom-Json
    } catch {
        [Console]::Error.WriteLine("Failed to parse '$lastRunJsonPath': $_")
        exit 1
    }

    $entries = Get-PerAssetEntries $manifest
    if ($entries.Count -eq 0) {
        Write-Output '(No per-asset entries in LastRun.json.)'
    }
    Write-PerAssetSummaryLines $entries

    $manifestError = Get-PropertyValue $manifest @('error', 'Error')
    if ($manifestError) { Write-Output "Manifest-level error: $manifestError" }

    exit (Get-OneShotExitCode -Manifest $manifest -Entries $entries -ProcessExitCode $proc.ExitCode)
}

try {
    Invoke-Main
} catch {
    [Console]::Error.WriteLine("Unhandled error: $_")
    [Console]::Error.WriteLine($_.ScriptStackTrace)
    exit 1
}
