#pragma once

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle.h"

#include <Templates/SharedPointer.h>

// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable once CppInconsistentNaming
namespace JPH { class PhysicsSystem; }
namespace ck::jolt { class FCk_Jolt_CollisionLayerTable; }

// --------------------------------------------------------------------------------------------------------------------

/**
 * Everything a ProbeTrace needs from the Jolt world, resolved from the entity's registry contexts.
 * Exists so callers (CkEqs, game code) can hold and pass trace state WITHOUT naming a Jolt type: the
 * members below are only ever dereferenced inside CkProbeTrace_Utils.cpp.
 * Cheap to build (two registry-context reads) — build it per tick, never cache across worlds.
 */
struct CKSPATIALQUERY_API FCk_ProbeTrace_Context
{
public:
    // An invalid context is a legitimate state, not an error: worlds without a Jolt subsystem publish
    // neither registry context. Callers that need the trace to run ensure at their own boundary.
    static auto
    Get_ForEntity(
        const FCk_Handle& InHandle) -> FCk_ProbeTrace_Context;

    auto
    Get_IsValid() const -> bool;

private:
    friend class UCk_Utils_ProbeTrace_UE;

    TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    const ck::jolt::FCk_Jolt_CollisionLayerTable* _LayerTable = nullptr;
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProbeTrace_Context, IsValid_Policy_Default,
    [=](const FCk_ProbeTrace_Context& InContext)
    {
        return InContext.Get_IsValid();
    });

// --------------------------------------------------------------------------------------------------------------------
