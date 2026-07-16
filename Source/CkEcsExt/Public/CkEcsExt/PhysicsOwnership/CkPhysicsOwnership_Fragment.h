#pragma once

#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // An entity is EITHER Chaos-simulated (CkOverlapBody's UShapeComponent path, CkRaySense's
    // engine-trace path) OR Jolt-simulated (CkSpatialQuery's Probe, CkJolt's bodies/characters) —
    // never both. The two engines maintain independent, non-interacting world representations;
    // composing both onto one entity is a design error surfaced at COMPOSITION time via
    // physics_ownership::TryClaim_* (CK_ENSURE at the composing call site), not a runtime log.
    //
    // COUNTED: multiple same-world features may stack on one entity (e.g. a Probe and a JoltBody
    // both claim Jolt; two Sensors both claim Chaos) — the conflict is only ever cross-world.
    CK_DEFINE_ECS_TAG_COUNTED(FTag_PhysicsOwnership_Chaos);
    CK_DEFINE_ECS_TAG_COUNTED(FTag_PhysicsOwnership_Jolt);
}

// --------------------------------------------------------------------------------------------------------------------
