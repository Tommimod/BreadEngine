#ifndef _OVERLAY_FXH_
#define _OVERLAY_FXH_

/// What the renderer offers every overlay effect, and the whole of what it knows about one.
/// Declared here rather than in each effect so the block the pass fills and the block a shader
/// reads are one declaration: a client ships its own sources, but not its own idea of where
/// the camera is.
///
/// A stage that reads none of this may leave the block out entirely - the pass binds it only
/// where the compiled shader still has it.
cbuffer OverlayFrameConstants
{
    float4x4 g_OverlayViewProjection;
    /// Takes a point in normalized device space back into the world, which is what turns a
    /// pixel into the view ray through it.
    float4x4 g_OverlayInverseViewProjection;
    /// xyz is the eye the frame was drawn from; w is unused.
    float4 g_OverlayCameraPosition;
};

/// The layout every overlay mesh arrives in. What a position means - a point in the world, or
/// a pixel of the target - is the effect's own decision, and so is whether it reads the colour
/// or the texture coordinate at all.
struct OverlayVSInput
{
    float3 Position : ATTRIB0;
    float2 UV       : ATTRIB1;
    float4 Color    : ATTRIB2;
};

#endif // _OVERLAY_FXH_
