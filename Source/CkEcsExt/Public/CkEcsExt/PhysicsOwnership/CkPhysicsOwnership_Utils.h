#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::physics_ownership
{
    /// ENSURES and returns false when the entity is already Chaos-simulated — callers must early-out
    /// with an invalid handle, making cross-engine composition a construction failure, not a warning.
    CKECSEXT_API auto TryClaim_Jolt(
        FCk_Handle& InHandle) -> bool;

    /// Chaos-side twin of TryClaim_Jolt.
    CKECSEXT_API auto TryClaim_Chaos(
        FCk_Handle& InHandle) -> bool;

    /// Releases one claim; the ownership tag disappears when the last same-world feature releases.
    /// EndPlay teardown does NOT need to release (the entity is dying).
    CKECSEXT_API auto Release_Jolt(
        FCk_Handle& InHandle) -> void;

    CKECSEXT_API auto Release_Chaos(
        FCk_Handle& InHandle) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
