// Depth-only pass that fills one shadow cascade. There is no pixel shader: the cascade has no
// colour attachment and the depth the rasterizer writes is the whole output.

cbuffer ShadowPassConstants
{
    /// World to the light's clip space, for the cascade currently being filled.
    float4x4 g_WorldToLightProj;
};

// The scene pass's own per-draw block, reused as-is so one buffer feeds both pipelines. Only
// the model matrix is read here; the second member is declared to keep the layouts identical.
cbuffer DrawConstants
{
    float4x4 g_Model;
    float4x4 g_NormalMatrix;
};

struct VSInput
{
    float3 Position : ATTRIB0;
};

struct PSInput
{
    float4 Position : SV_POSITION;
};

void main(in VSInput VSIn, out PSInput PSIn)
{
    PSIn.Position = mul(g_WorldToLightProj, mul(g_Model, float4(VSIn.Position, 1.0)));
}
