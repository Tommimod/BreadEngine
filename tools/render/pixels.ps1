<#
.SYNOPSIS
Shared pixel access. Dot-source it; do not run it.

.DESCRIPTION
GetPixel costs a bounds check and a marshalling hop per call, which is minutes rather than
seconds over a 2560x1369 frame. Every script here reads a capture once through LockBits
into a byte array instead, laid out BGRA and bottom-up or top-down as GDI+ chose - the
stride is what says which, so it is carried alongside rather than assumed.
#>

Add-Type -AssemblyName System.Drawing

function Read-Frame
{
    param([Parameter(Mandatory)][string]$Path)

    $bitmap = New-Object System.Drawing.Bitmap($Path)
    $rect = New-Object System.Drawing.Rectangle 0, 0, $bitmap.Width, $bitmap.Height
    $data = $bitmap.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
                             [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bytes = New-Object 'byte[]' ([Math]::Abs($data.Stride) * $bitmap.Height)
    [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $bytes.Length)
    $bitmap.UnlockBits($data)

    $frame = [pscustomobject]@{
        Path   = $Path
        Width  = $bitmap.Width
        Height = $bitmap.Height
        Stride = [Math]::Abs($data.Stride)
        Bytes  = $bytes
    }
    $bitmap.Dispose()
    return $frame
}

# Byte offset of a pixel's blue channel; red is +2 and the three run B, G, R.
function Get-Offset
{
    param($Frame, [int]$X, [int]$Y)
    return $Y * $Frame.Stride + $X * 4
}
