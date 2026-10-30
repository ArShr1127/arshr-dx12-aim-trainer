$ErrorActionPreference = 'Stop'

function New-Text {
    param([Parameter(Mandatory = $true)][int[]]$CodePoints)
    return -join ($CodePoints | ForEach-Object { [char]$_ })
}

$oldAppName = 'ArShr' + (New-Text @(0x7684, 0x6E32, 0x67D3, 0x7BA1, 0x7EBF))
$appName = $oldAppName + (New-Text @(0x4E0E, 0x7EC3, 0x67AA, 0x8F6F, 0x4EF6))
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = [System.IO.Path]::GetFullPath((Join-Path $scriptDir '..'))
$solution = Join-Path $root 'Dx12Cpp26Pipeline.sln'
$releaseRoot = Join-Path $root 'release'
$stageDir = Join-Path $releaseRoot $appName
$zipPath = Join-Path $releaseRoot "$appName.zip"
$standaloneExePath = Join-Path $releaseRoot "$appName.exe"

function Assert-ChildPath {
    param(
        [Parameter(Mandatory = $true)][string]$Parent,
        [Parameter(Mandatory = $true)][string]$Child
    )

    $parentFull = [System.IO.Path]::GetFullPath($Parent).TrimEnd('\') + '\'
    $childFull = [System.IO.Path]::GetFullPath($Child)
    if (-not $childFull.StartsWith($parentFull, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to operate outside release folder: $childFull"
    }
}

function Resolve-MSBuild {
    $candidates = @(
        'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    $vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' |
            Select-Object -First 1
        if ($found) {
            return $found
        }
    }

    throw 'Could not find MSBuild. Install Visual Studio Community with C++ desktop development tools.'
}

Assert-ChildPath -Parent $root -Child $releaseRoot
Assert-ChildPath -Parent $releaseRoot -Child $stageDir
Assert-ChildPath -Parent $releaseRoot -Child $zipPath
Assert-ChildPath -Parent $releaseRoot -Child $standaloneExePath

Get-Process -Name 'Dx12Cpp26Pipeline',$oldAppName,$appName -ErrorAction SilentlyContinue |
    Stop-Process -Force

$msbuild = Resolve-MSBuild
& $msbuild $solution /m /p:Configuration=Release /p:Platform=x64
if ($LASTEXITCODE -ne 0) {
    throw "MSBuild failed with exit code $LASTEXITCODE"
}

$exePath = Join-Path $root "bin\x64\Release\$appName.exe"
if (-not (Test-Path -LiteralPath $exePath)) {
    throw "Release exe was not found: $exePath"
}

New-Item -ItemType Directory -Force -Path $releaseRoot | Out-Null
if (Test-Path -LiteralPath $stageDir) {
    Remove-Item -LiteralPath $stageDir -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
if (Test-Path -LiteralPath $standaloneExePath) {
    Remove-Item -LiteralPath $standaloneExePath -Force
}

New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
Copy-Item -LiteralPath $exePath -Destination (Join-Path $stageDir "$appName.exe") -Force
Copy-Item -LiteralPath $exePath -Destination $standaloneExePath -Force
Copy-Item -LiteralPath (Join-Path $root 'assets') -Destination (Join-Path $stageDir 'assets') -Recurse -Force

$publicHighScores = Join-Path $stageDir 'assets\aim_high_scores.txt'
if (Test-Path -LiteralPath $publicHighScores) {
    Remove-Item -LiteralPath $publicHighScores -Force
}

$publicShaders = Join-Path $stageDir 'assets\shaders'
if (Test-Path -LiteralPath $publicShaders) {
    Remove-Item -LiteralPath $publicShaders -Recurse -Force
}

$instructions = @"
$appName

How to run:
1. Keep the exe and assets folder in the same directory.
2. Double-click $appName.exe.
3. Requires Windows 10/11, a DirectX 12 capable GPU, and a recent graphics driver.

Package notes:
- This package contains only the executable and runtime assets.
- It does not contain src, project files, solution files, obj, pdb, or other source/debug files.
- Personal high score data is not included. A player's own score file is created under assets after playing.
"@

Set-Content -LiteralPath (Join-Path $stageDir 'README.txt') -Value $instructions -Encoding UTF8

Compress-Archive -Path (Join-Path $stageDir '*') -DestinationPath $zipPath -Force

Write-Host "Packaged: $zipPath"
Write-Host "Standalone: $standaloneExePath"
