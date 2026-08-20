#ifndef _SCENE_TARGETS_FXH_
#define _SCENE_TARGETS_FXH_

/// What the scene pass leaves on screen, and how a later pass reads it back.
///
/// The surface target packs five numbers into four channels: the shading normal in xy, the
/// roughness in z, and in w the whole scalar the ambient specular was scaled by. That is why
/// the normal is octahedral rather than stored outright - and storing the finished weight
/// rather than the metalness it came from is what lets the reflection pass remove exactly what
/// the scene pass added, occlusion map and ambient energy included, without a BRDF table or a
/// Fresnel term of its own.
///
/// Both halves are contracts between the pass that writes and the passes that read: an encode
/// and a decode that disagree is a normal pointing somewhere plausible, and a reconstruction
/// that disagrees with the projection it came from is a position that moves when the camera
/// turns.

/// Sign of each component, counting zero as positive - which is what keeps the two folds below
/// exact inverses of each other. Written per component because a vector ?: is HLSL only and
/// has no GLSL counterpart.
float2 sceneSignNotZero(float2 v)
{
    return float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

/// Folds the lower hemisphere of the octahedron out into the corners of the square, which is
/// the step that makes the mapping continuous and its own inverse.
float2 sceneOctahedronFold(float2 v)
{
    return (1.0 - abs(float2(v.y, v.x))) * sceneSignNotZero(v);
}

/// A unit normal as two numbers in [-1, 1]. The octahedral projection spreads the error evenly
/// over the sphere, so at this target's precision the round trip is far below a degree.
float2 encodeSurfaceNormal(float3 normal)
{
    float3 projected = normal / (abs(normal.x) + abs(normal.y) + abs(normal.z));
    if (projected.z < 0.0) return sceneOctahedronFold(projected.xy);
    return projected.xy;
}

float3 decodeSurfaceNormal(float2 encoded)
{
    float3 normal = float3(encoded.x, encoded.y, 1.0 - abs(encoded.x) - abs(encoded.y));
    if (normal.z < 0.0) normal.xy = sceneOctahedronFold(normal.xy);
    return normalize(normal);
}

/// The world position a depth texel was written from. The matrix is a parameter rather than a
/// constant of its own because each pass keeps its own copy of it, and the reconstruction is
/// the same one for all of them.
float3 worldPositionAt(float4x4 inverseViewProjection, float2 normalizedXY, float depth)
{
    float4 homogeneous = mul(inverseViewProjection,
                             float4(normalizedXY, DepthToNormalizedDeviceZ(depth), 1.0));
    return homogeneous.xyz / homogeneous.w;
}

#endif // _SCENE_TARGETS_FXH_
