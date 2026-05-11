param(
    [Parameter(Mandatory = $true)]
    [string[]] $FilePath,

    [string] $CertificateBase64 = $env:WINDOWS_SIGNING_CERT_BASE64,

    [AllowNull()]
    [System.Security.SecureString] $CertificatePassword,

    [string] $TimestampUrl = 'http://timestamp.digicert.com'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $PSBoundParameters.ContainsKey('CertificatePassword') -and
    -not [string]::IsNullOrEmpty($env:WINDOWS_SIGNING_CERT_PASSWORD)) {
    $CertificatePassword = ConvertTo-SecureString -String $env:WINDOWS_SIGNING_CERT_PASSWORD -AsPlainText -Force
}

function Get-PlainTextFromSecureString {
    param(
        [Parameter(Mandatory = $true)]
        [System.Security.SecureString] $SecureString
    )

    $bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($SecureString)
    try {
        return [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr)
    }
    finally {
        if ($bstr -ne [IntPtr]::Zero) {
            [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
        }
    }
}

function Resolve-SignToolPath {
    $command = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $roots = @(
        (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'),
        (Join-Path $env:ProgramFiles 'Windows Kits\10\bin')
    ) | Where-Object { $_ -and (Test-Path $_) }

    $signToolCandidates = foreach ($root in $roots) {
        Get-ChildItem -Path $root -Filter signtool.exe -Recurse -File -ErrorAction SilentlyContinue
    }

    $preferred = $signToolCandidates |
    Where-Object { $_.FullName -match '\\x64\\signtool\.exe$' } |
    Sort-Object FullName -Descending |
    Select-Object -First 1
    if ($preferred) {
        return $preferred.FullName
    }

    $fallback = $signToolCandidates | Sort-Object FullName -Descending | Select-Object -First 1
    if ($fallback) {
        return $fallback.FullName
    }

    throw 'Unable to locate signtool.exe on the runner'
}

if ([string]::IsNullOrWhiteSpace($CertificateBase64)) {
    Write-Host 'No Windows signing certificate configured; skipping code signing.'
    exit 0
}

$signTool = Resolve-SignToolPath
$certificatePath = Join-Path $env:RUNNER_TEMP 'spring2-signing-cert.pfx'
$certificatePasswordPlainText = $null

try {
    [System.IO.File]::WriteAllBytes(
        $certificatePath,
        [System.Convert]::FromBase64String($CertificateBase64))

    if ($null -ne $CertificatePassword) {
        $certificatePasswordPlainText = Get-PlainTextFromSecureString -SecureString $CertificatePassword
    }

    foreach ($path in $FilePath) {
        $resolvedPath = Resolve-Path -Path $path -ErrorAction Stop
        $arguments = @(
            'sign',
            '/fd', 'SHA256',
            '/td', 'SHA256',
            '/tr', $TimestampUrl,
            '/f', $certificatePath
        )

        if ($null -ne $certificatePasswordPlainText) {
            $arguments += @('/p', $certificatePasswordPlainText)
        }

        $arguments += $resolvedPath.Path
        & $signTool @arguments
        if ($LASTEXITCODE -ne 0) {
            throw "signtool.exe failed while signing $($resolvedPath.Path)"
        }
    }
}
finally {
    $certificatePasswordPlainText = $null
    if (Test-Path $certificatePath) {
        Remove-Item -Path $certificatePath -Force
    }
}