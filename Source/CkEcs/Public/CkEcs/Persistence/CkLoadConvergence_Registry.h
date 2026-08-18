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
    };
}

// --------------------------------------------------------------------------------------------------------------------
