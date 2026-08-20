<#
.SYNOPSIS
Runs a build with its output captured, then prints the lines that matter.

.DESCRIPTION
Neither stream reaches the console on its own, and the renderer reports every failure it
survives - a shader that would not compile, a pipeline that would not create - rather than
dying. A silent run and a broken run look identical without this.
#>
param(
    [Parameter(Mandatory)][string]$Exe,
    [int]$WaitSeconds = 14,
    [string]$Pattern = 'rror|arning|ailed'
)

$stdout = Join-Path $env:TEMP 'bread-run.out'
$stderr = Join-Path $env:TEMP 'bread-run.err'
$proc = Start-Process -FilePath $Exe -WorkingDirectory (Split-Path $Exe) -PassThru `
                      -RedirectStandardOutput $stdout -RedirectStandardError $stderr
Start-Sleep -Seconds $WaitSeconds
if (-not $proc.HasExited) { $proc.Kill() }
Start-Sleep -Seconds 1

Write-Output "--- stderr ---"
Get-Content $stderr -ErrorAction SilentlyContinue | Select-Object -Last 40
Write-Output "--- stdout matching '$Pattern' ---"
Get-Content $stdout -ErrorAction SilentlyContinue | Select-String -Pattern $Pattern | Select-Object -Last 40
