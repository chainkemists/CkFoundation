#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkJolt/CkJolt_ContactEvent.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt_body
{
    // Routes CkJolt's drained contact events into per-JoltBody contact signals (Added/Persisted/Removed).
    // Registered on the contact-router registry by UCk_Jolt_Subsystem::Initialize as "JoltBody.Signals"; runs
    // on the game thread inside FProcessor_JoltWorld_DrainEvents, so ECS reads and signal broadcasts are safe.
    // InTransientEntity is the ECS world's transient entity, used to resolve body UserData to live handles.
    CKJOLT_API auto RouteContactEvents(
        const FCk_Handle& InTransientEntity,
        const TArray<FCk_Jolt_ContactEvent>& InEvents) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
