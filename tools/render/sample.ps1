<#
.SYNOPSIS
Named texels, and optionally the same texels across several captures side by side.

.DESCRIPTION
The form a prediction is checked in: compute what a pixel must be, then read it. Several
images at once is what turns a scanline into evidence about how far an effect reaches.
#>
param(
    [Parameter(Mandatory)][string[]]$Images,
    # "x,y;x,y;..." or, with -ScanY, a range walked along one row.
    [string]$Points,
    [int]$ScanY = -1,
    [int]$ScanFrom = 0,
    [int]$ScanTo = 0,
    [int]$ScanStep = 8
)

. (Join-Path $PSScriptRoot 'pixels.ps1')

$frames = @($Images | ForEach-Object { Read-Frame $_ })

$coordinates = @()
if ($ScanY -ge 0)
{
    for ($x = $ScanFrom; $x -le $ScanTo; $x += $ScanStep) { $coordinates += , @($x, $ScanY) }
}
else
{
    foreach ($point in $Points.Split(';')) { $pair = $point.Split(','); $coordinates += , @([int]$pair[0], [int]$pair[1]) }
}

$header = "     x     y"
foreach ($frame in $frames) { $header += ("  {0,-18}" -f (Split-Path $frame.Path -Leaf)) }
Write-Output $header

foreach ($coordinate in $coordinates)
{
    $line = "{0,6}{1,6}" -f $coordinate[0], $coordinate[1]
    foreach ($frame in $frames)
    {
        $offset = Get-Offset $frame $coordinate[0] $coordinate[1]
        $line += ("  {0,3} {1,3} {2,3}  #{3:x2}{4:x2}{5:x2}" -f
                  $frame.Bytes[$offset + 2], $frame.Bytes[$offset + 1], $frame.Bytes[$offset],
                  $frame.Bytes[$offset + 2], $frame.Bytes[$offset + 1], $frame.Bytes[$offset])
    }
    Write-Output $line
}
