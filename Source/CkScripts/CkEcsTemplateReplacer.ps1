<#
.SYNOPSIS
    Creates a new ECS module from the CkEcsTemplate - Menu-driven interface.

.DESCRIPTION
    Interactive menu-driven tool to copy the CkEcsTemplate folder and replace all template identifiers.
    
.EXAMPLE
    .\CkEcsTemplateReplacer.ps1
    Launches interactive menu
#>

[CmdletBinding()]
param()

# Enable strict mode
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ============================================================================
# CONFIGURATION
# ============================================================================

# Get template path relative to script location
$scriptDir = Split-Path -Parent $PSCommandPath
$defaultTemplatePath = Join-Path (Split-Path -Parent $scriptDir) "CkEcsTemplate"

$script:Config = @{
    NewName = ""
    NewNamespace = "Ck"
    TemplatePath = $defaultTemplatePath
    OutputPath = ""
}

# ============================================================================
# HELPER FUNCTIONS
# ============================================================================

function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

function Write-Success {
    param([string]$Message)
    Write-ColorOutput "✓ $Message" "Green"
}

function Write-Info {
    param([string]$Message)
    Write-ColorOutput "→ $Message" "Cyan"
}

function Write-Warning {
    param([string]$Message)
    Write-ColorOutput "⚠ $Message" "Yellow"
}

function Write-Error {
    param([string]$Message)
    Write-ColorOutput "✗ $Message" "Red"
}

function Write-Header {
    param([string]$Message)
    Write-Host ""
    Write-ColorOutput "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" "Cyan"
    Write-ColorOutput " $Message" "Cyan"
    Write-ColorOutput "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" "Cyan"
    Write-Host ""
}

function Get-ValidatedPath {
    param(
        [string]$Prompt,
        [bool]$MustExist = $true
    )
    
    do {
        $path = Read-Host $Prompt
        if ([string]::IsNullOrWhiteSpace($path)) {
            return ""
        }
        
        if ($MustExist -and -not (Test-Path $path)) {
            Write-Error "Path does not exist: $path"
            continue
        }
        
        return $path
    } while ($true)
}

function Show-CurrentConfig {
    Write-Header "Current Configuration"
    
    $newPrefix = "$($script:Config.NewNamespace)$($script:Config.NewName)"
    $newMacro = ($script:Config.NewNamespace.ToUpper() + $script:Config.NewName.ToUpper())
    $newApiMacro = "${newMacro}_API"
    $newNamespaceFull = "$($script:Config.NewNamespace.ToLower())::$($script:Config.NewName.ToLower())"
    $outputPath = $script:Config.OutputPath
    if ([string]::IsNullOrWhiteSpace($outputPath)) {
        $outputPath = Split-Path $script:Config.TemplatePath -Parent
    }
    $newModulePath = Join-Path $outputPath $newPrefix
    
    Write-Host "  Module Name:     " -NoNewline
    if ([string]::IsNullOrWhiteSpace($script:Config.NewName)) {
        Write-ColorOutput "<NOT SET>" "Red"
    } else {
        Write-ColorOutput $script:Config.NewName "Green"
    }
    
    Write-Host "  Namespace:       " -NoNewline
    Write-ColorOutput $script:Config.NewNamespace "Green"
    
    Write-Host "  Template Path:   " -NoNewline
    if (Test-Path $script:Config.TemplatePath) {
        Write-ColorOutput $script:Config.TemplatePath "Green"
    } else {
        Write-ColorOutput "$($script:Config.TemplatePath) [MISSING]" "Red"
    }
    
    Write-Host "  Output Path:     " -NoNewline
    Write-ColorOutput $outputPath "White"
    
    if (-not [string]::IsNullOrWhiteSpace($script:Config.NewName)) {
        Write-Host ""
        Write-Host "  Result:"
        Write-Host "    Full Name:     " -NoNewline
        Write-ColorOutput $newPrefix "Cyan"
        Write-Host "    API Macro:     " -NoNewline
        Write-ColorOutput $newApiMacro "Magenta"
        Write-Host "    Namespace:     " -NoNewline
        Write-ColorOutput $newNamespaceFull "Cyan"
        Write-Host "    Module Path:   " -NoNewline
        Write-ColorOutput $newModulePath "Cyan"
    }
    
    Write-Host ""
}

function Show-MainMenu {
    Write-Header "ECS Module Template Generator"
    
    # Show current configuration first
    Show-CurrentConfig
    
    Write-Header "Menu Options"
    
    $moduleName = if ([string]::IsNullOrWhiteSpace($script:Config.NewName)) { "<NOT SET>" } else { $script:Config.NewName }
    $outputPath = if ([string]::IsNullOrWhiteSpace($script:Config.OutputPath)) { 
        "<DEFAULT: " + (Split-Path $script:Config.TemplatePath -Parent) + ">" 
    } else { 
        $script:Config.OutputPath 
    }
    
    Write-Host "  1. Set Module Name        (current: $moduleName)"
    Write-Host "  2. Set Namespace          (current: $($script:Config.NewNamespace))"
    Write-Host "  3. Set Template Path      (current: $($script:Config.TemplatePath))"
    Write-Host "  4. Set Output Path        (current: $outputPath)"
    Write-Host "  5. Generate Module"
    Write-Host "  Q. Quit"
    Write-Host ""
}

