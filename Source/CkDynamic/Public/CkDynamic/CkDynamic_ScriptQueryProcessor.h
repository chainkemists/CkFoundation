#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Processor/CkProcessor_ScriptQuery_Data.h"

#include "CkDynamic/CkDynamic_ScriptQueryBatch.h"   // FCk_ScriptQueryBatchState

#include <Stats/Stats.h>
#include <UObject/StrongObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UClass;
class UCk_Processor_Script_Base_UE;

// --------------------------------------------------------------------------------------------------------------------
// Hosted wrapper for a typed script processor — the sole hosting path. Satisfies ck::concepts::FTickable_Concept
// so the scheduler drives it alongside C++ TProcessors.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKDYNAMIC_API FProcessor_ScriptQueryHosted
    {
    public:
        CK_GENERATED_BODY(FProcessor_ScriptQueryHosted);

    public:
        using TimeType = FCk_Time;
        using RegistryType = FCk_Registry;

    public:
        // InDriverClass == nullptr means direct mode: InDevClass overrides ForEachBatch itself.
        FProcessor_ScriptQueryHosted(
            const RegistryType& InRegistry,
            UClass* InDevClass,
            UClass* InDriverClass);

        FProcessor_ScriptQueryHosted(const FProcessor_ScriptQueryHosted&) = delete;
        auto operator=(const FProcessor_ScriptQueryHosted&) -> FProcessor_ScriptQueryHosted& = delete;

        FProcessor_ScriptQueryHosted(FProcessor_ScriptQueryHosted&&) noexcept = default;
        auto operator=(FProcessor_ScriptQueryHosted&&) noexcept -> FProcessor_ScriptQueryHosted& = default;

        ~FProcessor_ScriptQueryHosted();

    public:
        auto Tick(TimeType InDeltaT) -> void;
        auto Pump() -> int32;

    private:
        // False means the empty-join early-out, so the caller skips the VM call entirely.
        auto DoResolveAndJoin() -> bool;

        // Bumps the generation on the way out so a batch the script stashed past the call is detectably stale.
        auto DoDispatchBatch(TimeType InDeltaT) -> void;

    private:
        RegistryType                                   _Registry;
        TStrongObjectPtr<UCk_Processor_Script_Base_UE> _Instance;       // the driver (subclass of dev), or the dev itself in direct mode
        FCk_ScriptProcessorQuery                       _Query;          // resolved once at construction
        FCk_ScriptQueryBatchState                      _BatchState;     // per-tick native state handed to script
        bool                                           _Disabled = false;

        // What the last Tick actually handed to the VM, so Pump() can report real work instead of the
        // pessimistic -1 sentinel. -1 stays for the NoEntities query shape: that body runs with no join
        // and may mutate state, so it is not inspectable.
        int32                                          _LastTickVisitedCount = 0;

        // Covers the native join AND the ForEachBatch VM call; without it the cost hides in Dispatch self-time.
        TStatId                                        _TickStatId;
    };
}
