param([switch]$Cpp)

$ErrorActionPreference = 'Stop'

$dir = if ($Cpp) { 'cpp' } else { 'c' }
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$exe = Join-Path $root "$dir\read_passport.exe"
$vcvars = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'

if (-not (Test-Path $exe)) {
    Write-Host 'FAIL: exe not found. Build it first.' -ForegroundColor Red
    exit 1
}

$cp = [System.Text.Encoding]::GetEncoding(1251)
$passed = 0
$failed = 0

function Run-Test {
    param([string]$InputText,[int]$ExpectedExit,[string]$StdOutContains,[string]$StdErrContains,[string]$Name)

    Write-Host -NoNewline "  $Name ... "

    $inpf = [System.IO.Path]::GetTempFileName()
    $outf = [System.IO.Path]::GetTempFileName()
    $errf = [System.IO.Path]::GetTempFileName()

    [System.IO.File]::WriteAllBytes($inpf, $cp.GetBytes($InputText))

    cmd /c "`"$vcvars`" >nul 2>&1 && `"$exe`" < `"$inpf`" > `"$outf`" 2> `"$errf`""
    $actualExit = $LASTEXITCODE

    $stdoutBytes = [System.IO.File]::ReadAllBytes($outf)
    $stderrBytes = [System.IO.File]::ReadAllBytes($errf)

    Remove-Item -LiteralPath $inpf, $outf, $errf -ErrorAction SilentlyContinue

    if ($actualExit -ne $ExpectedExit) {
        Write-Host "FAIL (exit: expected $ExpectedExit, got $actualExit)" -ForegroundColor Red
        if ($stderrBytes.Length -gt 0) { Write-Host "  stderr: $($cp.GetString($stderrBytes))" -ForegroundColor Gray }
        $script:failed++
        return
    }

    if ($StdErrContains) {
        $stderrText = $cp.GetString($stderrBytes)
        if ($stderrText.IndexOf($StdErrContains) -lt 0) {
            Write-Host "FAIL (stderr: missing '$StdErrContains')" -ForegroundColor Red
            Write-Host "  got: '$stderrText'" -ForegroundColor Gray
            $script:failed++
            return
        }
    }

    if ($StdOutContains) {
        $stdoutText = $cp.GetString($stdoutBytes)
        if ($stdoutText.IndexOf($StdOutContains) -lt 0) {
            Write-Host "FAIL (stdout: missing '$StdOutContains')" -ForegroundColor Red
            Write-Host "  got: '$stdoutText'" -ForegroundColor Gray
            $script:failed++
            return
        }
    }

    Write-Host 'PASS' -ForegroundColor Green
    $script:passed++
}

Write-Host "=== Acceptance tests: $dir ===" -ForegroundColor Cyan

# ---- valid cases ----
Run-Test -InputText "морской закат`r`n1920`r`n" -ExpectedExit 0 -StdOutContains "Изображение «морской закат»: 1920 пикселей." -Name "multi-word + 1920"
Run-Test -InputText "фон`r`n1`r`n" -ExpectedExit 0 -StdOutContains "1 пиксель" -Name "single-word + 1"
Run-Test -InputText "дом`r`n2`r`n" -ExpectedExit 0 -StdOutContains "2 пикселя" -Name "дом + 2"
Run-Test -InputText "этюд`r`n3`r`n" -ExpectedExit 0 -StdOutContains "3 пикселя" -Name "этюд + 3"
Run-Test -InputText "полотно`r`n4`r`n" -ExpectedExit 0 -StdOutContains "4 пикселя" -Name "полотно + 4"
Run-Test -InputText "картина`r`n5`r`n" -ExpectedExit 0 -StdOutContains "5 пикселей" -Name "картина + 5"
Run-Test -InputText "картина`r`n10`r`n" -ExpectedExit 0 -StdOutContains "10 пикселей" -Name "+ 10"
Run-Test -InputText "работа`r`n11`r`n" -ExpectedExit 0 -StdOutContains "11 пикселей" -Name "+ 11"
Run-Test -InputText "этюд`r`n12`r`n" -ExpectedExit 0 -StdOutContains "12 пикселей" -Name "+ 12"
Run-Test -InputText "этюд`r`n13`r`n" -ExpectedExit 0 -StdOutContains "13 пикселей" -Name "+ 13"
Run-Test -InputText "этюд`r`n14`r`n" -ExpectedExit 0 -StdOutContains "14 пикселей" -Name "+ 14"
Run-Test -InputText "этюд`r`n20`r`n" -ExpectedExit 0 -StdOutContains "20 пикселей" -Name "+ 20"
Run-Test -InputText "этюд`r`n21`r`n" -ExpectedExit 0 -StdOutContains "21 пиксель" -Name "+ 21"
Run-Test -InputText "этюд`r`n22`r`n" -ExpectedExit 0 -StdOutContains "22 пикселя" -Name "+ 22"
Run-Test -InputText "этюд`r`n100`r`n" -ExpectedExit 0 -StdOutContains "100 пикселей" -Name "+ 100"
Run-Test -InputText "этюд`r`n101`r`n" -ExpectedExit 0 -StdOutContains "101 пиксель" -Name "+ 101"
Run-Test -InputText "этюд`r`n111`r`n" -ExpectedExit 0 -StdOutContains "111 пикселей" -Name "+ 111"

# ---- error cases ----
Run-Test -InputText "`r`n1920`r`n" -ExpectedExit 65 -StdErrContains "не может быть пустым" -Name "empty name"
Run-Test -InputText "" -ExpectedExit 66 -StdErrContains "нет входных данных" -Name "EOF (no input)"
Run-Test -InputText "тест`r`nabc`r`n" -ExpectedExit 65 -StdErrContains "целым числом" -Name "bad count (abc)"
Run-Test -InputText "тест`r`n-5`r`n" -ExpectedExit 65 -StdErrContains "положительным" -Name "negative count (-5)"
Run-Test -InputText "тест`r`n0`r`n" -ExpectedExit 65 -StdErrContains "положительным" -Name "zero count"

Write-Host "---" -ForegroundColor Cyan
Write-Host "Passed: $passed, Failed: $failed" -ForegroundColor Cyan

if ($failed -gt 0) {
    Write-Host 'SOME TESTS FAILED' -ForegroundColor Red
    exit 1
}
Write-Host 'All tests PASSED' -ForegroundColor Green
exit 0
