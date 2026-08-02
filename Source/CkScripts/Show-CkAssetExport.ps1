<#
.SYNOPSIS
    Renders a CkAssetExporter JSON sidecar as a compact, human/agent-readable text view, on
    demand, to stdout.

    A '<Asset>.ckexport' (this project) or '<Asset>.json' (the legacy 5.5 project) sits next to
    '<Asset>.uasset' and is plain UTF-8 JSON. This script turns one into the kind of summary the
    retired '<Asset>.txt' sibling used to carry -- except it is derived per invocation from the
    committed JSON, so it can never be staler than the JSON, and it is NEVER written next to an
    asset (nothing under Content/ is touched, ever, and no file is written at all unless -Out
    is passed).

.DESCRIPTION
    Self-contained PowerShell 7 renderer. Input is file paths only -- no project detection, no
    engine/editor interaction, no dot-sourcing.

    Family dispatch is on JSON SHAPE, not on file extension or asset-name suffix: Blueprint /
    WidgetBlueprint / AnimBlueprint, BehaviorTree, StateTree, EQS, UserDefinedEnum,
    UserDefinedStruct, the DataAsset family, Niagara, Cascade, and Material each get a bespoke
    view; anything unrecognised falls back to a generic key/value walk rather than failing.

    Two verbosity tiers: FULL (default -- property values, pins, execution flows, tooltips) and
    -Outline (asset/section headers, counts, names and types only).

.PARAMETER Path
    Sidecar files, directories (swept RECURSIVELY for *.ckexport and *.json -- never *.txt), or
    wildcard patterns. Positional; required in effect.

.PARAMETER Outline
    Terse tier: asset headers + section headers with counts + names/types only. No property
    values, no pins, no execution flows, no tooltips, no descriptions.

.PARAMETER Out
    Concatenate the output to this file (UTF-8, no BOM) instead of stdout. REFUSED if the
    resolved path has any segment named 'Content' or ends in .ckexport -- writing a derived view
    next to the assets is the drift/auto-reimport hazard this whole design exists to avoid.

.PARAMETER Stats
    Measurement mode: render every view in memory but print ONLY the per-file and aggregate byte
    statistics (JsonBytes / ViewBytes / Ratio / Family / File), not the views themselves.

.EXAMPLE
    ./Show-CkAssetExport.ps1 Content/BusterBlock/Items/TestItem_BB_IDA.ckexport
    Full view of one DataAsset sidecar.

.EXAMPLE
    ./Show-CkAssetExport.ps1 SomeWidget_BB_WBP.ckexport -Outline
    Terse skeleton of a WidgetBlueprint: hierarchy, section headers and counts only.

.EXAMPLE
    ./Show-CkAssetExport.ps1 Content/BusterBlock/NPC -Outline
    Recursive sweep of a folder, each view separated by a '===== FILE: ... =====' banner.

.EXAMPLE
    ./Show-CkAssetExport.ps1 Content -Stats
    Corpus measurement: per-file and aggregate JSON-vs-view byte ratios, no views.
#>

#Requires -Version 7.0

