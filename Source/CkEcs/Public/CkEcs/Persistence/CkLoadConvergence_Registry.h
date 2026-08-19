#pragma once

// Convergence facts a load waits on before it hands the world back. Registered by whichever module OWNS the
// fact (physics bodies by CkJolt, probe overlaps by CkSpatialQuery, the scheduler's own quiescence by CkEcs), so
// CkEcs needs no knowledge of any of them and CkSnapshot needs no build dependency on any of them either.

#include "CkEcs/Registry/CkRegistry.h"

#include <Containers/Array.h>
#include <Templates/Function.h>
#include <UObject/NameTypes.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // TRI-state, deliberately. A world with no Jolt world cannot register bodies, and answering Pending there
    // would make every Jolt-less load burn the convergence cap and report losses it never had — while answering
    // Satisfied would claim a fact nobody established. NotApplicable says the third thing: this question does
    // not arise in this world.
    enum class ECk_LoadConvergence : uint8
    {
        Satisfied,
        Pending,
        NotApplicable
    };

    // --------------------------------------------------------------------------------------------------------------

    // What the convergence phase has done so far, written once per frame by whoever drives it and read by the
    // predicates. It lives in the registry because a predicate is handed nothing else — and because that is what
    // lets a predicate stay a pure read of state somebody else already produced.
    struct FCtx_LoadConvergence
    {
        // Frames spent converging so far. Zero means nothing has been driven yet, so a predicate reading a quiet
        // world must not mistake "not started" for "settled".
        int32 _FramesConverging = 0;

        // The last frame's pump outcome, recorded by the driver. The SKIPPED count is not decoration: a tick group
        // that could not be pumped contributes zero passes, which reads identically to one that ran and found
        // nothing to do.
        int32 _PumpCountLastFrame = 0;
        int32 _PumpSkippedGroupsLastFrame = 0;

        // Monotonic counters sampled at the moment the phase began, so a predicate can ask "how much has happened
        // SINCE the hold" without keeping state of its own.
        int32 _PhysicsGrantedStepsBaseline = 0;

        // How many of those granted steps have actually executed since the phase began, republished each frame by
        // the module that owns the counter. Nothing reads it to decide anything — it exists so whoever drives the
        // phase can SAY how far physics got, without taking a dependency on the module that stepped it.
        int32 _PhysicsGrantedStepsSinceHold = 0;
    };

    // --------------------------------------------------------------------------------------------------------------

    struct CKECS_API FCk_LoadConvergenceRegistry
    {
        // PURE. A predicate READS the outcome of work that already happened and says whether it is done; it must
        // never pump, tick, request or otherwise cause the thing it is measuring. A side-effecting predicate
        // doubles the phase's work, reorders it against whatever the phase is driving, and — worst — reports on
        // a world it just changed, which is how a convergence check ends up certifying its own side effects.
        using FPredicate = TFunction<ECk_LoadConvergence(const FCk_Registry&)>;

        // Registering the same name twice REPLACES: registration is idempotent so a module that re-registers
        // (hot reload, a test installing its own row) does not accumulate duplicates under one name.
        static auto
        Register(
            FName InName,
            FPredicate InPredicate) -> void;

        static auto
        Unregister(
            FName InName) -> void;

        // Every registered fact that reports Pending, in registration order. EMPTY means converged — Satisfied
        // and NotApplicable are both "nothing to wait for here", and only the caller's bounded escape decides
        // what to do when this stops emptying.
        static auto
        Get_Pending(
            const FCk_Registry& InRegistry) -> TArray<FName>;

        // The other half of a fact that has to be DRIVEN rather than merely observed. Physics is the case: a load
        // freezes game time, so the frame's delta plans zero steps and the bodies a predicate is waiting on would
        // never move. The owning module registers the work here, the driver runs it once per frame BEFORE reading
        // the predicates, and the predicate itself stays pure — which is the whole reason the two are separate
        // registries rather than one side-effecting call.
        // NON-const, unlike a predicate: an advance drives work, and the first thing several of them need to do is
        // stamp their own baseline into FCtx_LoadConvergence so the predicate that reads it stays stateless.
        using FAdvance = TFunction<void(FCk_Registry&)>;

        static auto
        Register_Advance(
            FName InName,
            FAdvance InAdvance) -> void;

        static auto
        Unregister_Advance(
            FName InName) -> void;

        // Runs every registered advance once, in registration order.
        static auto
        Run_Advances(
            FCk_Registry& InRegistry) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
