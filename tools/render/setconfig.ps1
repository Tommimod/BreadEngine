<#
.SYNOPSIS
Rewrites one scalar under one top-level block of a config, for A/B captures.

.DESCRIPTION
Turning an effect on and off between two captures is the whole verification loop, and
doing it through the editor by hand is what makes the loop slow.

Edits the copy under bin/, which is the one a build actually reads. The build only copies
what is absent and never overwrites, so bin/ is where experiments belong - and why
anything worth keeping has to be copied back out to games/<name>/assets/ by hand.
#>
param(
    [Parameter(Mandatory)][string]$Path,
    [Parameter(Mandatory)][string]$Block,
    [Parameter(Mandatory)][string]$Key,
    [Parameter(Mandatory)][string]$Value
)

$lines = Get-Content $Path
$inBlock = $false
$hit = $false
for ($i = 0; $i -lt $lines.Count; $i++)
{
    if ($lines[$i] -match '^_[A-Za-z]+:')
    {
        $inBlock = $lines[$i] -match ("^" + [regex]::Escape($Block) + ":")
        continue
    }
    if ($inBlock -and $lines[$i] -match ("^  " + [regex]::Escape($Key) + ": "))
    {
        $lines[$i] = "  $Key`: $Value"
        $hit = $true
    }
}

if (-not $hit) { Write-Error "no '$Key' under '$Block' in $Path"; return }
Set-Content -Path $Path -Value $lines -Encoding utf8
