#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Registry/CkRegistry.h"

#include <Stats/Stats.h>
#include <UObject/StrongObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UClass;
class UCk_Processor_Script_Base_UE;

// --------------------------------------------------------------------------------------------------------------------
// Non-templated processor wrapper that satisfies ck::concepts::FTickable_Concept (Tick + Pump), letting the
// scheduler drive AngelScript/Blueprint processors uniformly alongside C++ TProcessors.
//
// Owns a heap-spawned instance of the author's UCk_Processor_Script_Base_UE subclass via TStrongObjectPtr.
//
// Lifetime contract: the hosted instance's outer is GetTransientPackage(), NOT a UWorld. Its lifetime is
// pinned by the TStrongObjectPtr here and released when this wrapper is destroyed during processor-graph
// teardown (see UCk_EcsWorld_Subsystem_UE::DoTeardownAndRebuild). Do not reach UWorld via _Instance->GetOuter()
// — go through the registry handle injected via Set_Handle instead.
//
// On each Tick/Pump the wrapper:
//   1. Checks the MarkedDirtyBy gate (if set), skipping the call when the ECS has no matching dirty fragment.
//   2. Invokes the hosted instance's Tick UFUNCTION, which itself iterates via
//      UCk_Utils_DynamicFragment_UE::ForEach_EntityWith*Fragments.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECS_API FProcessor_ScriptHosted
    {
    public:
        CK_GENERATED_BODY(FProcessor_ScriptHosted);

    public:
        using TimeType = FCk_Time;
        using RegistryType = FCk_Registry;

    public:
        // The MarkedDirtyBy gate is enforced by the scheduler before it calls this wrapper's Tick, so the
        // wrapper itself does not need to perform the dirty check.
        FProcessor_ScriptHosted(
            const RegistryType& InRegistry,
            UClass* InScriptClass);

        FProcessor_ScriptHosted(const FProcessor_ScriptHosted&) = delete;
        auto operator=(const FProcessor_ScriptHosted&) -> FProcessor_ScriptHosted& = delete;

        FProcessor_ScriptHosted(FProcessor_ScriptHosted&&) noexcept = default;
        auto operator=(FProcessor_ScriptHosted&&) noexcept -> FProcessor_ScriptHosted& = default;

        ~FProcessor_ScriptHosted();

    public:
        auto Tick(TimeType InDeltaT) -> void;
        auto Pump() -> int32;

    private:
        RegistryType _Registry;
        TStrongObjectPtr<UCk_Processor_Script_Base_UE> _Instance;

        // One stat row per script processor class in stat CkProcessors, alongside the C++
        // TProcessor rows — without it the whole script Tick cost hides in the scheduler's
        // Dispatch self-time. Cached-dynamic-id pattern, same as the EcsWorld actor tick.
        TStatId _TickStatId;
    };
}

// --------------------------------------------------------------------------------------------------------------------
