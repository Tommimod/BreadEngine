<#
.SYNOPSIS
How much a post-effect scaled the image, measured where its kernel cannot have changed the answer.

.DESCRIPTION
A post-effect that blurs cannot be checked pixel by pixel against a prediction, because the
prediction would have to model the kernel. Over a region flat enough that every tap reads
the same value, the kernel drops out and only the effect's own scale factor is left - which
is usually a closed-form number.

Written for 7.e's bloom: a one-level chain with threshold zero must scale a flat region by
exactly (1 + intensity) in linear, so 1.25 becomes 1.25^(1/2.2) = 1.1068 encoded. It reads
back unchanged for anything else whose strength is a scalar - which is what SSAO and SSR
will need.

Two things it must do, both learned the hard way. Channels already at 255 in the result are
dropped: a clipped channel reports a ratio of 1.0 and drags the mean toward "no effect".
And flatness is judged on the *baseline*, so a region the effect itself made flat cannot
qualify.
#>
param(
    [Parameter(Mandatory)][string]$Base,
    [Parameter(Mandatory)][string]$Effect,
    # Half-width of the flatness window. Must exceed the effect's reach, or the kernel is
    # still in the answer: a long bloom chain needs a bigger window than a short one.
    [int]$Half = 8,
    [int]$Tolerance = 1,
    [int]$Stride = 7,
    [int]$FloorValue = 20,
    [double]$Expected = 0
)

. (Join-Path $PSScriptRoot 'pixels.ps1')

$base = Read-Frame $Base
$effect = Read-Frame $Effect
if ($base.Width -ne $effect.Width -or $base.Height -ne $effect.Height) { Write-Error "size mismatch"; return }

$ratios = New-Object System.Collections.ArrayList
$flat = 0

for ($y = $Half; $y -lt $base.Height - $Half; $y += $Stride)
{
    for ($x = $Half; $x -lt $base.Width - $Half; $x += $Stride)
    {
        $centre = Get-Offset $base $x $y
        $b0 = $base.Bytes[$centre]; $g0 = $base.Bytes[$centre + 1]; $r0 = $base.Bytes[$centre + 2]

        $isFlat = $true
        for ($dy = -$Half; $dy -le $Half -and $isFlat; $dy += 4)
        {
            for ($dx = -$Half; $dx -le $Half -and $isFlat; $dx += 4)
            {
                $neighbour = Get-Offset $base ($x + $dx) ($y + $dy)
                if ([Math]::Abs([int]$base.Bytes[$neighbour] - $b0) -gt $Tolerance -or
                    [Math]::Abs([int]$base.Bytes[$neighbour + 1] - $g0) -gt $Tolerance -or
                    [Math]::Abs([int]$base.Bytes[$neighbour + 2] - $r0) -gt $Tolerance) { $isFlat = $false }
            }
        }
        if (-not $isFlat) { continue }
        $flat++

        $after = Get-Offset $effect $x $y
        $b1 = $effect.Bytes[$after]; $g1 = $effect.Bytes[$after + 1]; $r1 = $effect.Bytes[$after + 2]
        if ($r0 -ge $FloorValue -and $r1 -lt 255) { [void]$ratios.Add($r1 / $r0) }
        if ($g0 -ge $FloorValue -and $g1 -lt 255) { [void]$ratios.Add($g1 / $g0) }
        if ($b0 -ge $FloorValue -and $b1 -lt 255) { [void]$ratios.Add($b1 / $b0) }
    }
}

if ($ratios.Count -eq 0) { Write-Output "no usable samples - loosen -Tolerance or lower -Half"; return }

$stats = $ratios | Measure-Object -Average -Minimum -Maximum
Write-Output ("flat regions {0}   channels {1}" -f $flat, $ratios.Count)
Write-Output ("ratio  mean {0:N4}   min {1:N4}   max {2:N4}" -f $stats.Average, $stats.Minimum, $stats.Maximum)
if ($Expected -gt 0)
{
    # Quantization alone reaches a few percent at the dark end - 1/20 is 5% - so the mean is
    # what carries the verdict and the spread is expected to be wider than it.
    Write-Output ("expected {0:N4}   error {1:P2}" -f $Expected, [Math]::Abs($stats.Average - $Expected) / $Expected)
}
