#include "CkJoltStaticActor_Processor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h"
#include "CkJolt/World/CkJoltWorld.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_JoltStaticActor_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_static_actor_processor
{
    // Resolves the registry's Jolt-world context. A world with no Jolt subsystem never publishes it — an
    // absent/null context is legal, so the async guard simply does nothing.
    auto
        TryResolve_JoltWorld(
            const FCk_Handle& InTransientEntity)
        -> ck::FJoltWorld*
    {
        const auto* WorldPtr = InTransientEntity.Get_RegistryView().TryGetContext<TSharedPtr<ck::FJoltWorld>>();
        if (WorldPtr == nullptr || NOT WorldPtr->IsValid())
        { return nullptr; }

        return WorldPtr->Get();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_JoltStaticActor_EndPlay::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        // Resolve the static-world subsystem once per tick. A world tearing down may already have destroyed it
        // — a null subsystem makes every entity below a correct silent skip.
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);
        _StaticWorldSubsystem = ck::IsValid(World)
            ? World->GetSubsystem<UCk_JoltStaticWorld_Subsystem_UE>()
            : nullptr;

        // ASYNC GUARD (house rule for Jolt *_EndPlay processors): FGroup_EndPlay runs later in the SAME tick
        // that kicked this frame's async step, and the removal funnel below mutates the broadphase the in-flight
        // Update is reading. Wait the step out first; self-guarded on future validity, so this is free in sync mode.
        auto* JoltWorld = ck_jolt_static_actor_processor::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld != nullptr && JoltWorld->Get_AsyncFuture().IsValid())
        { JoltWorld->WaitForAsyncStep(); }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_JoltStaticActor_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_JoltStaticActor_Current& InCurrent) const
        -> void
    {
        auto* Subsystem = _StaticWorldSubsystem.Get();
        if (ck::Is_NOT_Valid(Subsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        Subsystem->Request_RemoveBodiesForEntity(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
