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
// Hosted wrapper for a typed script processor. Satisfies ck::concepts::FTickable_Concept (Tick + Pump) so the
// scheduler drives it alongside C++ TProcessors, exactly like the legacy FProcessor_ScriptHosted it supersedes.
//
// Per tick it performs the native N-way join over the declared dynamic-fragment storages (driving the smallest pool,
// contains()-probing the rest, honoring Require/Exclude + the destroy filter) into a persistent, reused entity list,
// then makes ONE ForEachBatch call into the batch instance. That is the whole point: O(1) native->script crossings per
// tick instead of one per visited entity.
//
// Two instances: the DEV instance (the author's UCk_Processor_Script_Base_UE subclass — owns BeginPlay/EndPlay and the
// typed ForEachEntity) and the BATCH instance (the generated <Dev>_Driver whose ForEachBatch loops the batch and
// forwards to the dev's ForEachEntity). In direct mode (InDriverClass == nullptr) the dev class overrides ForEachBatch
// itself and the two instances are the same object.
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
        // Resolve the declared slots' storages into _BatchState and fill _BatchState._Entities via the join. Returns
        // false for the empty-join early-out (a required/read pool is empty, or only Exclude slots were declared) so
        // the caller skips the VM call entirely.
        auto DoResolveAndJoin() -> bool;

        // Construct the batch over the current _BatchState, call the batch instance's ForEachBatch, then bump the
        // generation so any batch the script stashed past the call is detectably stale.
        auto DoDispatchBatch(TimeType InDeltaT) -> void;

    private:
        RegistryType                                   _Registry;
        TStrongObjectPtr<UCk_Processor_Script_Base_UE> _DevInstance;    // BeginPlay/EndPlay + ForEachEntity target
        TStrongObjectPtr<UCk_Processor_Script_Base_UE> _BatchInstance;  // generated driver, or == dev in direct mode
        FCk_ScriptProcessorQuery                       _Query;          // resolved once at construction
        FCk_ScriptQueryBatchState                      _BatchState;     // per-tick native state handed to script
        bool                                           _Disabled = false;

        // One stat row per script processor class in stat CkProcessors, alongside the C++
        // TProcessor rows — covers the native join AND the ForEachBatch VM call; without it
        // the whole cost hides in the scheduler's Dispatch self-time.
        TStatId                                        _TickStatId;
    };
}
