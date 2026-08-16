// Matrices arrive in the column-major order rlgl uploads its own in, which is what HLSL's
// default cbuffer packing expects - so they are used column-vector style, mul(matrix, vector).

cbuffer FrameConstants
{
    float4x4 g_ViewProjection;
};

cbuffer DrawConstants
{
    float4x4 g_Model;
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
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 UV       : TEX_COORD;
};

void main(in VSInput VSIn, out PSInput PSIn)
{
    float4 worldPosition = mul(g_Model, float4(VSIn.Position, 1.0));

    PSIn.Position = mul(g_ViewProjection, worldPosition);
    // A zero w drops the translation, leaving the rotation and scale a normal is subject to.
    PSIn.Normal   = mul(g_Model, float4(VSIn.Normal, 0.0)).xyz;
    PSIn.UV       = VSIn.UV;
}
