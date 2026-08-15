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

        if (Targets.IsEmpty())
        { return; }

        const auto PhysicsSystem = Subsystem->Get_PhysicsSystem().Pin();
        if (NOT PhysicsSystem.IsValid())
        { return; }

        auto& Renderer = FCk_Jolt_DebugRenderer::Get_OrCreate();

        const auto Revisions = ck::jolt::debug_draw::FCaptureRevisions{
            JoltWorld->Get_StaticSceneRevision(),
            JoltWorld->Get_BodyRemovedRevision()};

        for (const auto& Target : Targets)
        { Renderer.Capture_JoltWorld(*Target, *PhysicsSystem, Revisions, _TransientEntity); }
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
