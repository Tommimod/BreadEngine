<#
.SYNOPSIS
Launches a build, waits for it to settle, and saves its client area as a PNG.

.DESCRIPTION
The wait is the whole contract. The renderer decodes environment images on a worker
thread and the ambient precompute waits for the sky's inputs to settle, so a capture
taken too early shows a frame that is correct but not finished. Twelve seconds clears
both on the test scene; raise it for a heavier one rather than lowering it for speed.

Captures of one config are bit-identical between runs, which is what makes diff.ps1 a
refactor gate rather than an approximation.
#>
param(
    [Parameter(Mandatory)][string]$Exe,
    [Parameter(Mandatory)][string]$Out,
    [int]$WaitSeconds = 14
)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class BreadCaptureWin {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
"@

$proc = Start-Process -FilePath $Exe -WorkingDirectory (Split-Path $Exe) -PassThru
try {
    Start-Sleep -Seconds $WaitSeconds
    $proc.Refresh()

    # Compared against zero *and* null: a process that died before opening a window leaves
    # MainWindowHandle empty rather than IntPtr.Zero, and the P/Invoke below then throws a
    # conversion error instead of saying what actually happened.
    $handle = $proc.MainWindowHandle
    if ($null -eq $handle -or $handle -eq [IntPtr]::Zero) {
        Write-Error "No window after $WaitSeconds s - the build most likely failed to start. Run it with runlog.ps1 to see why."
        return
    }

    [void][BreadCaptureWin]::SetForegroundWindow($handle)
    Start-Sleep -Seconds 2

    $rect = New-Object BreadCaptureWin+RECT
    [void][BreadCaptureWin]::GetClientRect($handle, [ref]$rect)
    $origin = New-Object BreadCaptureWin+POINT
    [void][BreadCaptureWin]::ClientToScreen($handle, [ref]$origin)

    $width = $rect.R - $rect.L
    $height = $rect.B - $rect.T
    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.CopyFromScreen($origin.X, $origin.Y, 0, 0, $bitmap.Size)
    $bitmap.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
    $graphics.Dispose(); $bitmap.Dispose()

    Write-Output "$Out  ${width}x${height}"
}
finally {
    if (-not $proc.HasExited) { $proc.Kill() }
}