function Set-ModuleName {
    Write-Header "Set Module Name"
    Write-Host "Enter the module name (e.g., 'Audio', 'Interaction')"
    Write-Host "This will be used as: $($script:Config.NewNamespace)<YourName>"
    Write-Host ""
    
    $name = Read-Host "Module Name"
    if (-not [string]::IsNullOrWhiteSpace($name)) {
        $script:Config.NewName = $name
        Write-Success "Module name set to: $name"
    } else {
        Write-Warning "Module name cannot be empty"
    }
    
    Read-Host "`nPress Enter to continue"
}

function Set-Namespace {
    Write-Header "Set Namespace"
    Write-Host "Enter the namespace prefix (default: 'Ck')"
    Write-Host "Examples: 'Ck', 'Game', 'Project'"
    Write-Host ""
    Write-Host "Current: $($script:Config.NewNamespace)"
    Write-Host ""
    
    $namespace = Read-Host "Namespace (leave empty to keep current)"
    if (-not [string]::IsNullOrWhiteSpace($namespace)) {
        $script:Config.NewNamespace = $namespace
        Write-Success "Namespace set to: $namespace"
    }
    
    Read-Host "`nPress Enter to continue"
}

function Set-TemplatePath {
    Write-Header "Set Template Path"
    Write-Host "Current: $($script:Config.TemplatePath)"
    Write-Host ""
    
    $path = Get-ValidatedPath "Template Path (leave empty to keep current)" -MustExist $true
    if (-not [string]::IsNullOrWhiteSpace($path)) {
        $script:Config.TemplatePath = $path
        Write-Success "Template path set to: $path"
    }
    
    Read-Host "`nPress Enter to continue"
}

function Set-OutputPath {
    Write-Header "Set Output Path"
    $defaultPath = Split-Path $script:Config.TemplatePath -Parent
    Write-Host "Current: $defaultPath (default)"
    if (-not [string]::IsNullOrWhiteSpace($script:Config.OutputPath)) {
        Write-Host "Override: $($script:Config.OutputPath)"
    }
    Write-Host ""
    
    $path = Get-ValidatedPath "Output Path (leave empty for default)" -MustExist $false
    $script:Config.OutputPath = $path
    
    if ([string]::IsNullOrWhiteSpace($path)) {
        Write-Success "Using default output path"
    } else {
        Write-Success "Output path set to: $path"
    }
    
    Read-Host "`nPress Enter to continue"
}

function Confirm-Generation {
    Show-CurrentConfig
    
    # Validate required fields
    if ([string]::IsNullOrWhiteSpace($script:Config.NewName)) {
        Write-Error "Module name is required!"
        Read-Host "Press Enter to return to menu"
        return $false
    }
    
    if (-not (Test-Path $script:Config.TemplatePath)) {
        Write-Error "Template path does not exist: $($script:Config.TemplatePath)"
        Read-Host "Press Enter to return to menu"
        return $false
    }
    
    Write-Host "Ready to generate module. Continue? (y/n): " -NoNewline
    $response = Read-Host
    return ($response -eq 'y')
}

