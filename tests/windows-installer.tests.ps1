[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $BuildDirectory,

    [Parameter(Mandatory)]
    [ValidateSet('Release')]
    [string] $Configuration,

    [Parameter(Mandatory)]
    [string] $PackageDirectory,

    [Parameter(Mandatory)]
    [string] $InstallerPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$expectedLegalFileNames = @(
    'JUCE-LICENSE.md'
    'JUCE-SPDX.json'
    'LICENSE'
    'THIRD_PARTY_NOTICES.md'
    'VST3-SDK-LICENSE.txt'
)

$pluginDirectory = Join-Path $BuildDirectory "ShowLyricsPlugin_artefacts\$Configuration\VST3\Show Lyrics.vst3"
$pluginBinary = Join-Path $pluginDirectory 'Contents\x86_64-win\Show Lyrics.vst3'
$moduleInfo = Join-Path $pluginDirectory 'Contents\Resources\moduleinfo.json'
foreach ($requiredPath in @($pluginBinary, $moduleInfo)) {
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "Missing Windows VST3 bundle file: $requiredPath"
    }
}

if (-not (Test-Path -LiteralPath $InstallerPath -PathType Leaf)) {
    throw "Windows installer was not created: $InstallerPath"
}
$installerBytes = [IO.File]::ReadAllBytes($InstallerPath)
if ($installerBytes.Length -lt 2 -or $installerBytes[0] -ne 0x4D -or $installerBytes[1] -ne 0x5A) {
    throw "Windows installer does not have a valid PE header: $InstallerPath"
}

$packageMatch = [regex]::Match(
    (Split-Path -Leaf $InstallerPath),
    '^Show-Lyrics-(?<version>\d+\.\d+\.\d+)-windows-x64\.exe$'
)
if (-not $packageMatch.Success) {
    throw "Unexpected installer filename: $InstallerPath"
}
$expectedVersion = $packageMatch.Groups['version'].Value

$installerVersionInfo = [Diagnostics.FileVersionInfo]::GetVersionInfo($InstallerPath)
$expectedInstallerVersionInfo = [ordered]@{
    CompanyName = 'jfheinrich'
    FileDescription = 'Show Lyrics Windows Installer'
    FileVersion = $expectedVersion
    OriginalFilename = (Split-Path -Leaf $InstallerPath)
    ProductName = 'Show Lyrics'
    ProductVersion = $expectedVersion
}
foreach ($propertyName in $expectedInstallerVersionInfo.Keys) {
    $expectedValue = $expectedInstallerVersionInfo[$propertyName]
    $actualValue = $installerVersionInfo.$propertyName
    if ($actualValue -ne $expectedValue) {
        throw "Unexpected installer ${propertyName}: expected '$expectedValue', actual '$actualValue'"
    }
}

$checksumPath = "$InstallerPath.sha256"
if (-not (Test-Path -LiteralPath $checksumPath -PathType Leaf)) {
    throw "Windows installer checksum was not created: $checksumPath"
}
$recordedChecksum = ((Get-Content -Raw -LiteralPath $checksumPath).Trim() -split '\s+')[0]
$actualChecksum = (Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256).Hash
if ($recordedChecksum -ne $actualChecksum) {
    throw "Windows installer SHA-256 mismatch: expected $recordedChecksum, actual $actualChecksum"
}

$generatedScripts = @(Get-ChildItem -LiteralPath $PackageDirectory -Filter '*.nsi' -File -Recurse)
if ($generatedScripts.Count -ne 1) {
    throw "Expected one generated NSIS script, found $($generatedScripts.Count)."
}

$nsisScript = Get-Content -Raw -LiteralPath $generatedScripts[0].FullName
if (-not $nsisScript.Contains('RequestExecutionLevel admin', [StringComparison]::Ordinal)) {
    throw 'Generated installer does not request administrator rights.'
}
if (-not $nsisScript.Contains('Show Lyrics by jfheinrich', [StringComparison]::Ordinal)) {
    throw 'Generated installer does not contain the expected manufacturer branding.'
}
if ($nsisScript -notmatch '(?ms)Push "Publisher"\s+Push "jfheinrich"') {
    throw 'Generated installer does not register the expected publisher.'
}

$expectedComponents = @(
    [pscustomobject] @{ Id = 'Documentation'; DisplayName = 'Licenses and notices' }
    [pscustomobject] @{ Id = 'VST3'; DisplayName = 'Show Lyrics VST3' }
)
foreach ($component in $expectedComponents) {
    $visibleDeclaration = "Section `"$($component.DisplayName)`" $($component.Id)"
    $hiddenDeclaration = "Section `"-$($component.DisplayName)`" $($component.Id)"
    if (-not $nsisScript.Contains($visibleDeclaration, [StringComparison]::Ordinal)) {
        throw "Generated installer does not display required component: $($component.DisplayName)"
    }
    if ($nsisScript.Contains($hiddenDeclaration, [StringComparison]::Ordinal)) {
        throw "Generated installer hides required component: $($component.DisplayName)"
    }

    $sectionStart = $nsisScript.IndexOf($visibleDeclaration, [StringComparison]::Ordinal)
    $sectionEnd = $nsisScript.IndexOf('SectionEnd', $sectionStart, [StringComparison]::Ordinal)
    if ($sectionEnd -lt 0) {
        throw "Generated installer component has no section end: $($component.DisplayName)"
    }
    $sectionBody = $nsisScript.Substring($sectionStart, $sectionEnd - $sectionStart)
    if (-not $sectionBody.Contains('SectionIn RO', [StringComparison]::Ordinal)) {
        throw "Generated installer component is not read-only: $($component.DisplayName)"
    }
}

$requiredInstallerPaths = @(
    '$COMMONFILES64\VST3'
    '$PROGRAMFILES64\Show Lyrics'
)
foreach ($requiredInstallerPath in $requiredInstallerPaths) {
    if (-not $nsisScript.Contains($requiredInstallerPath, [StringComparison]::Ordinal)) {
        throw "Generated installer does not contain required destination: $requiredInstallerPath"
    }
}

if (-not $nsisScript.Contains('Show Lyrics.vst3', [StringComparison]::Ordinal)) {
    throw 'Generated installer does not contain the Show Lyrics VST3 bundle.'
}
foreach ($legalFileName in $expectedLegalFileNames) {
    if (-not $nsisScript.Contains($legalFileName, [StringComparison]::Ordinal)) {
        throw "Generated installer does not contain required legal file: $legalFileName"
    }
}

$forbiddenUserPaths = @(
    '$APPDATA\VST3 Presets\jfheinrich\Show Lyrics'
    '$DOCUMENTS\VST3 Presets\jfheinrich\Show Lyrics'
    '$LOCALAPPDATA\Programs\Common\VST3\Show Lyrics.vst3'
)
foreach ($forbiddenUserPath in $forbiddenUserPaths) {
    if ($nsisScript.Contains($forbiddenUserPath, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Generated installer must not access a user-scoped path: $forbiddenUserPath"
    }
}

Write-Output 'Windows installer verification passed: VST3 bundle, legal files, metadata, checksum, and system-wide destinations.'
