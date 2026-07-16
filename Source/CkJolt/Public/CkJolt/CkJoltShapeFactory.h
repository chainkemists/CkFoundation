#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/Query/CkJoltQuery_Data.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    /// Builds a Jolt shape from CkJolt-owned dimensions. Capsules/cylinders come out UPRIGHT
    /// (Z-axis aligned) — the Y->Z axis correction is applied internally, matching the Probe
    /// shape factories and UE's FKSphylElem convention. Used by scene queries (Phase 2) and
    /// JoltBody explicit-shape setup (Phase 3). Returns null (after CK_ENSURE) on failure.
    CKJOLT_API auto CreateShape_FromDimensions(
        const FCk_Jolt_ShapeDimensions& InDimensions,
        const FString& InDebugName) -> JPH::Ref<JPH::Shape>;
}

// --------------------------------------------------------------------------------------------------------------------
