#pragma once

namespace BreadEngine {
    /// Serialized as its underlying value by MeshRenderer's primitive data blob, so values
    /// may be appended and removed from the tail, never reordered or removed from the middle.
    enum class MeshPrimitiveType
    {
        None = 0,
        Cube,
        Sphere,
        HalfSphere,
        Cylinder,
        Capsule,
        Plane,
        Quad
    };
}
