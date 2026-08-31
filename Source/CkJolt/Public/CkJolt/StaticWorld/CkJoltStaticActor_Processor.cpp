#include "CkJoltStaticActor_Processor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h"
#include "CkJolt/World/CkJoltWorld.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_JoltStaticActor_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("JoltStaticActor_EndPlayWaitForAsync"), STAT_CkJolt_StaticActorEndPlayWaitForAsync,
    STATGROUP_CkJolt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_static_actor_processor
{
    // A world with no Jolt subsystem never publishes the context — a null return is legal.
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
        // A world tearing down may already have destroyed it — null makes every entity below a silent skip.
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);
        _StaticWorldSubsystem = ck::IsValid(World)
            ? World->GetSubsystem<UCk_JoltStaticWorld_Subsystem_UE>()
            : nullptr;

        // ASYNC GUARD (house rule for Jolt *_EndPlay processors): the removal funnel below mutates the
        // broadphase an in-flight Update is reading. Self-guarded on future validity, so free in sync mode.
        auto* JoltWorld = ck_jolt_static_actor_processor::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld != nullptr && JoltWorld->Get_AsyncFuture().IsValid())
        {
            SCOPE_CYCLE_COUNTER(STAT_CkJolt_StaticActorEndPlayWaitForAsync);
            JoltWorld->WaitForAsyncStep();
        }

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
        if (ck::Is_NOT_Valid(Subsystem))
        { return; }

        Subsystem->Request_RemoveBodiesForEntity(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
