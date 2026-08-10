#include "CkProbeTrace_Context.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Validation/CkIsValid_Defaults.h"

#include "CkEcs/Registry/CkRegistry.h"

#include "CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ProbeTrace_Context::
    Get_ForEntity(
        const FCk_Handle& InHandle)
    -> FCk_ProbeTrace_Context
{
    auto Context = FCk_ProbeTrace_Context{};

    if (ck::Is_NOT_Valid(InHandle))
    { return Context; }

    const auto Registry = InHandle.Get_RegistryView();

    const auto* PhysicsSystemCtx = Registry.TryGetContext<TWeakPtr<JPH::PhysicsSystem>>();
    const auto* LayerCtx = Registry.TryGetContext<ck::jolt::FCk_Jolt_LayerContext>();

    if (PhysicsSystemCtx == nullptr || LayerCtx == nullptr)
    { return Context; }

    Context._PhysicsSystem = *PhysicsSystemCtx;
    Context._LayerTable = LayerCtx->_Table;

    return Context;
}

auto
    FCk_ProbeTrace_Context::
    Get_IsValid() const
    -> bool
{
    return ck::IsValid(_PhysicsSystem.Pin()) && _LayerTable != nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
