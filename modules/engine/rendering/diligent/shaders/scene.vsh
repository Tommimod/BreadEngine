// Matrices arrive in the column-major order rlgl uploads its own in, which is what HLSL's
// default cbuffer packing expects - so they are used column-vector style, mul(matrix, vector).

// Both stages are bound to the same buffer, so this block has to be declared identically in
// the pixel shader even though only the first member is read here.
cbuffer FrameConstants
{
    float4x4 g_ViewProjection;
    float4   g_CameraPosition;
    float4   g_CameraForward;
    float4   g_AmbientColor;
    float4   g_OutputEncoding;
};

cbuffer DrawConstants
{
    float4x4 g_Model;
    // Inverse transpose of g_Model: the only matrix that carries normals through a
    // non-uniform scale without shearing them off the surface.
    float4x4 g_NormalMatrix;
};

struct VSInput
{
    float3 Position : ATTRIB0;
    float3 Normal   : ATTRIB1;
    float2 UV       : ATTRIB2;
    float3 Tangent  : ATTRIB3;
};

struct PSInput
{
    float4 Position  : SV_POSITION;
    float3 WorldPos  : WORLD_POS;
    float3 Normal    : NORMAL;
    float3 Tangent   : TANGENT;
    float2 UV        : TEX_COORD;
};

void main(in VSInput VSIn, out PSInput PSIn)
{
    // A zero w drops the translation, leaving only what a direction is subject to.
    float4 worldPosition = mul(g_Model, float4(VSIn.Position, 1.0));

    PSIn.Position = mul(g_ViewProjection, worldPosition);
    PSIn.WorldPos = worldPosition.xyz;
    PSIn.Normal   = mul(g_NormalMatrix, float4(VSIn.Normal, 0.0)).xyz;
    // Tangents ride the surface, so they follow the model matrix rather than its inverse
    // transpose - re-orthogonalized against the normal in the pixel shader.
    PSIn.Tangent  = mul(g_Model, float4(VSIn.Tangent, 0.0)).xyz;
    PSIn.UV       = VSIn.UV;
}
