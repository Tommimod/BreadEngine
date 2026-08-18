// One triangle large enough to cover the whole target, shared by every fullscreen pass. There
// is no vertex buffer behind it - the corner comes from the vertex index alone, so the pass
// needs no geometry and no input layout.

struct PSInput
{
    float4 Position     : SV_POSITION;
    /// Normalized device XY of this pixel. Turned into a texture coordinate or a view ray by
    /// whoever consumes it: which way v runs is the device's own convention, and
    /// NormalizedDeviceXYToTexUV is what knows it.
    float2 NormalizedXY : NORMALIZED_XY;
};

void main(in uint VertexId : SV_VertexID, out PSInput PSIn)
{
    float2 corners[3];
    corners[0] = float2(-1.0, -1.0);
    corners[1] = float2(-1.0, +3.0);
    corners[2] = float2(+3.0, -1.0);

    float2 corner = corners[VertexId];
    // The far plane, in both depth conventions. Passes with no depth attachment ignore it;
    // the skybox needs it, because drawing at the far plane under a LESS_EQUAL test is what
    // confines it to the pixels no geometry reached.
    PSIn.Position = float4(corner, 1.0, 1.0);
    PSIn.NormalizedXY = corner;
}
