#include "overlay.fxh"

// Every handle of the gizmo is one of a handful of unit shapes, placed by the matrix in this
// block and painted by the colour beside it - so an arrow, a ring and a plane reach the GPU
// through the same pipeline and differ only in what was uploaded for them.

cbuffer OverlayParameters
{
    float4x4 g_GizmoModel;
    float4 g_GizmoColor;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
};

void main(in OverlayVSInput VSIn, out PSInput PSIn)
{
    float4 worldPosition = mul(g_GizmoModel, float4(VSIn.Position, 1.0));
    PSIn.Position = mul(g_OverlayViewProjection, worldPosition);
    // The vertex colour modulates rather than replaces: it is how one mesh carries a
    // translucent face and an opaque one, while the handle's own colour stays a parameter.
    PSIn.Color = g_GizmoColor * VSIn.Color;
}
