#pragma once

#include "CkCore/Macros/CkMacros.h"

// Re-export, not a use: consumers reach the whole of ck::jolt::* through this header.
#include "CkJolt/CkJolt_Utils.h"

#include "CkShapes/Box/CkShapeBox_Fragment_Data.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment_Data.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment_Data.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment_Data.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Body/MotionQuality.h>
#include <Jolt/Physics/Collision/BackFaceMode.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    // ----------------------------------------------------------------------------------------------------------------
    // Probe Body User Data
    // ----------------------------------------------------------------------------------------------------------------

    CKSPATIALQUERY_API auto Get_ProbeBodyUserData(
        const JPH::Body& InBody) -> uint64;

    CKSPATIALQUERY_API auto Get_ProbeBodyUserData(
        const JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InBodyId) -> uint64;

    /// Resolves the Probe entity a Jolt query hit refers to (via the body's UserData).
    /// Returns an INVALID handle when the hit is InSelf itself, when the body's entity is no
    /// longer alive (a body can briefly outlive its owning entity — deferred end-of-frame
    /// destroy, or a snapshot restore that wipes the registry before physics tears the body
    /// down), or when the entity is not a Probe. Callers treat all three as "skip this hit".
    CKSPATIALQUERY_API auto TryGet_ProbeFromBodyHit(
        const FCk_Handle& InSelf,
        const JPH::BodyInterface& InBodyInterface,
        JPH::BodyID InHitBodyId) -> FCk_Handle_Probe;
}

// --------------------------------------------------------------------------------------------------------------------