function Start-Generation {
    if (-not (Confirm-Generation)) {
        return
    }
    
    Write-Header "Generating Module"
    
    # Setup replacements
    $oldSuffix = "EcsTemplate"
    $oldMacroSuffix = "ECSTEMPLATE"
    $oldNamespace = "ck::ecs_template"
    $oldPrefixPartial = "Ck"
    $oldMacroPartial = "CK"
    $oldNamespacePartial = "ck"
    
    $newPrefix = "$($script:Config.NewNamespace)$($script:Config.NewName)"
    $newMacro = ($script:Config.NewNamespace.ToUpper() + $script:Config.NewName.ToUpper())
    $newNamespaceFull = ($script:Config.NewNamespace.ToLower() + "::" + $script:Config.NewName.ToLower())
    $newNamespacePartial = $script:Config.NewNamespace.ToLower()
    
    $replacements = @(
        @{ Old = "$oldPrefixPartial$oldSuffix"; New = "$($script:Config.NewNamespace)$($script:Config.NewName)" }
        @{ Old = "$oldMacroPartial$oldMacroSuffix"; New = $newMacro }
        @{ Old = $oldSuffix; New = $script:Config.NewName }
        @{ Old = $oldMacroSuffix; New = $script:Config.NewName.ToUpper() }
        @{ Old = $oldNamespace; New = $newNamespaceFull }
        @{ Old = $oldNamespacePartial; New = $newNamespacePartial }
    )
    
    # Determine output path
    $outputPath = $script:Config.OutputPath
    if ([string]::IsNullOrWhiteSpace($outputPath)) {
        $outputPath = Split-Path $script:Config.TemplatePath -Parent
    }
    
    $newModulePath = Join-Path $outputPath $newPrefix
    
    # Check if exists
    if (Test-Path $newModulePath) {
        Write-Warning "Output folder already exists: $newModulePath"
        $response = Read-Host "Delete and recreate? (y/n)"
        if ($response -ne 'y') {
            Write-Info "Operation cancelled"
            Read-Host "Press Enter to return to menu"
            return
        }
        Remove-Item $newModulePath -Recurse -Force
    }
    
    # Copy template
    Write-Info "Copying template to $newModulePath..."
    Copy-Item $script:Config.TemplatePath -Destination $newModulePath -Recurse
    Write-Success "Template copied"
    
    # Replace in files
    Write-Info "Replacing symbols in files..."
    $filesProcessed = 0
    $totalReplacements = 0
    
    $allFiles = Get-ChildItem -Path $newModulePath -File -Recurse
    
    foreach ($file in $allFiles) {
        $filesProcessed++
        $fileChanged = $false
        
        try {
            $content = Get-Content $file.FullName -Raw -Encoding UTF8
            
            foreach ($replacement in $replacements) {
                if ($content -match [regex]::Escape($replacement.Old)) {
                    $content = $content -creplace [regex]::Escape($replacement.Old), $replacement.New
                    $fileChanged = $true
                    $totalReplacements++
                }
            }
            
            if ($fileChanged) {
                Set-Content -Path $file.FullName -Value $content -Encoding UTF8 -NoNewline
            }
        } catch {
            Write-Warning "Error processing file $($file.FullName): $_"
        }
    }
    
    Write-Success "Processed $filesProcessed files with $totalReplacements replacements"
    
    # Rename files
    Write-Info "Renaming files..."
    $filesRenamed = 0
    $allFiles = Get-ChildItem -Path $newModulePath -File -Recurse | Sort-Object FullName -Descending
    
    foreach ($file in $allFiles) {
        $newFileName = $file.Name
        
        foreach ($replacement in $replacements) {
            $newFileName = $newFileName -creplace [regex]::Escape($replacement.Old), $replacement.New
        }
        
        if ($newFileName -ne $file.Name) {
            $newPath = Join-Path $file.DirectoryName $newFileName
            try {
                Move-Item -Path $file.FullName -Destination $newPath -Force
                $filesRenamed++
            } catch {
                Write-Warning "Error renaming file $($file.FullName): $_"
            }
        }
    }
    
    Write-Success "Renamed $filesRenamed files"
    
    # Rename directories
    Write-Info "Renaming directories..."
    $dirsRenamed = 0
    $allDirs = Get-ChildItem -Path $newModulePath -Directory -Recurse | 
        Sort-Object { ($_.FullName.Split([IO.Path]::DirectorySeparatorChar)).Count } -Descending
    
    foreach ($dir in $allDirs) {
        $newDirName = $dir.Name
        
        foreach ($replacement in $replacements) {
            $newDirName = $newDirName -creplace [regex]::Escape($replacement.Old), $replacement.New
        }
        
        if ($newDirName -ne $dir.Name) {
            $newPath = Join-Path $dir.Parent.FullName $newDirName
            try {
                Move-Item -Path $dir.FullName -Destination $newPath -Force
                $dirsRenamed++
            } catch {
                Write-Warning "Error renaming directory $($dir.FullName): $_"
            }
        }
    }
    
    Write-Success "Renamed $dirsRenamed directories"
    
    # Summary
    Write-Host ""
    Write-ColorOutput "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" "Green"
    Write-Success "Module creation complete!"
    Write-ColorOutput "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" "Green"
    Write-Host ""
    Write-Host "Module Location: $newModulePath"
    Write-Host "Files Processed: $filesProcessed"
    Write-Host "Replacements:    $totalReplacements"
    Write-Host "Files Renamed:   $filesRenamed"
    Write-Host "Dirs Renamed:    $dirsRenamed"
    Write-Host ""
    Write-Info "Next steps:"
    Write-Host "  1. Add module to CkFoundation.uplugin"
    Write-Host "  2. Add module to CkFoundation.Build.cs"
    Write-Host "  3. Regenerate project files"
    Write-Host "  4. Build and test"
    Write-Host ""
    
    Read-Host "Press Enter to return to menu"
}

# ============================================================================
# MAIN LOOP
# ============================================================================

do {
    Clear-Host
    Show-MainMenu
    
    Write-Host "Select option: " -NoNewline
    $key = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    $choice = $key.Character
    Write-Host $choice
    Write-Host ""
    
    switch ($choice.ToString().ToUpper()) {
        '1' { Set-ModuleName }
        '2' { Set-Namespace }
        '3' { Set-TemplatePath }
        '4' { Set-OutputPath }
        '5' { Start-Generation }
        'Q' { 
            Write-Info "Goodbye!"
            exit 0
        }
        default {
            Write-Warning "Invalid option"
            Start-Sleep -Seconds 1
        }
    }
    
} while ($true)
