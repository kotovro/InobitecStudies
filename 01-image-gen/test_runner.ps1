param(
    [switch]$Cpp
)

$ErrorActionPreference = 'Stop'

$dir = if ($Cpp) { "cpp" } else { "c" }
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$exe = Join-Path $root "$dir\gen_image.exe"
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if (-not (Test-Path $exe)) {
    Write-Host "FAIL: $exe not found. Build it first." -ForegroundColor Red
    exit 1
}

$passed = 0
$failed = 0

function Run-Test {
    param([string[]]$Arguments, [int]$ExpectedExit, [string]$StdErrMatch, [string]$StdOutMatch, [string]$Name)

    Write-Host -NoNewline "  $Name ... "

    # Build quoted argument string
    $argStr = ""
    foreach ($a in $Arguments) {
        if ($a -match '\s' -or $a -eq '') { $argStr += " `"$a`"" }
        else { $argStr += " $a" }
    }

    $tmpOut = [System.IO.Path]::GetTempFileName()
    $tmpErr = [System.IO.Path]::GetTempFileName()

    # Run with vcvars environment
    $cmdLine = "`"$vcvars`" >nul 2>&1 && `"$exe`"$argStr > `"$tmpOut`" 2> `"$tmpErr`""
    cmd /c $cmdLine
    $actualExit = $LASTEXITCODE

    $stdoutText = ""
    if ((Test-Path $tmpOut) -and ((Get-Item $tmpOut).Length -gt 0)) {
        $stdoutText = (Get-Content $tmpOut -Raw).Trim()
    }
    $stderrText = ""
    if ((Test-Path $tmpErr) -and ((Get-Item $tmpErr).Length -gt 0)) {
        $stderrText = (Get-Content $tmpErr -Raw).Trim()
    }

    Remove-Item -LiteralPath $tmpOut, $tmpErr -ErrorAction SilentlyContinue

    if ($actualExit -ne $ExpectedExit) {
        Write-Host "FAIL (exit: expected $ExpectedExit, got $actualExit)" -ForegroundColor Red
        if ($stderrText) { Write-Host "  stderr: $stderrText" -ForegroundColor Gray }
        $script:failed++
        return
    }

    if ($StdErrMatch -and $stderrText -notmatch $StdErrMatch) {
        Write-Host "FAIL (stderr: expected '$StdErrMatch')" -ForegroundColor Red
        Write-Host "  got: '$stderrText'" -ForegroundColor Gray
        $script:failed++
        return
    }

    if ($StdOutMatch -and $stdoutText -notmatch $StdOutMatch) {
        Write-Host "FAIL (stdout: expected '$StdOutMatch')" -ForegroundColor Red
        Write-Host "  got: '$stdoutText'" -ForegroundColor Gray
        $script:failed++
        return
    }

    Write-Host "PASS" -ForegroundColor Green
    $script:passed++
}

Write-Host "=== Acceptance tests: $dir ===" -ForegroundColor Cyan

# ---- error cases ----
Run-Test -Arguments @()              -ExpectedExit 66 -StdErrMatch "N не указано" -Name "no args"
Run-Test -Arguments @("abc")         -ExpectedExit 64 -StdErrMatch "целым числом"  -Name "abc"
Run-Test -Arguments @("4abc")        -ExpectedExit 64 -StdErrMatch "целым числом"  -Name "4abc"
Run-Test -Arguments @("0")           -ExpectedExit 64 -StdErrMatch "1; 512"        -Name "N=0"
Run-Test -Arguments @("513")         -ExpectedExit 64 -StdErrMatch "1; 512"        -Name "N=513"
Run-Test -Arguments @("-1")          -ExpectedExit 64 -StdErrMatch "1; 512"        -Name "N=-1"
Run-Test -Arguments @("5", "invalid") -ExpectedExit 64 -StdErrMatch "Неизвестный паттерн" -Name "invalid pattern"

# ---- valid cases ----
Run-Test -Arguments @("3")           -ExpectedExit 0 -StdOutMatch "P3\s*3 3\s*255" -Name "3 gradient (default)"
Run-Test -Arguments @("3", "gradient") -ExpectedExit 0 -StdOutMatch "P3\s*3 3\s*255" -Name "3 gradient"
Run-Test -Arguments @("3", "checker")  -ExpectedExit 0 -StdOutMatch "255\s+0\s+0" -Name "3 checker"
Run-Test -Arguments @("3", "radial")   -ExpectedExit 0 -StdOutMatch "P3\s*3 3\s*255" -Name "3 radial"

# ---- edge cases ----
Run-Test -Arguments @("1")           -ExpectedExit 0 -StdOutMatch "P3\s*1 1\s*255" -Name "size=1 gradient"
Run-Test -Arguments @("1", "checker") -ExpectedExit 0 -StdOutMatch "255 255 255"  -Name "size=1 checker"
Run-Test -Arguments @("1", "radial")  -ExpectedExit 0 -StdOutMatch "255\s+0\s+0"  -Name "size=1 radial"
Run-Test -Arguments @("512")         -ExpectedExit 0 -StdOutMatch "P3\s*512 512\s*255" -Name "size=512 gradient"

Write-Host "---" -ForegroundColor Cyan
Write-Host "Passed: $passed, Failed: $failed" -ForegroundColor Cyan

if ($failed -gt 0) {
    Write-Host "SOME TESTS FAILED" -ForegroundColor Red
    exit 1
}
Write-Host "All tests PASSED" -ForegroundColor Green
exit 0