[CmdletBinding()]
param(
    [Parameter(Position = 0)] [string[]]$Path,
    [switch]$Outline,
    [string]$Out,
    [switch]$Stats
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Captured at script scope: $PSBoundParameters is per-function-call, so referencing the bare name
# inside Invoke-Main would resolve to Invoke-Main's OWN (empty) bound parameters.
$script:TopLevelBoundParameters = $PSBoundParameters

# Internal tunables (deliberately NOT flags -- see CkFoundation CLAUDE.md "don't invent extra flags").
$script:MaxFlattenDepth   = 24      # value-tree recursion cap inside Flatten-Value
$script:MaxGenericDepth   = 8       # object recursion cap in the unknown-family fallback walk
$script:PropIndentUnit    = '  '    # DataAsset/Blueprint exporters indent 2 spaces per depth
$script:NodeIndentUnit    = '    '  # BT/EQS/StateTree exporters indent 4 spaces per depth
$script:SidecarExtensions = @('.ckexport', '.json')

# '_<argIndex>_<32 hex>' authoring suffix UE appends to UserDefinedStruct field names.
$script:GuidSuffixPattern = '_\d+_[0-9A-Fa-f]{32}$'

# Run tallies, reported by -Stats and used for the exit code.
$script:FallbackCount     = 0
$script:ParseFailureCount = 0

# ----------------------------------------------------------------------------------------------
# Function purity discipline (mirrors Export-CkAssets.ps1)
#
# A PowerShell function's captured output is the concatenation of EVERY unsuppressed pipeline
# object it emits, not just its `return` value -- so a function that both prints and returns hands
# its caller an array. Every function below is therefore exactly one of:
#
#   (a) PURE       -- computes and returns a value, emits NOTHING to the pipeline.
#   (b) PRINT-ONLY -- writes to stderr/stdout, returns nothing; invoked as a bare statement.
#   (c) BUILD-ONLY -- appends to a caller-owned [StringBuilder], returns nothing, emits nothing.
#
# (c) exists because the views are large (a 1.9 MB sidecar renders to hundreds of KB): string
# concatenation would be quadratic. Every StringBuilder call is [void]-cast for the same reason
# the (a)/(b) split exists -- Append() RETURNS the builder, and an uncast call would inject it
# into the pipeline.
# ----------------------------------------------------------------------------------------------

# (b) PRINT-ONLY + terminal
function Fail([string]$Message) {
    [Console]::Error.WriteLine($Message)
    exit 1
}

# (b) PRINT-ONLY (stderr only -- never pollutes the rendered view on stdout)
function Write-Diag([string]$Message) {
    [Console]::Error.WriteLine($Message)
}

# ----------------------------------------------------------------------------------------------
# Tolerant JSON accessors
#
# Under `Set-StrictMode -Version Latest`, `$obj.MissingKey` THROWS rather than returning $null, and
# these sidecars span three exporter versions (legacy _meta without `format`, exporterVersion 1/2/3)
# plus per-family shape drift. Every field read goes through these -- never bare dot access.
# The `return ,$x` idiom preserves arrays: a bare `return $arr` unrolls, and a one-element array
# would arrive at the caller as a scalar.
# ----------------------------------------------------------------------------------------------

# (a) PURE
function Test-Key($Obj, [string]$Name) {
    if ($Obj -is [System.Collections.IDictionary]) { return $Obj.Contains($Name) }
    return $false
}

# (a) PURE
function Get-Field($Obj, [string]$Name, $Default = $null) {
    if ($Obj -is [System.Collections.IDictionary] -and $Obj.Contains($Name)) {
        $value = $Obj[$Name]
        if ($null -eq $value) { return ,$Default }
        return ,$value
    }
    return ,$Default
}

# (a) PURE -- ENUMERATES an array field's elements into the pipeline (nothing for an absent or
# empty field), so both `foreach ($x in (Get-Arr ...))` and `@(Get-Arr ...)` behave. Deliberately
# NOT `return ,@(...)`: that idiom protects a scalar-or-array ASSIGNMENT, but makes `@(Get-Arr ...)`
# collect one nested array instead of N elements. Get-Field is the assignment-safe accessor.
function Get-Arr($Obj, [string]$Name) {
    $value = Get-Field $Obj $Name $null
    if ($null -eq $value) { return }
    if ($value -is [System.Collections.IDictionary]) { return $value }
    return $value
}

# (a) PURE
function Get-Str($Obj, [string]$Name, [string]$Default = '') {
    $value = Get-Field $Obj $Name $null
    if ($null -eq $value) { return $Default }
    return (Format-Scalar $value)
}

# (a) PURE -- true only for a JSON `true`.
function Get-Bool($Obj, [string]$Name, [bool]$Default = $false) {
    $value = Get-Field $Obj $Name $null
    if ($null -eq $value) { return $Default }
    if ($value -is [bool]) { return $value }
    if ($value -is [string]) { return ($value -eq 'true' -or $value -eq 'True') }
    return $Default
}

# (a) PURE -- array-ness test that does NOT treat strings or dictionaries as arrays.
function Test-IsArray($Value) {
    return ($Value -is [System.Collections.IEnumerable] -and
            $Value -isnot [string] -and
            $Value -isnot [System.Collections.IDictionary])
}

# (a) PURE -- UE text spelling for bools; culture-invariant numbers so output is machine-stable.
function Format-Scalar($Value) {
    if ($null -eq $Value)      { return '' }
    if ($Value -is [bool])     { if ($Value) { return 'True' } else { return 'False' } }
    if ($Value -is [string])   { return $Value }
    if ($Value -is [System.IFormattable]) { return $Value.ToString($null, [cultureinfo]::InvariantCulture) }
    return [string]$Value
}

# (a) PURE
function Format-OneLine([string]$Text) {
    return (($Text -replace "`r`n", ' | ') -replace "`n", ' | ') -replace "`r", ' | '
}

# ----------------------------------------------------------------------------------------------
# Shared value flattening: a JSON value tree -> one compact UE-text-flavored string.
#
# Approximates the retired .txt's ExportTextItem_Direct output (byte identity is explicitly NOT a
# goal): bools as True/False, objects as (Key=Val,...), arrays as (a,b,c), and four recognised
# composite shapes -- reference stubs, inline instanced objects, FInstancedStruct (both the legacy
# GUID-keyed dialect and the current one), and map [{key,value}] arrays.
# ----------------------------------------------------------------------------------------------

# (a) PURE
function Test-ReferenceStub($Value) {
    return ((Test-Key $Value 'objectPath') -and (Test-Key $Value 'alreadyExported'))
}

# (a) PURE -- an instanced sub-object carrying its own {name,type,value} property array.
function Test-InstancedObject($Value) {
    if (-not (Test-Key $Value 'objectClass')) { return $false }
    $props = Get-Field $Value 'properties' $null
    return (Test-IsArray $props)
}

# (a) PURE
function Test-MapEntryArray($Items) {
    if ($Items.Count -eq 0) { return $false }
    foreach ($item in $Items) {
        if ($item -isnot [System.Collections.IDictionary]) { return $false }
        if ($item.Count -ne 2) { return $false }
        if (-not (Test-Key $item 'key') -or -not (Test-Key $item 'value')) { return $false }
    }
    return $true
}

# (a) PURE
function Flatten-Value($Value, [int]$Depth = 0, [bool]$Quoted = $false) {
    if ($Depth -gt $script:MaxFlattenDepth) { return '<depth cap>' }
    if ($null -eq $Value) { return '' }

    if ($Value -is [System.Collections.IDictionary]) { return (Flatten-Object $Value $Depth) }
    if (Test-IsArray $Value)                         { return (Flatten-Array $Value $Depth) }

    if ($Quoted -and $Value -is [string]) { return '"' + $Value + '"' }
    return (Format-Scalar $Value)
}

# (a) PURE
function Flatten-Object($Obj, [int]$Depth) {
    if (Test-ReferenceStub $Obj) { return (Get-Str $Obj 'objectPath') }

    if (Test-Key $Obj 'truncated') {
        if (Get-Bool $Obj 'truncated') { return '<truncated>' }
    }

    if (Test-InstancedObject $Obj) {
        $class = Get-Str $Obj 'objectClass'
        $name  = Get-Str $Obj 'objectName'
        if ($name) { return "[$class] $name" }
        return "[$class]"
    }

    # FInstancedStruct: `properties` is an OBJECT keyed by GUID-suffixed field names in the legacy
    # dialect and a {name,type,value} array in the current one -- both land here.
    if (Test-Key $Obj 'structType') {
        $structType = Get-Str $Obj 'structType'
        $structPath = Get-Str $Obj 'structPath'
        $inner      = Flatten-StructProperties (Get-Field $Obj 'properties' $null) ($Depth + 1)
        $label      = if ($structPath) { "$structType [$structPath]" } else { $structType }
        return '(' + $label + ': ' + $inner + ')'
    }

    $parts = [System.Collections.Generic.List[string]]::new()
    foreach ($key in $Obj.Keys) {
        $parts.Add([string]$key + '=' + (Flatten-Value $Obj[$key] ($Depth + 1) $true))
    }
    return '(' + ($parts -join ',') + ')'
}

# (a) PURE
function Flatten-StructProperties($Props, [int]$Depth) {
    if ($null -eq $Props) { return '()' }
    if ($Depth -gt $script:MaxFlattenDepth) { return '<depth cap>' }

    $parts = [System.Collections.Generic.List[string]]::new()

    if ($Props -is [System.Collections.IDictionary]) {
        foreach ($key in $Props.Keys) {
            $clean = [string]$key -replace $script:GuidSuffixPattern, ''
            $parts.Add($clean + '=' + (Flatten-Value $Props[$key] ($Depth + 1) $true))
        }
    } elseif (Test-IsArray $Props) {
        foreach ($entry in @($Props)) {
            if ($entry -is [System.Collections.IDictionary] -and (Test-Key $entry 'name')) {
                $clean = (Get-Str $entry 'name') -replace $script:GuidSuffixPattern, ''
                $parts.Add($clean + '=' + (Flatten-Value (Get-Field $entry 'value' $null) ($Depth + 1) $true))
            } else {
                $parts.Add((Flatten-Value $entry ($Depth + 1) $true))
            }
        }
    } else {
        return (Flatten-Value $Props $Depth $true)
    }

    return '(' + ($parts -join ',') + ')'
}

# (a) PURE
function Flatten-Array($Arr, [int]$Depth) {
    $items = @($Arr)
    if ($items.Count -eq 0) { return '()' }

    $parts = [System.Collections.Generic.List[string]]::new()

    if (Test-MapEntryArray $items) {
        foreach ($item in $items) {
            $parts.Add('(' + (Flatten-Value (Get-Field $item 'key' $null) ($Depth + 1) $true) +
                       '=' + (Flatten-Value (Get-Field $item 'value' $null) ($Depth + 1) $true) + ')')
        }
        return '(' + ($parts -join ',') + ')'
    }

    foreach ($item in $items) {
        $parts.Add((Flatten-Value $item ($Depth + 1) $true))
    }
    return '(' + ($parts -join ',') + ')'
}

# ----------------------------------------------------------------------------------------------
# Shared property-list emitter -- mirrors FCk_DataAssetExporter::DoSerializeProperties_Text
# (CkDataAssetExporter.cpp:531-587). Fed from any JSON array of {name,type,category?,value,
# tooltip?}: Blueprint classDefaults, DataAsset properties, component properties, widget slots.
# ----------------------------------------------------------------------------------------------

# (a) PURE, returns [string]
function Render-PropertyList($Props, [int]$Depth, [bool]$Full) {
    $indent = $script:PropIndentUnit * $Depth
    # NOT `$items = if (...) { @() } else { @($Props) }`: an if-EXPRESSION emits through the
    # pipeline, which unrolls -- an empty array would arrive as $null and a one-element array as a
    # bare object, and `.Count` then throws under StrictMode.
    $items = @()
    if ($null -ne $Props) { $items = @($Props) }

    $sb = [System.Text.StringBuilder]::new()

    if ($items.Count -eq 0) {
        [void]$sb.AppendLine("$indent(No exported properties)")
        return $sb.ToString()
    }

    [void]$sb.AppendLine("$indent--- Properties ($($items.Count)) ---")

    $order  = [System.Collections.Generic.List[string]]::new()
    $groups = @{}
    foreach ($prop in $items) {
        $category = Get-Str $prop 'category' 'Uncategorized'
        if ([string]::IsNullOrEmpty($category)) { $category = 'Uncategorized' }
        if (-not $groups.ContainsKey($category)) {
            $groups[$category] = [System.Collections.Generic.List[object]]::new()
            $order.Add($category)
        }
        $groups[$category].Add($prop)
    }

    foreach ($category in $order) {
        [void]$sb.AppendLine("$indent  [$category]")
        foreach ($prop in $groups[$category]) {
            $name  = Get-Str $prop 'name'
            $type  = Get-Str $prop 'type'
            $value = Get-Field $prop 'value' $null
            [void]$sb.AppendLine("$indent    ($type) $name = " + (Flatten-Value $value 0 $false))

            if (-not $Full) { continue }

            # NOTE: the per-property 'tooltip' field is deliberately NOT emitted, in any tier.
            # It is pure C++ header documentation, identical for every asset sharing a class, and
            # measured at 915,308 of the 1,334,010 bytes (69%) of the BalloonDarts_BB_BP full view
            # -- it was single-handedly holding the corpus compression ratio down to 1.76x. The
            # retired .txt never carried it either. It stays in the .ckexport for anyone who wants
            # it; read the sidecar. (The UserDefinedEnum 'Tooltip:'/' - tip' and the widget
            # 'toolTip=' lines are a DIFFERENT thing -- those the old .txt did emit, so they stay.)

            # A single inline instanced object: recurse per spec at depth+2.
            if (Test-InstancedObject $value) {
                [void]$sb.Append((Render-PropertyList (Get-Field $value 'properties' $null) ($Depth + 2) $Full))
                continue
            }

            # An ARRAY of instanced objects (the common CkFoundation trait/definition shape).
            # Flatten-Value can only render these inline as '[Class] Name', which would drop every
            # authored value -- for an item definition that IS the entire asset. Expand each.
            if (Test-IsArray $value) {
                foreach ($element in @($value)) {
                    if (-not (Test-InstancedObject $element)) { continue }
                    $elementClass = Get-Str $element 'objectClass'
                    $elementName  = Get-Str $element 'objectName'
                    [void]$sb.AppendLine("$indent      [$elementClass] $elementName")
                    [void]$sb.Append((Render-PropertyList (Get-Field $element 'properties' $null) ($Depth + 4) $Full))
                }
            }
        }
    }

    [void]$sb.AppendLine()
    return $sb.ToString()
}

# (a) PURE, returns [string] -- the -Outline stand-in for Render-PropertyList.
function Render-PropertyListOutline($Props, [int]$Depth, [bool]$PerCategory) {
    $indent = $script:PropIndentUnit * $Depth
    # NOT `$items = if (...) { @() } else { @($Props) }`: an if-EXPRESSION emits through the
    # pipeline, which unrolls -- an empty array would arrive as $null and a one-element array as a
    # bare object, and `.Count` then throws under StrictMode.
    $items = @()
    if ($null -ne $Props) { $items = @($Props) }

    $sb = [System.Text.StringBuilder]::new()

    if ($items.Count -eq 0) {
        [void]$sb.AppendLine("$indent(No exported properties)")
        return $sb.ToString()
    }

    [void]$sb.AppendLine("$indent--- Properties ($($items.Count)) ---")

    if ($PerCategory) {
        $order  = [System.Collections.Generic.List[string]]::new()
        $counts = @{}
        foreach ($prop in $items) {
            $category = Get-Str $prop 'category' 'Uncategorized'
            if ([string]::IsNullOrEmpty($category)) { $category = 'Uncategorized' }
            if (-not $counts.ContainsKey($category)) { $counts[$category] = 0; $order.Add($category) }
            $counts[$category]++
        }
        foreach ($category in $order) {
            [void]$sb.AppendLine("$indent  [$category] $($counts[$category]) properties")
        }
    }

    [void]$sb.AppendLine()
    return $sb.ToString()
}

# (a) PURE, returns [string] -- flat `name -> {value,isDefault?}` maps (BT/EQS/StateTree node
# properties). Lazy header: nothing is emitted for a node with no properties.
function Render-FlatPropertyMap($Map, [string]$Indent, [bool]$WithDefaultAnnotation) {
    $sb = [System.Text.StringBuilder]::new()
    if ($Map -isnot [System.Collections.IDictionary] -or $Map.Count -eq 0) { return $sb.ToString() }

    [void]$sb.AppendLine("${Indent}Properties:")
    foreach ($key in $Map.Keys) {
        $entry = $Map[$key]
        $value = if ($entry -is [System.Collections.IDictionary] -and (Test-Key $entry 'value')) {
            Flatten-Value (Get-Field $entry 'value' $null) 0 $false
        } else {
            Flatten-Value $entry 0 $false
        }
        $annotation = ''
        if ($WithDefaultAnnotation -and (Get-Bool $entry 'isDefault')) { $annotation = ' [default]' }
        [void]$sb.AppendLine("$Indent  $key = $value$annotation")
    }
    return $sb.ToString()
}

# (a) PURE, returns [string] -- StateTree editor-node property maps carry no 'Properties:' header.
function Render-BarePropertyMap($Map, [string]$Indent) {
    $sb = [System.Text.StringBuilder]::new()
    if ($Map -isnot [System.Collections.IDictionary] -or $Map.Count -eq 0) { return $sb.ToString() }

    foreach ($key in $Map.Keys) {
        $entry = $Map[$key]
        $value = if ($entry -is [System.Collections.IDictionary] -and (Test-Key $entry 'value')) {
            Flatten-Value (Get-Field $entry 'value' $null) 0 $false
        } else {
            Flatten-Value $entry 0 $false
        }
        $annotation = ''
        if (Get-Bool $entry 'isDefault') { $annotation = ' [default]' }
        [void]$sb.AppendLine("$Indent$key = $value$annotation")
    }
    return $sb.ToString()
}

# ----------------------------------------------------------------------------------------------
# Family dispatch -- on SHAPE, not on extension or asset-name suffix. First match wins; order
# matters (Cascade before Niagara; enum/struct before the DataAsset catch-all).
# ----------------------------------------------------------------------------------------------

# (a) PURE
function Get-AssetFamily($Json) {
    if (Test-Key $Json 'blueprintType')                            { return 'Blueprint' }
    if (Test-Key $Json 'rootNode')                                 { return 'BehaviorTree' }
    if (Test-Key $Json 'subTrees')                                 { return 'StateTree' }
    if ((Test-Key $Json 'options') -and (Test-Key $Json 'assetName')) { return 'EQS' }
    if (Test-Key $Json 'enumerators')                              { return 'UserDefinedEnum' }
    if ((Test-Key $Json 'fields') -and (Test-Key $Json 'structGuid')) { return 'UserDefinedStruct' }

    # Before the DataAsset catch-all. Verified against all 54 DataTable-shaped sidecars in both
    # corpora: every one carries BOTH assetClass=="DataTable" AND rowStruct+rows, so either test
    # alone would do -- accepting both keeps a future exporter that drops one of them working.
    if ((Get-Str $Json 'assetClass') -eq 'DataTable')              { return 'DataTable' }
    if ((Test-Key $Json 'rowStruct') -and (Test-Key $Json 'rows')) { return 'DataTable' }

    if ((Test-Key $Json 'assetClass') -and (Test-Key $Json 'properties')) { return 'DataAsset' }

    if ((Get-Str $Json 'type') -eq 'cascade')                      { return 'Cascade' }
    if ((Test-Key $Json 'system') -and (Test-Key $Json 'emitters')) {
        # A Cascade dump with no 'type' marker is still recognisable: its emitters carry a
        # 'required' module object, which a Niagara emitter never has.
        foreach ($emitter in (Get-Arr $Json 'emitters')) {
            if (Test-Key $emitter 'required') { return 'Cascade' }
        }
        return 'Niagara'
    }
    if (Test-Key $Json 'material')                                 { return 'Material' }

    return 'Unknown'
}

# ----------------------------------------------------------------------------------------------
# Per-family views. All (c) BUILD-ONLY: append to $SB, return nothing.
# ----------------------------------------------------------------------------------------------

# (c) BUILD-ONLY
function Write-DataAssetView($SB, $Json, [bool]$Full) {
    [void]$SB.AppendLine("=== DataAsset: $(Get-Str $Json 'assetName') ===")
    [void]$SB.AppendLine("Path: $(Get-Str $Json 'assetPath')")
    [void]$SB.AppendLine("Class: $(Get-Str $Json 'assetClass')")

    $parents = @(Get-Arr $Json 'parentClasses' | ForEach-Object { Format-Scalar $_ })
    [void]$SB.AppendLine("Parent Classes: $($parents -join ' -> ')")

    $properties = Get-Field $Json 'properties' $null
    if ($Full) {
        [void]$SB.Append((Render-PropertyList $properties 0 $true))
    } else {
        [void]$SB.Append((Render-PropertyListOutline $properties 0 $true))
    }
}

# (c) BUILD-ONLY
#
# Row NAMES only, never row contents. A DataTable row is a fully-flattened UE text blob (a single
# RichTextStyleRow runs ~3 KB), so echoing rows renders at ~1x compression -- the view would be a
# slower way to read a file you already have. Every DataTable has a sibling .csv with the rows, and
# this sidecar's own JSON has them too; the view's job is to tell you the shape and the row keys.
function Write-DataTableView($SB, $Json, [bool]$Full) {
    [void]$SB.AppendLine("=== DataTable: $(Get-Str $Json 'assetName') ===")
    [void]$SB.AppendLine("Path: $(Get-Str $Json 'assetPath')")
    [void]$SB.AppendLine("Row Struct: $(Get-Str $Json 'rowStruct')")

    $rows = @(Get-Arr $Json 'rows')
    [void]$SB.AppendLine("--- Rows ($($rows.Count)) ---")

    if (-not $Full) { return }

    # 'Name' is the row key in every DataTable sidecar in both corpora; the fallbacks cover an
    # exporter that renames it, and the final else keeps a nameless row visible as a position.
    $index = 0
    foreach ($row in $rows) {
        $rowName = Get-Str $row 'Name'
        if (-not $rowName) { $rowName = Get-Str $row 'RowName' }
        if (-not $rowName) { $rowName = Get-Str $row 'name' }
        if (-not $rowName) { $rowName = "<unnamed row $index>" }
        [void]$SB.AppendLine("  $rowName")
        $index++
    }

    [void]$SB.AppendLine('(Full row data: the sibling .csv, or this sidecar''s JSON)')
}

# (c) BUILD-ONLY
function Write-UserDefinedEnumView($SB, $Json, [bool]$Full) {
    [void]$SB.AppendLine("=== UserDefinedEnum: $(Get-Str $Json 'assetName') ===")
    [void]$SB.AppendLine("Path: $(Get-Str $Json 'assetPath')")

    $tooltip = Get-Str $Json 'tooltip'
    if ($tooltip) { [void]$SB.AppendLine("Tooltip: $(Format-OneLine $tooltip)") }

    $enumerators = @(Get-Arr $Json 'enumerators')
    [void]$SB.AppendLine("--- Enumerators ($($enumerators.Count)) ---")
    if ($enumerators.Count -eq 0) {
        [void]$SB.AppendLine('(No enumerators)')
        return
    }
    foreach ($entry in $enumerators) {
        $line = "  [$(Get-Str $entry 'value')] $(Get-Str $entry 'displayName') ($(Get-Str $entry 'internalName'))"
        $entryTooltip = Get-Str $entry 'tooltip'
        if ($entryTooltip) { $line += " - $(Format-OneLine $entryTooltip)" }
        [void]$SB.AppendLine($line)
    }
}

# (c) BUILD-ONLY
function Write-UserDefinedStructView($SB, $Json, [bool]$Full) {
    [void]$SB.AppendLine("=== UserDefinedStruct: $(Get-Str $Json 'assetName') ===")
    [void]$SB.AppendLine("Path: $(Get-Str $Json 'assetPath')")
    [void]$SB.AppendLine("GUID: $(Get-Str $Json 'structGuid')")

    $fields = @(Get-Arr $Json 'fields')
    [void]$SB.AppendLine("--- Fields ($($fields.Count)) ---")
    foreach ($field in $fields) {
        $line = "  ($(Get-Str $field 'type')) $(Get-Str $field 'name')"
        if ($Full) { $line += ' = ' + (Flatten-Value (Get-Field $field 'defaultValue' $null) 0 $false) }
        [void]$SB.AppendLine($line)
    }
}

# ----------------------------------------------------------------------------------------------
# Blueprint / WidgetBlueprint / AnimBlueprint
# ----------------------------------------------------------------------------------------------

# (a) PURE
function Format-InterfaceParameter($Param) {
    $type = Get-Str $Param 'type'
    if (Get-Bool $Param 'isReference') { $type += '&' }
    if (Get-Bool $Param 'isOut')       { $type = "out $type" }
    return ($type + ' ' + (Get-Str $Param 'name')).Trim()
}

# (c) BUILD-ONLY
function Write-BlueprintInterfaces($SB, $Json, [bool]$Full) {
    $interfaces = @(Get-Arr $Json 'implementedInterfaces')
    if ($interfaces.Count -eq 0) { return }

    [void]$SB.AppendLine("--- Implemented Interfaces ($($interfaces.Count)) ---")
    foreach ($iface in $interfaces) {
        [void]$SB.AppendLine("  [$(Get-Str $iface 'interfaceName')] $(Get-Str $iface 'interfacePath')")
        foreach ($fn in (Get-Arr $iface 'functions')) {
            $params = @(Get-Arr $fn 'parameters' | ForEach-Object { Format-InterfaceParameter $_ })
            [void]$SB.AppendLine("    $(Get-Str $fn 'returnType') $(Get-Str $fn 'functionName')($($params -join ', '))")
        }
    }
}

# (c) BUILD-ONLY
function Write-BlueprintVariables($SB, $Json, [bool]$Full) {
    $variables = @(Get-Arr $Json 'variables')
    if ($variables.Count -eq 0) { return }

    [void]$SB.AppendLine("--- Variables ($($variables.Count)) ---")
    foreach ($variable in $variables) {
        $line = "  [$(Get-Str $variable 'varType')] $(Get-Str $variable 'varName')"

        $default = Get-Str $variable 'defaultValue'
        if ($Full -and $default) { $line += " = $default" }

        $suffix = [System.Collections.Generic.List[string]]::new()
        $category = Get-Str $variable 'category'
        if ($category -and $category -ne 'Default') { $suffix.Add("Category: $category") }
        if (Get-Bool $variable 'isReplicated') { $suffix.Add('Replicated: Yes') }
        $repNotify = Get-Str $variable 'repNotifyFunc'
        if ($repNotify) { $suffix.Add("RepNotify: $repNotify") }
        if ($suffix.Count -gt 0) { $line += ' (' + ($suffix -join ', ') + ')' }

        [void]$SB.AppendLine($line)
    }
}

# (c) BUILD-ONLY
function Write-BlueprintComponents($SB, $Json, [bool]$Full) {
    $components = @(Get-Arr $Json 'components')
    if ($components.Count -eq 0) { return }

    [void]$SB.AppendLine("--- Components ($($components.Count)) ---")
    foreach ($component in $components) {
        [void]$SB.AppendLine("  [$(Get-Str $component 'componentClass')] $(Get-Str $component 'componentName') ($(Get-Str $component 'origin'))")
        if (-not $Full) { continue }

        $attachParent = Get-Str $component 'attachParent'
        if ($attachParent) { [void]$SB.AppendLine("    AttachParent: $attachParent") }
        $attachSocket = Get-Str $component 'attachSocket'
        if ($attachSocket) { [void]$SB.AppendLine("    AttachSocket: $attachSocket") }

        $transform = Get-Field $component 'relativeTransform' $null
        if ($transform -is [System.Collections.IDictionary]) {
            $location = Get-Field $transform 'location' $null
            $rotation = Get-Field $transform 'rotation' $null
            $scale    = Get-Field $transform 'scale' $null
            if ($location) {
                [void]$SB.AppendLine("    Location: ($(Get-Str $location 'x'), $(Get-Str $location 'y'), $(Get-Str $location 'z'))")
            }
            if ($rotation) {
                [void]$SB.AppendLine("    Rotation: (P=$(Get-Str $rotation 'pitch'), Y=$(Get-Str $rotation 'yaw'), R=$(Get-Str $rotation 'roll'))")
            }
            if ($scale) {
                [void]$SB.AppendLine("    Scale:    ($(Get-Str $scale 'x'), $(Get-Str $scale 'y'), $(Get-Str $scale 'z'))")
            }
        }

        [void]$SB.Append((Render-PropertyList (Get-Field $component 'properties' $null) 2 $true))
    }
}

# (a) PURE
function Get-GraphCategoryLabel([string]$Category) {
    switch ($Category) {
        'EventGraph' { return 'Event Graph' }
        'Function'   { return 'Function' }
        'Macro'      { return 'Macro' }
        default      { if ($Category) { return $Category } else { return 'Graph' } }
    }
}

# (a) PURE -- input and output pins, tolerating both the split (inputPins/outputPins) and the
# flat ('pins' + per-pin 'direction') shapes.
function Get-NodePins($Node, [string]$Direction) {
    $key = if ($Direction -eq 'in') { 'inputPins' } else { 'outputPins' }
    if (Test-Key $Node $key) { return ,@(Get-Arr $Node $key) }

    $matched = [System.Collections.Generic.List[object]]::new()
    foreach ($pin in (Get-Arr $Node 'pins')) {
        $pinDirection = (Get-Str $pin 'direction').ToLowerInvariant()
        $isInput = ($pinDirection -eq 'in' -or $pinDirection -eq 'input' -or $pinDirection -eq 'egpd_input')
        if (($Direction -eq 'in') -eq $isInput) { $matched.Add($pin) }
    }
    return ,@($matched)
}

# (c) BUILD-ONLY
function Write-GraphPins($SB, $Pins, [string]$Header, [string]$Arrow) {
    $items = @($Pins)
    if ($items.Count -eq 0) { return }

    [void]$SB.AppendLine("      $Header")
    foreach ($pin in $items) {
        $line = "        ($(Get-Str $pin 'pinType')) $(Get-Str $pin 'pinName')"
        $connections = @(Get-Arr $pin 'connections' | ForEach-Object { Format-Scalar $_ })
        if ($connections.Count -gt 0) {
            $line += " $Arrow " + ($connections -join ', ')
        } else {
            $default = Get-Str $pin 'defaultValue'
            if ($default) { $line += " = $default" }
        }
        [void]$SB.AppendLine($line)
    }
}

# (c) BUILD-ONLY
function Write-BlueprintGraphs($SB, $Json, [string]$Key, [bool]$Full) {
    foreach ($graph in (Get-Arr $Json $Key)) {
        $label     = Get-GraphCategoryLabel (Get-Str $graph 'graphCategory')
        $graphName = Get-Str $graph 'graphName'
        $nodes     = @(Get-Arr $graph 'nodes')
        $flows     = @(Get-Arr $graph 'executionFlows')

        if (-not $Full) {
            [void]$SB.AppendLine("--- ${label}: $graphName --- ($($nodes.Count) nodes, $($flows.Count) flows)")
            continue
        }

        [void]$SB.AppendLine("--- ${label}: $graphName ---")

        if ($flows.Count -gt 0) {
            [void]$SB.AppendLine('  Execution Flows:')
            $index = 0
            foreach ($flow in $flows) {
                $index++
                $steps = [System.Collections.Generic.List[string]]::new()
                foreach ($step in (Get-Arr $flow 'steps')) {
                    $title = Get-Str $step 'title'
                    $branchLabel = Get-Str $step 'branchLabel'
                    if ($branchLabel) { $title = "[$branchLabel] $title" }
                    $steps.Add($title)
                }
                [void]$SB.AppendLine("    [$index] $(Get-Str $flow 'entryPoint'): " + ($steps -join ' -> '))
            }
        }

        [void]$SB.AppendLine("  Nodes ($($nodes.Count)):")
        foreach ($node in $nodes) {
            [void]$SB.AppendLine("    [$(Get-Str $node 'nodeClass')] `"$(Get-Str $node 'nodeTitle')`" ($(Get-Str $node 'nodeId'))")
            $comment = Get-Str $node 'nodeComment'
            if ($comment) { [void]$SB.AppendLine("      Comment: $(Format-OneLine $comment)") }

            Write-GraphPins $SB (Get-NodePins $node 'in')  'In Pins:'  '<-'
            Write-GraphPins $SB (Get-NodePins $node 'out') 'Out Pins:' '->'
        }
    }
}

# (c) BUILD-ONLY
function Write-WidgetNode($SB, $Node, [int]$Depth, [bool]$Full) {
    $indent = $script:PropIndentUnit * $Depth
    $line   = "$indent[$(Get-Str $Node 'class')] $(Get-Str $Node 'name')"
    $namedSlot = Get-Str $Node 'namedSlot'
    if ($namedSlot) { $line += " (named slot: $namedSlot)" }
    [void]$SB.AppendLine($line)

    if ($Full) {
        $inner = $script:PropIndentUnit * ($Depth + 1)
        [void]$SB.AppendLine("${inner}isVariable=$(Get-Str $Node 'isVariable') | visibility=$(Get-Str $Node 'visibility')")

        $toolTip = Get-Str $Node 'toolTip'
        if ($toolTip) { [void]$SB.AppendLine("${inner}toolTip=`"$(Format-OneLine $toolTip)`"") }

        $slot = Get-Field $Node 'slot' $null
        if ($slot -is [System.Collections.IDictionary]) {
            [void]$SB.AppendLine("${inner}slot: $(Get-Str $slot 'type')")
            [void]$SB.Append((Render-PropertyList (Get-Field $slot 'properties' $null) ($Depth + 3) $true))
        }

        [void]$SB.Append((Render-PropertyList (Get-Field $Node 'properties' $null) ($Depth + 2) $true))
    }

    foreach ($child in (Get-Arr $Node 'children')) {
        Write-WidgetNode $SB $child ($Depth + 1) $Full
    }
}

# (c) BUILD-ONLY
function Write-BlueprintView($SB, $Json, [bool]$Full) {
    [void]$SB.AppendLine("=== Blueprint: $(Get-Str $Json 'assetName') ===")
    [void]$SB.AppendLine("Path: $(Get-Str $Json 'assetPath')")
    [void]$SB.AppendLine("Type: $(Get-Str $Json 'blueprintType')")

    $parentClass = Get-Str $Json 'parentClass'
    if ($parentClass) {
        [void]$SB.AppendLine("Parent Class: $parentClass ($(Get-Str $Json 'parentClassPath'))")
    }

    Write-BlueprintInterfaces $SB $Json $Full
    Write-BlueprintVariables  $SB $Json $Full

    if (Test-Key $Json 'classDefaults') {
        [void]$SB.AppendLine('--- Class Defaults ---')
        $classDefaults = Get-Field $Json 'classDefaults' $null
        if ($Full) {
            [void]$SB.Append((Render-PropertyList $classDefaults 0 $true))
        } else {
            [void]$SB.Append((Render-PropertyListOutline $classDefaults 0 $false))
        }
    }

    Write-BlueprintComponents $SB $Json $Full

    Write-BlueprintGraphs $SB $Json 'eventGraphs'    $Full
    Write-BlueprintGraphs $SB $Json 'functionGraphs' $Full
    Write-BlueprintGraphs $SB $Json 'macroGraphs'    $Full

    $hierarchy = Get-Field $Json 'widgetHierarchy' $null
    if ($hierarchy -is [System.Collections.IDictionary]) {
        [void]$SB.AppendLine('--- Widget Hierarchy ---')
        $root = Get-Field $hierarchy 'root' $null
        if ($root -is [System.Collections.IDictionary]) {
            Write-WidgetNode $SB $root 1 $Full
        } else {
            [void]$SB.AppendLine('  (No root widget)')
        }
    }

    # WidgetBlueprints always get the animations header, even at zero -- 'no animations' is a fact
    # a porting reader needs stated, not inferred from a missing section.
    if (Test-Key $Json 'animations') {
        $animations = @(Get-Arr $Json 'animations')
        [void]$SB.AppendLine("--- Animations ($($animations.Count)) ---")
        foreach ($animation in $animations) {
            [void]$SB.AppendLine("  $(Get-Str $animation 'name') [$(Get-Str $animation 'startTime')s -> $(Get-Str $animation 'endTime')s]")
            foreach ($widget in (Get-Arr $animation 'boundWidgets')) {
                [void]$SB.AppendLine("    -> $(Format-Scalar $widget)")
            }
        }
    }
}

# ----------------------------------------------------------------------------------------------
# BehaviorTree -- indentation and the two description-indent quirks mirror
# CkBehaviorTreeExporter.cpp:435-664 (4 spaces per depth; a Task's Description is indented 4 extra
# spaces where a Composite's is 2; a Decorator/Service emits its properties at its OWN depth).
# ----------------------------------------------------------------------------------------------

# (c) BUILD-ONLY
function Write-BtNode($SB, $Node, [int]$Depth, [bool]$Full) {
    if ($Node -isnot [System.Collections.IDictionary]) { return }

    $indent    = $script:NodeIndentUnit * $Depth
    $nodeType  = Get-Str $Node 'nodeType'
    $className = Get-Str $Node 'className'
    $nodeName  = Get-Str $Node 'nodeName'

    switch ($nodeType) {
        'Decorator' { [void]$SB.AppendLine("$indent{Decorator} ${className}: `"$nodeName`"") }
        'Service'   { [void]$SB.AppendLine("$indent{Service} ${className}: `"$nodeName`"") }
        'Task'      { [void]$SB.AppendLine("$indent[Task] ${className}: `"$nodeName`"") }
        default     { [void]$SB.AppendLine("$indent[Composite] ${className}: `"$nodeName`"") }
    }

    if ($Full) {
        $description = Get-Str $Node 'description'
        if ($description) {
            $descriptionIndent = if ($nodeType -eq 'Task') { "$indent    " } else { "$indent  " }
            [void]$SB.AppendLine("${descriptionIndent}Description: $(Format-OneLine $description)")
        }

        $propertyDepth = if ($nodeType -eq 'Decorator' -or $nodeType -eq 'Service') { $Depth } else { $Depth + 1 }
        [void]$SB.Append((Render-FlatPropertyMap (Get-Field $Node 'properties' $null) ($script:NodeIndentUnit * $propertyDepth) $true))
    }

    foreach ($service in (Get-Arr $Node 'services')) {
        Write-BtNode $SB $service ($Depth + 1) $Full
    }

    foreach ($child in (Get-Arr $Node 'children')) {
        foreach ($decorator in (Get-Arr $child 'decorators')) {
            Write-BtNode $SB $decorator ($Depth + 1) $Full
        }
        $childNode = Get-Field $child 'node' $null
        if ($null -ne $childNode) { Write-BtNode $SB $childNode ($Depth + 1) $Full }
    }
}

# (c) BUILD-ONLY
function Write-BehaviorTreeView($SB, $Json, [bool]$Full) {
    [void]$SB.AppendLine("=== Behavior Tree: $(Get-Str $Json 'assetName') ===")
    [void]$SB.AppendLine("Path: $(Get-Str $Json 'assetPath')")
    [void]$SB.AppendLine()

    $blackboard = Get-Field $Json 'blackboard' $null
    if ($blackboard -is [System.Collections.IDictionary]) {
        [void]$SB.AppendLine("--- Blackboard: $(Get-Str $blackboard 'assetName') ---")
        [void]$SB.AppendLine("  Path: $(Get-Str $blackboard 'assetPath')")

        $parent = Get-Str $blackboard 'parentBlackboard'
        if ($parent) { [void]$SB.AppendLine("  Parent: $parent") }

        $parentKeys = @(Get-Arr $blackboard 'parentKeys')
        if ($parentKeys.Count -gt 0) {
            [void]$SB.AppendLine('  Parent Keys:')
            foreach ($key in $parentKeys) {
                $synced = if (Get-Bool $key 'isInstanceSynced') { 'Yes' } else { 'No' }
                [void]$SB.AppendLine("    [$(Get-Str $key 'keyType')] $(Get-Str $key 'entryName') (InstanceSynced: $synced)")
            }
        }

        [void]$SB.AppendLine('  Keys:')
        foreach ($key in (Get-Arr $blackboard 'keys')) {
            $synced = if (Get-Bool $key 'isInstanceSynced') { 'Yes' } else { 'No' }
            [void]$SB.AppendLine("    [$(Get-Str $key 'keyType')] $(Get-Str $key 'entryName') (InstanceSynced: $synced)")
        }
        [void]$SB.AppendLine()
    }

    [void]$SB.AppendLine('--- Tree Structure ---')
    $rootNode = Get-Field $Json 'rootNode' $null
    if ($rootNode -is [System.Collections.IDictionary]) {
        Write-BtNode $SB $rootNode 0 $Full
    } else {
        [void]$SB.AppendLine('(No root node)')
    }
}

# ----------------------------------------------------------------------------------------------
# EQS
# ----------------------------------------------------------------------------------------------

# (c) BUILD-ONLY
function Write-EqsNode($SB, $Node, [string]$Label, [int]$Depth, [bool]$Full) {
    if ($Node -isnot [System.Collections.IDictionary]) { return }

    $indent      = $script:NodeIndentUnit * $Depth
    $className   = Get-Str $Node 'className'
    $description = Get-Str $Node 'description'

    if ($description) {
        [void]$SB.AppendLine("$indent[$Label] ${className}: `"$(Format-OneLine $description)`"")
    } else {
        [void]$SB.AppendLine("$indent[$Label] $className")
    }

    if ($Full) {
        [void]$SB.Append((Render-FlatPropertyMap (Get-Field $Node 'properties' $null) ($script:NodeIndentUnit * ($Depth + 1)) $false))
    }
}

# (c) BUILD-ONLY
function Write-EqsView($SB, $Json, [bool]$Full) {
    [void]$SB.AppendLine("=== EQS Query: $(Get-Str $Json 'assetName') ===")
    [void]$SB.AppendLine("Path: $(Get-Str $Json 'assetPath')")
    [void]$SB.AppendLine()

    foreach ($option in (Get-Arr $Json 'options')) {
        [void]$SB.AppendLine("--- Option $(Get-Str $option 'optionIndex') ---")
        Write-EqsNode $SB (Get-Field $option 'generator' $null) 'Generator' 1 $Full
        foreach ($test in (Get-Arr $option 'tests')) {
            Write-EqsNode $SB $test 'Test' 1 $Full
        }
        [void]$SB.AppendLine()
    }
}

# ----------------------------------------------------------------------------------------------
# StateTree
# ----------------------------------------------------------------------------------------------

# (c) BUILD-ONLY
function Write-StateTreeEditorNode($SB, $Node, [string]$Label, [int]$Depth, [bool]$Full) {
    if ($Node -isnot [System.Collections.IDictionary]) { return }

    $indent = $script:NodeIndentUnit * $Depth
    [void]$SB.AppendLine("$indent{$Label} $(Get-Str $Node 'className'): `"$(Get-Str $Node 'nodeName')`"")

    if (-not $Full) { return }

    $innerIndent = $script:NodeIndentUnit * ($Depth + 1)
    [void]$SB.Append((Render-BarePropertyMap (Get-Field $Node 'nodeProperties' $null) $innerIndent))
    [void]$SB.Append((Render-BarePropertyMap (Get-Field $Node 'instanceProperties' $null) $innerIndent))
}

# (c) BUILD-ONLY
function Write-StateTreeTransition($SB, $Transition, [int]$Depth, [bool]$Full) {
    $indent    = $script:NodeIndentUnit * $Depth
    $toState   = Get-Str $Transition 'toState'
    $linkType  = Get-Str $Transition 'toLinkType'
    $target    = if ($toState) { "$toState ($linkType)" } else { $linkType }
    $disabled  = if ((Test-Key $Transition 'enabled') -and -not (Get-Bool $Transition 'enabled' $true)) { ' [disabled]' } else { '' }

    $delay = Get-Str $Transition 'delayDuration'
    $delayText = if ($delay) { " (Delay: ${delay}s)" } else { '' }

    [void]$SB.AppendLine("$indent-> [Transition] On $(Get-Str $Transition 'trigger') -> $target (Priority: $(Get-Str $Transition 'priority'))$delayText$disabled")

    if (-not $Full) { return }

    $requiredEvent = Get-Str $Transition 'requiredEventTag'
    if ($requiredEvent) { [void]$SB.AppendLine("$indent    RequiredEvent: $requiredEvent") }

    foreach ($condition in (Get-Arr $Transition 'conditions')) {
        Write-StateTreeEditorNode $SB $condition 'Condition' ($Depth + 1) $Full
    }
}

# (c) BUILD-ONLY
function Write-StateTreeState($SB, $State, [int]$Depth, [bool]$Full) {
    if ($State -isnot [System.Collections.IDictionary]) { return }

    $indent   = $script:NodeIndentUnit * $Depth
    $disabled = if ((Test-Key $State 'enabled') -and -not (Get-Bool $State 'enabled' $true)) { ' [disabled]' } else { '' }
    [void]$SB.AppendLine("$indent[State] `"$(Get-Str $State 'name')`" ($(Get-Str $State 'type'))$disabled")

    if ($Full) {
        $description = Get-Str $State 'description'
        if ($description) { [void]$SB.AppendLine("$indent  Description: $(Format-OneLine $description)") }

        [void]$SB.AppendLine("$indent  SelectionBehavior: $(Get-Str $State 'selectionBehavior')")

        $linkedAsset = Get-Str $State 'linkedAsset'
        if ($linkedAsset) { [void]$SB.AppendLine("$indent  LinkedAsset: $linkedAsset") }
    }

    foreach ($condition in (Get-Arr $State 'enterConditions')) {
        Write-StateTreeEditorNode $SB $condition 'EnterCondition' ($Depth + 1) $Full
    }
    foreach ($task in (Get-Arr $State 'tasks')) {
        Write-StateTreeEditorNode $SB $task 'Task' ($Depth + 1) $Full
    }
    foreach ($transition in (Get-Arr $State 'transitions')) {
        Write-StateTreeTransition $SB $transition ($Depth + 1) $Full
    }
    foreach ($child in (Get-Arr $State 'children')) {
        Write-StateTreeState $SB $child ($Depth + 1) $Full
    }
}

# (c) BUILD-ONLY
function Write-StateTreeView($SB, $Json, [bool]$Full) {
    [void]$SB.AppendLine("=== State Tree: $(Get-Str $Json 'assetName') ===")
    [void]$SB.AppendLine("Path: $(Get-Str $Json 'assetPath')")
    [void]$SB.AppendLine("Schema: $(Get-Str $Json 'schema')")
    [void]$SB.AppendLine()

    $rootParameters = Get-Field $Json 'rootParameters' $null
    if ($rootParameters -is [System.Collections.IDictionary] -and $rootParameters.Count -gt 0) {
        [void]$SB.AppendLine('--- Root Parameters ---')
        [void]$SB.Append((Render-BarePropertyMap $rootParameters $script:NodeIndentUnit))
        [void]$SB.AppendLine()
    }

    $evaluators = @(Get-Arr $Json 'globalEvaluators')
    if ($evaluators.Count -gt 0) {
        [void]$SB.AppendLine('--- Global Evaluators ---')
        foreach ($evaluator in $evaluators) { Write-StateTreeEditorNode $SB $evaluator 'Evaluator' 1 $Full }
        [void]$SB.AppendLine()
    }

    $globalTasks = @(Get-Arr $Json 'globalTasks')
    if ($globalTasks.Count -gt 0) {
        [void]$SB.AppendLine('--- Global Tasks ---')
        foreach ($task in $globalTasks) { Write-StateTreeEditorNode $SB $task 'Task' 1 $Full }
        [void]$SB.AppendLine()
    }

    [void]$SB.AppendLine('--- Tree Structure ---')
    $subTrees = @(Get-Arr $Json 'subTrees')
    if ($subTrees.Count -eq 0) {
        [void]$SB.AppendLine('(No states)')
        return
    }
    foreach ($subTree in $subTrees) { Write-StateTreeState $SB $subTree 0 $Full }
}

# ----------------------------------------------------------------------------------------------
# Niagara -- text shape per CkNiagaraExporter.cpp:622-793, rendered from the JSON recipe.
# ----------------------------------------------------------------------------------------------

# (a) PURE
function Format-NiagaraCurve($Override) {
    $parts = [System.Collections.Generic.List[string]]::new()
    foreach ($channel in (Get-Arr $Override 'channels')) {
        $keys = [System.Collections.Generic.List[string]]::new()
        foreach ($key in (Get-Arr $channel 'keys')) {
            $keys.Add('(' + (Get-Str $key 't') + ', ' + (Get-Str $key 'v') + ')' + (Get-Str $key 'i'))
        }
        $parts.Add((Get-Str $channel 'name') + ': ' + ($keys -join ' '))
    }
    return 'curve<' + (Get-Str $Override 'diClass') + '> ' + ($parts -join '  ')
}

# (a) PURE
function Format-NiagaraOverride($Override) {
    $path  = Get-Str $Override 'path'
    $kind  = Get-Str $Override 'kind'
    $value = Get-Str $Override 'value'

    switch ($kind) {
        'value'        { return "$path = $value" }
        'linked'       { return "$path = linked:$value" }
        'asset'        { return "$path = asset:$value" }
        'dataInterface'{ return "$path = DI<$value>" }
        'curve'        { return "$path = " + (Format-NiagaraCurve $Override) }
        'dynamicInput' {
            $text = "$path = dyn:$value"
            $nested = @(Get-Arr $Override 'overrides')
            if ($nested.Count -gt 0) {
                $inner = @($nested | ForEach-Object { Format-NiagaraOverride $_ })
                $text += ' { ' + ($inner -join '; ') + ' }'
            }
            return $text
        }
        default        { return "$path = ${kind}:$value" }
    }
}

# (c) BUILD-ONLY
function Write-NiagaraView($SB, $Json, [bool]$Full) {
    [void]$SB.AppendLine("NIAGARA SYSTEM: $(Get-Str $Json 'system')")
    [void]$SB.AppendLine('====================================================================')
    [void]$SB.AppendLine()

    $packagePath = Get-Str $Json 'packagePath'
    if ($packagePath) {
        [void]$SB.AppendLine("Path: $packagePath")
        [void]$SB.AppendLine()
    }

    [void]$SB.AppendLine('USER PARAMETERS')
    foreach ($parameter in (Get-Arr $Json 'userParameters')) {
        $line = "  - $(Get-Str $parameter 'name') : $(Get-Str $parameter 'type')"
        $value = Get-Str $parameter 'value'
        if ($Full -and $value) { $line += " = $value" }
        [void]$SB.AppendLine($line)
    }
    [void]$SB.AppendLine()

    $emitters = @(Get-Arr $Json 'emitters')
    [void]$SB.AppendLine("EMITTERS ($($emitters.Count))")
    [void]$SB.AppendLine('--------------------------------------------------------------------')

    foreach ($emitter in $emitters) {
        $disabled = if ((Test-Key $emitter 'enabled') -and -not (Get-Bool $emitter 'enabled' $true)) { '  (DISABLED)' } else { '' }
        [void]$SB.AppendLine()
        [void]$SB.AppendLine("[EMITTER] $(Get-Str $emitter 'name')$disabled")

        if ($Full) {
            $seed = Get-Str $emitter 'randomSeed'
            $seedText = if ($seed) { " (seed $seed)" } else { '' }
            [void]$SB.AppendLine("  Sim: $(Get-Str $emitter 'sim')   LocalSpace: $(Get-Str $emitter 'localSpace')   Determinism: $(Get-Str $emitter 'determinism')$seedText")

            # 'fixedBounds' arrives pre-formatted as a single '<min> to <max>' string.
            $bounds = Get-Str $emitter 'fixedBounds'
            $boundsText = if ($bounds) { "  $bounds" } else { '' }
            [void]$SB.AppendLine("  Bounds: $(Get-Str $emitter 'boundsMode')$boundsText")
        }

        foreach ($stack in (Get-Arr $emitter 'stacks')) {
            $modules = @(Get-Arr $stack 'modules')
            [void]$SB.AppendLine("  $(Get-Str $stack 'stage') ($($modules.Count) modules):")

            $index = 0
            foreach ($module in $modules) {
                $index++
                $moduleDisabled = if ((Test-Key $module 'enabled') -and -not (Get-Bool $module 'enabled' $true)) { '  (DISABLED)' } else { '' }
                [void]$SB.AppendLine("    $index. $(Get-Str $module 'name')$moduleDisabled")
                if (-not $Full) { continue }

                foreach ($moduleInput in (Get-Arr $module 'inputs')) {
                    $inputValue = Get-Str $moduleInput 'value'
                    if (-not $inputValue) { continue }
                    [void]$SB.AppendLine("         $(Get-Str $moduleInput 'name') = $inputValue")
                }
                foreach ($override in (Get-Arr $module 'overrides')) {
                    [void]$SB.AppendLine("         [override] $(Format-NiagaraOverride $override)")
                }
            }

            if (-not $Full) { continue }

            $unattached = @(Get-Arr $stack 'unattachedOverrides')
            if ($unattached.Count -gt 0) {
                [void]$SB.AppendLine('    [unattached overrides]')
                foreach ($override in $unattached) {
                    [void]$SB.AppendLine("       $(Get-Str $override 'owner'): $(Format-NiagaraOverride $override)")
                }
            }

            $values = @(Get-Arr $stack 'values')
            if ($values.Count -gt 0) {
                [void]$SB.AppendLine('    [values]')
                foreach ($value in $values) {
                    [void]$SB.AppendLine("       $(Get-Str $value 'name') = $(Get-Str $value 'value')")
                }
            }
        }

        $renderers = @(Get-Arr $emitter 'renderers')
        if ($renderers.Count -gt 0) {
            [void]$SB.AppendLine("  Renderers ($($renderers.Count)):")
            foreach ($renderer in $renderers) {
                $line = "    - $(Get-Str $renderer 'type')"
                foreach ($pair in @(
                    @('material',            'Material'),
                    @('alignment',           'Alignment'),
                    @('facing',              'Facing'),
                    @('facingMode',          'Facing'),
                    @('sortMode',            'Sort'),
                    @('subImage',            'SubUV'),
                    @('radiusScale',         'RadiusScale'),
                    @('affectsTranslucency', 'AffectsTranslucency'))) {
                    $value = Get-Str $renderer $pair[0]
                    if ($value) { $line += "  $($pair[1]): $value" }
                }
                [void]$SB.AppendLine($line)

                foreach ($mesh in (Get-Arr $renderer 'meshes')) {
                    $meshLine = "        mesh: $(Get-Str $mesh 'mesh')"
                    $meshScale = Get-Str $mesh 'scale'
                    if ($meshScale) { $meshLine += "  scale: $meshScale" }
                    [void]$SB.AppendLine($meshLine)
                }
                foreach ($material in (Get-Arr $renderer 'overrideMaterials')) {
                    [void]$SB.AppendLine("        material: $(Format-Scalar $material)")
                }
            }
        }
    }
}

# ----------------------------------------------------------------------------------------------
# Cascade -- text shape per CkCascadeExporter.cpp:405-451 (which itself renders FROM the json).
# ----------------------------------------------------------------------------------------------

# (c) BUILD-ONLY
function Write-CascadeModule($SB, $Module, [string]$Label, [string]$Indent, [bool]$Full) {
    if ($Module -isnot [System.Collections.IDictionary]) { return }
    if (-not (Test-Key $Module 'class')) { return }

    $disabled = if ((Test-Key $Module 'enabled') -and -not (Get-Bool $Module 'enabled' $true)) { '  (DISABLED)' } else { '' }
    [void]$SB.AppendLine("$Indent$Label$(Get-Str $Module 'class')$disabled")

    if (-not $Full) { return }
    foreach ($prop in (Get-Arr $Module 'props')) {
        [void]$SB.AppendLine("$Indent   $(Get-Str $prop 'name') = $(Get-Str $prop 'value')")
    }
}

# (c) BUILD-ONLY
function Write-CascadeView($SB, $Json, [bool]$Full) {
    [void]$SB.AppendLine("CASCADE SYSTEM: $(Get-Str $Json 'system')")
    [void]$SB.AppendLine('====================================================================')

    foreach ($emitter in (Get-Arr $Json 'emitters')) {
        $disabled = if ((Test-Key $emitter 'enabled') -and -not (Get-Bool $emitter 'enabled' $true)) { '  (DISABLED)' } else { '' }
        [void]$SB.AppendLine()
        [void]$SB.AppendLine("[EMITTER] $(Get-Str $emitter 'name')$disabled")

        $emitterError = Get-Str $emitter 'error'
        if ($emitterError) { [void]$SB.AppendLine("  <$emitterError>") }

        Write-CascadeModule $SB (Get-Field $emitter 'required' $null) 'Required: ' '  ' $Full
        Write-CascadeModule $SB (Get-Field $emitter 'spawn' $null)    'Spawn: '    '  ' $Full
        Write-CascadeModule $SB (Get-Field $emitter 'typeData' $null) 'TypeData: ' '  ' $Full

        $modules = @(Get-Arr $emitter 'modules')
        if ($modules.Count -eq 0) { continue }

        [void]$SB.AppendLine("  Modules ($($modules.Count)):")
        $index = 0
        foreach ($module in $modules) {
            $index++
            Write-CascadeModule $SB $module "$index. " '    ' $Full
        }
    }
}

# ----------------------------------------------------------------------------------------------
# Material + the generic fallback walk
# ----------------------------------------------------------------------------------------------

# (c) BUILD-ONLY -- key/value walk used by Material and by the unknown-family fallback.
function Write-GenericWalk($SB, $Node, [int]$Depth, [bool]$Full, [string[]]$SkipKeys) {
    if ($Depth -gt $script:MaxGenericDepth) {
        [void]$SB.AppendLine(($script:PropIndentUnit * $Depth) + '<depth cap>')
        return
    }
    if ($Node -isnot [System.Collections.IDictionary]) { return }

    $indent = $script:PropIndentUnit * $Depth

    foreach ($key in $Node.Keys) {
        if ($SkipKeys -contains [string]$key) { continue }
        $value = $Node[$key]

        if ($value -is [System.Collections.IDictionary]) {
            [void]$SB.AppendLine("$indent${key}:")
            Write-GenericWalk $SB $value ($Depth + 1) $Full @()
            continue
        }

        if (Test-IsArray $value) {
            $items = @($value)
            [void]$SB.AppendLine("$indent$key ($($items.Count)):")
            if (-not $Full) { continue }
            $index = 0
            foreach ($item in $items) {
                [void]$SB.AppendLine(($script:PropIndentUnit * ($Depth + 1)) + "[$index] " + (Flatten-Value $item 0 $false))
                $index++
            }
            continue
        }

        if (-not $Full) {
            [void]$SB.AppendLine("$indent${key}:")
            continue
        }
        [void]$SB.AppendLine("$indent${key}: " + (Flatten-Value $value 0 $false))
    }
}

# (c) BUILD-ONLY
function Write-MaterialView($SB, $Json, [bool]$Full) {
    $name = Get-Str $Json 'material'
    if (-not $name) { $name = Get-Str $Json 'assetName' }
    [void]$SB.AppendLine("=== Material: $name ===")

    $path = Get-Str $Json 'packagePath'
    if (-not $path) { $path = Get-Str $Json 'assetPath' }
    if ($path) { [void]$SB.AppendLine("Path: $path") }

    Write-GenericWalk $SB $Json 0 $Full @('material', 'packagePath', 'assetName', 'assetPath', '_meta')
}

# (c) BUILD-ONLY
function Write-UnknownView($SB, $Json, [bool]$Full) {
    $script:FallbackCount++

    $keys = @()
    if ($Json -is [System.Collections.IDictionary]) {
        $keys = @($Json.Keys | Where-Object { $_ -ne '_meta' } | ForEach-Object { [string]$_ })
    }
    [void]$SB.AppendLine("=== Unknown ($($keys -join ', ')) ===")

    $assetName = Get-Str $Json 'assetName'
    if ($assetName) { [void]$SB.AppendLine("Name: $assetName") }
    $assetPath = Get-Str $Json 'assetPath'
    if ($assetPath) { [void]$SB.AppendLine("Path: $assetPath") }
    $assetClass = Get-Str $Json 'assetClass'
    if ($assetClass) { [void]$SB.AppendLine("Class: $assetClass") }

    Write-GenericWalk $SB $Json 0 $Full @('assetName', 'assetPath', 'assetClass', '_meta')
}

# ----------------------------------------------------------------------------------------------
# View orchestration
# ----------------------------------------------------------------------------------------------

# (a) PURE -- returns the rendered view text for one parsed sidecar.
function Get-AssetView($Json, [string]$Family, [bool]$Full) {
    $sb = [System.Text.StringBuilder]::new()

    switch ($Family) {
        'Blueprint'         { Write-BlueprintView         $sb $Json $Full }
        'BehaviorTree'      { Write-BehaviorTreeView      $sb $Json $Full }
        'StateTree'         { Write-StateTreeView         $sb $Json $Full }
        'EQS'               { Write-EqsView               $sb $Json $Full }
        'UserDefinedEnum'   { Write-UserDefinedEnumView   $sb $Json $Full }
        'UserDefinedStruct' { Write-UserDefinedStructView $sb $Json $Full }
        'DataAsset'         { Write-DataAssetView         $sb $Json $Full }
        'DataTable'         { Write-DataTableView         $sb $Json $Full }
        'Niagara'           { Write-NiagaraView           $sb $Json $Full }
        'Cascade'           { Write-CascadeView           $sb $Json $Full }
        'Material'          { Write-MaterialView          $sb $Json $Full }
        default             { Write-UnknownView           $sb $Json $Full }
    }

    return $sb.ToString()
}

# ----------------------------------------------------------------------------------------------
# Input resolution
# ----------------------------------------------------------------------------------------------

# (a) PURE -- returns the resolved, de-duplicated, sorted sidecar file list.
function Resolve-InputFiles([string[]]$Patterns) {
    $resolved = [System.Collections.Generic.List[string]]::new()
    $seen     = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

    foreach ($pattern in $Patterns) {
        if ([string]::IsNullOrWhiteSpace($pattern)) { continue }

        $candidates = @()
        if (Test-Path -LiteralPath $pattern) {
            $candidates = @(Get-Item -LiteralPath $pattern)
        } else {
            $candidates = @(Get-Item -Path $pattern -ErrorAction SilentlyContinue)
            if ($candidates.Count -eq 0) {
                Write-Diag "WARN: no file or directory matched '$pattern'."
                continue
            }
        }

        foreach ($candidate in $candidates) {
            if ($candidate.PSIsContainer) {
                # Recursive sweep. Explicitly extension-filtered: *.txt of any flavour (including
                # the retired lossy summaries and the WBP paste artifacts) is NEVER an input.
                $swept = Get-ChildItem -LiteralPath $candidate.FullName -Recurse -File -ErrorAction SilentlyContinue |
                         Where-Object { $script:SidecarExtensions -contains $_.Extension.ToLowerInvariant() }
                foreach ($file in $swept) {
                    if ($seen.Add($file.FullName)) { $resolved.Add($file.FullName) }
                }
            } else {
                if ($seen.Add($candidate.FullName)) { $resolved.Add($candidate.FullName) }
            }
        }
    }

    return ,@($resolved | Sort-Object)
}

# (b) PRINT-ONLY + terminal on refusal -- validates -Out before any work happens.
function Assert-OutPathAllowed([string]$OutPath) {
    $full = $OutPath
    if (-not [System.IO.Path]::IsPathRooted($full)) {
        $full = Join-Path (Get-Location).Path $OutPath
    }
    $full = [System.IO.Path]::GetFullPath($full)

    foreach ($segment in ($full -split '[\\/]+')) {
        if ($segment -ieq 'Content') {
            Fail ("-Out path '$OutPath' resolves under a 'Content' directory. Refused: a rendered view " +
                  "written beside the assets is exactly the drift surface / auto-reimport hazard this " +
                  'script exists to avoid. Write it somewhere outside Content/, or drop -Out and use stdout.')
        }
    }

    if ([System.IO.Path]::GetExtension($full) -ieq '.ckexport') {
        Fail "-Out path '$OutPath' ends in .ckexport. Refused: that extension is the exporter's own sidecar, not a view."
    }
}

# (a) PURE
function Get-MedianRatio([double[]]$Ratios) {
    if ($Ratios.Count -eq 0) { return 0.0 }
    $sorted = @($Ratios | Sort-Object)
    $middle = [int][Math]::Floor($sorted.Count / 2)
    if ($sorted.Count % 2 -eq 1) { return $sorted[$middle] }
    return (($sorted[$middle - 1] + $sorted[$middle]) / 2.0)
}

# ----------------------------------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------------------------------

function Invoke-Main {
    if (-not $script:TopLevelBoundParameters.ContainsKey('Path') -or $null -eq $Path -or @($Path).Count -eq 0) {
        Fail ('Usage: Show-CkAssetExport.ps1 <sidecar|directory|wildcard> [...] [-Outline] [-Stats] [-Out <file>]' + [Environment]::NewLine +
              "  e.g. ./Show-CkAssetExport.ps1 Content/BusterBlock/Items/TestItem_BB_IDA.ckexport" + [Environment]::NewLine +
              "       ./Show-CkAssetExport.ps1 Content -Stats")
    }

    if ($script:TopLevelBoundParameters.ContainsKey('Out')) {
        if ([string]::IsNullOrWhiteSpace($Out)) { Fail '-Out was passed with an empty value.' }
        Assert-OutPathAllowed $Out
    }

    $files = Resolve-InputFiles $Path
    if ($files.Count -eq 0) {
        Fail "No sidecar files (*.ckexport / *.json) found for: $($Path -join ', ')"
    }

    $full        = -not $Outline
    $showBanners = ($files.Count -gt 1) -and (-not $Stats)

    $output      = [System.Text.StringBuilder]::new()
    $totalJson   = [long]0
    $totalView   = [long]0
    $ratios      = [System.Collections.Generic.List[double]]::new()
    $renderCount = 0

    foreach ($file in $files) {
        $jsonBytes = (Get-Item -LiteralPath $file).Length

        $json = $null
        try {
            $json = Get-Content -LiteralPath $file -Raw -Encoding utf8 | ConvertFrom-Json -AsHashtable -Depth 100
        } catch {
            $script:ParseFailureCount++
            Write-Diag "WARN: failed to parse '$file' as JSON: $_"
            continue
        }

        if ($json -isnot [System.Collections.IDictionary]) {
            $script:ParseFailureCount++
            Write-Diag "WARN: '$file' parsed but its root is not a JSON object -- skipped."
            continue
        }

        $family = Get-AssetFamily $json
        $view   = Get-AssetView $json $family $full
        $renderCount++

        $viewBytes = [System.Text.Encoding]::UTF8.GetByteCount($view)
        $totalJson += $jsonBytes
        $totalView += $viewBytes
        $ratio = if ($viewBytes -gt 0) { [double]$jsonBytes / [double]$viewBytes } else { 0.0 }
        $ratios.Add($ratio)

        if ($Stats) {
            [void]$output.AppendLine(('{0,10}  {1,9}  {2,6}  {3,-17}  {4}' -f
                $jsonBytes,
                $viewBytes,
                ($ratio.ToString('F2', [cultureinfo]::InvariantCulture) + 'x'),
                $family,
                ($file -replace '\\', '/')))
            continue
        }

        if ($showBanners) {
            [void]$output.AppendLine('===== FILE: ' + ($file -replace '\\', '/') + ' =====')
        }
        [void]$output.Append($view)
        if ($showBanners) { [void]$output.AppendLine() }
    }

    if ($Stats) {
        $aggregateRatio = if ($totalView -gt 0) { [double]$totalJson / [double]$totalView } else { 0.0 }
        $median         = Get-MedianRatio $ratios.ToArray()
        [void]$output.AppendLine(
            "TOTAL json=$totalJson view=$totalView ratio=$($aggregateRatio.ToString('F2', [cultureinfo]::InvariantCulture))x" +
            "  files=$renderCount  (median ratio $($median.ToString('F1', [cultureinfo]::InvariantCulture))x)")
        [void]$output.AppendLine(
            "FALLBACK generic renders=$($script:FallbackCount)  parse failures=$($script:ParseFailureCount)")
    }

    $text = $output.ToString()

    if ($script:TopLevelBoundParameters.ContainsKey('Out')) {
        $directory = Split-Path -Parent $Out
        if ($directory -and -not (Test-Path -LiteralPath $directory)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
        }
        Set-Content -LiteralPath $Out -Value $text -NoNewline -Encoding utf8NoBOM
        Write-Diag "Wrote $($text.Length) chars to '$Out' ($renderCount view(s))."
    } else {
        # Emitted as LINES, not as one blob: `Write-Output $text` renders identically but makes
        # `| Select-String`, `| Select-Object -First N` and `| Out-File` operate on the whole view
        # as a single object, which is useless for an agent grepping a 900-line render. The single
        # trailing newline is dropped first so splitting doesn't append a phantom blank line.
        Write-Output (($text -replace '(\r?\n)$', '') -split "`r?`n")
    }

    if ($script:ParseFailureCount -gt 0) { exit 1 }
    exit 0
}

try {
    Invoke-Main
} catch {
    [Console]::Error.WriteLine("Unhandled error: $_")
    [Console]::Error.WriteLine($_.ScriptStackTrace)
    exit 1
}
