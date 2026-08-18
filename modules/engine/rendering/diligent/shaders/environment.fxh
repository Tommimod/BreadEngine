#ifndef _ENVIRONMENT_FXH_
#define _ENVIRONMENT_FXH_

/// Turns a world direction into the environment cube's own space. The inspector's rotation
/// turns the sky and this turns the direction it is sampled with, so the quaternion handed
/// over is already the inverse of the authored one. Shared because the background pass and the
/// scene pass sample the same cube: if these two ever disagreed, a rotated sky would reflect
/// off surfaces somewhere other than where it is drawn.
float3 toEnvironmentSpace(float3 direction, float4 rotation)
{
    return direction + 2.0 * cross(rotation.xyz, cross(rotation.xyz, direction) + rotation.w * direction);
}

#endif // _ENVIRONMENT_FXH_
