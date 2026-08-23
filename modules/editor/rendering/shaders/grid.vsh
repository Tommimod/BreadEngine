#include "overlay.fxh"

// The grid has no geometry of its own: it is one triangle covering the viewport, and the
// surface it draws is found per pixel by intersecting the view ray with the ground plane. The
// corners arrive already in normalized device space, so this stage only hands them on.

struct PSInput
{
    float4 Position     : SV_POSITION;
    float2 NormalizedXY : NORMALIZED_XY;
};

void main(in OverlayVSInput VSIn, out PSInput PSIn)
{
    PSIn.Position = float4(VSIn.Position, 1.0);
    PSIn.NormalizedXY = VSIn.Position.xy;
}
