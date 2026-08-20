<#
.SYNOPSIS
Exact image equality between two captures. The refactor gate.

.DESCRIPTION
Two captures of one config are bit-identical, so a change that is meant to alter nothing
must diff to zero pixels. That turns "did the refactor preserve behaviour" from an argument
into a measurement. A non-zero count is a regression until explained, and MaxDelta says
whether it is a real difference or a single stray pixel.

Use flatratio.ps1 instead when the change is *meant* to alter the image.
#>
param(
    [Parameter(Mandatory)][string]$A,
    [Parameter(Mandatory)][string]$B,
    [int]$ReportFirst = 8
)

. (Join-Path $PSScriptRoot 'pixels.ps1')

$first = Read-Frame $A
$second = Read-Frame $B

if ($first.Width -ne $second.Width -or $first.Height -ne $second.Height)
{
    Write-Output ("SIZE MISMATCH  {0}x{1} vs {2}x{3}" -f $first.Width, $first.Height, $second.Width, $second.Height)
    return
}

$differing = 0
$maxDelta = 0
$reported = 0
for ($y = 0; $y -lt $first.Height; $y++)
{
    $row = $y * $first.Stride
    for ($x = 0; $x -lt $first.Width; $x++)
    {
        $offset = $row + $x * 4
        $db = [Math]::Abs($first.Bytes[$offset] - $second.Bytes[$offset])
        $dg = [Math]::Abs($first.Bytes[$offset + 1] - $second.Bytes[$offset + 1])
        $dr = [Math]::Abs([int]$first.Bytes[$offset + 2] - [int]$second.Bytes[$offset + 2])
        $delta = [Math]::Max($dr, [Math]::Max($dg, $db))
        if ($delta -eq 0) { continue }

        $differing++
        if ($delta -gt $maxDelta) { $maxDelta = $delta }
        if ($reported -lt $ReportFirst)
        {
            $reported++
            Write-Output ("  {0,5},{1,5}  {2,3} {3,3} {4,3}  ->  {5,3} {6,3} {7,3}" -f $x, $y,
                          $first.Bytes[$offset + 2], $first.Bytes[$offset + 1], $first.Bytes[$offset],
                          $second.Bytes[$offset + 2], $second.Bytes[$offset + 1], $second.Bytes[$offset])
        }
    }
}

$total = $first.Width * $first.Height
Write-Output ("differing {0} of {1} pixels ({2:N4}%)   maxDelta {3}" -f $differing, $total, (100.0 * $differing / $total), $maxDelta)
if ($differing -eq 0) { Write-Output "IDENTICAL" }
