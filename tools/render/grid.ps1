<#
.SYNOPSIS
A coarse grid of hex samples, to read a frame's layout without looking at it.

.DESCRIPTION
Cheaper than the image by two orders of magnitude and usually enough: it says where the
sky is, where the floor is, whether anything is saturated. Read the image itself only when
you do not yet know what you are looking for.
#>
param(
    [Parameter(Mandatory)][string]$Image,
    [int]$Cols = 16,
    [int]$Rows = 10
)

. (Join-Path $PSScriptRoot 'pixels.ps1')

$frame = Read-Frame $Image
Write-Output ("{0}  {1}x{2}" -f (Split-Path $Image -Leaf), $frame.Width, $frame.Height)
for ($row = 0; $row -lt $Rows; $row++)
{
    $line = ""
    for ($col = 0; $col -lt $Cols; $col++)
    {
        $x = [int](($col + 0.5) * $frame.Width / $Cols)
        $y = [int](($row + 0.5) * $frame.Height / $Rows)
        $offset = Get-Offset $frame $x $y
        $line += ("{0:x2}{1:x2}{2:x2} " -f $frame.Bytes[$offset + 2], $frame.Bytes[$offset + 1], $frame.Bytes[$offset])
    }
    Write-Output $line
}
