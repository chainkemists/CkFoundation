#include "CkJoltDebugDraw_Processor.h"

#include "CkCore/Validation/CkIsValid_Defaults.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget.h"
#include "CkJolt/Subsystem/CkJolt_DebugRenderer.h"
#include "CkJolt/Subsystem/CkJolt_Subsystem.h"

#include <Engine/World.h>

#include <Jolt/Physics/PhysicsSystem.h>

// --------------------------------------------------------------------------------------------------------------------

#if JPH_DEBUG_RENDERER
CK_REGISTER_PROCESSOR(ck::FProcessor_JoltDebugDraw_Capture);
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_debugdraw_processor
{
    // An absent context is legal (a world with no Jolt subsystem never publishes one).
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
    FProcessor_JoltDebugDraw_Capture::
        FProcessor_JoltDebugDraw_Capture(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    auto
        FProcessor_JoltDebugDraw_Capture::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
#if JPH_DEBUG_RENDERER
        auto* JoltWorld = ck_jolt_debugdraw_processor::TryResolve_JoltWorld(_TransientEntity);
        if (JoltWorld == nullptr)
        { return; }

        auto* World = JoltWorld->Get_World().Get();
        if (ck::Is_NOT_Valid(World))
        { return; }

        auto* Subsystem = World->GetSubsystem<UCk_Jolt_Subsystem>();
        if (ck::Is_NOT_Valid(Subsystem))
        { return; }

        auto Targets = TArray<TSharedPtr<FCk_Jolt_DebugDrawTarget>>{};
        Subsystem->Get_DemandingDebugDrawTargets(Targets);

        // A closed debugger still costs one map walk and no more, unless something asks for contacts — the
        // in-world target is captured from the subsystem's Tick, so the replay is the only thing this processor
        // owes it.
        const auto AnyContactDemand = ck::jolt::debug_draw::Get_IsAnyTargetDemandingContacts();

        if (Targets.IsEmpty() && NOT AnyContactDemand)
        { return; }

        const auto PhysicsSystem = Subsystem->Get_PhysicsSystem().Pin();
        if (NOT PhysicsSystem.IsValid())
        { return; }

        auto& Renderer = FCk_Jolt_DebugRenderer::Get_OrCreate();

        const auto Revisions = ck::jolt::debug_draw::FCaptureRevisions{
            JoltWorld->Get_StaticSceneRevision(),
            JoltWorld->Get_BodyRemovedRevision()};

        // BEFORE the captures, because a capture flushes the target's line component: the contacts recorded
        // around the step have to be in the channel by the time the capture that draws this frame's bodies
        // pushes it. This processor is the ONE consumer of this world's buffer — the step never replays it,
        // which is what keeps every touch of a target on the game thread. That single-consumer rule is also why
        // the subsystem's own in-world target is fed here rather than from its Tick: a second drain point would
        // race this one for the same record.
        auto ReplayTargets = Targets;

        if (const auto DefaultTarget = Subsystem->Get_DefaultDebugDrawTarget(); DefaultTarget.IsValid())
        { ReplayTargets.AddUnique(DefaultTarget); }

        ck::jolt::debug_draw::Replay_RecordedContacts(PhysicsSystem.Get(), ReplayTargets);

        for (const auto& Target : Targets)
        { Renderer.Capture_JoltWorld(*Target, *PhysicsSystem, Revisions, _TransientEntity); }
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
