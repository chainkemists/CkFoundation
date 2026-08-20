#pragma once

#include "CkCore/Concepts/CkConcepts.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Registry/CkRegistry.h"

#include <GameplayTagContainer.h>

#include "CkProcessorDescriptor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_ProcessorNetMode : uint8
{
    AllNetModes,
    Override
};

UENUM()
enum class ECk_TickGroupMode : uint8
{
    Inherit,
    Explicit
};

UENUM()
enum class ECk_UnresolvedRefPolicy : uint8
{
    Permissive,
    Strict
};

// --------------------------------------------------------------------------------------------------------------------
// Which world types a processor may run in. Anything that dereferences a NetDriver or assumes a live game world
// must opt out of the editor-world graph via RuntimeOnly. The graph builder turns a mismatched processor into a
// ghost node: its RunAfter/RunBefore edges still order the rest of the graph, but ForEachEntity never fires.
UENUM()
enum class ECk_ProcessorWorldTypeRequirement : uint8
{
    All,
    RuntimeOnly,
    EditorOnly,
};

// Parallel to ECk_ProcessorWorldTypeRequirement — selects which subsystem is currently building the graph.
UENUM()
enum class ECk_ProcessorWorldTypeContext : uint8
{
    Runtime,
    Editor,
};

// --------------------------------------------------------------------------------------------------------------------
// Whether a processor participates in the scheduler's pump phase (DoTick with DeltaT=0, so cascading reactive work
// drains in one frame). SkipPump is required only when BOTH hold: the processor reads a STICKY marker it does not
// consume, AND it is not idempotent w.r.t. DeltaT (an apply-offset consumer would enqueue its offset once per pump
// pass). Either condition alone tolerates pumping. Worked example: CkEcs/CLAUDE.md § Pump policy.
UENUM()
enum class ECk_ProcessorPumpPolicy : uint8
{
    Default,    // Pump-eligible: invoked with DeltaT=0 in the pump phase if MarkedDirtyBy entities exist.
    SkipPump,   // Pump-skipped: only the main Tick phase invokes this processor. Use for time-stepping consumers.
};

// --------------------------------------------------------------------------------------------------------------------
// Whether a processor ticks while a CkSnapshot load is rebuilding the world. Only the framework
// construction/lifecycle/hydration kernel opts in — the scheduler's LoadKernel tick scope iterates only
// RunsDuringLoad nodes. A feature processor marked RunsDuringLoad runs against half-rebuilt state; a kernel
// processor left gated hangs the load.
UENUM()
enum class ECk_ProcessorLoadPolicy : uint8
{
    GatedDuringLoad,   // Default: skipped while the load gate is active.
    RunsDuringLoad,    // Framework kernel only: ticks during the load rebuild.
};

// --------------------------------------------------------------------------------------------------------------------
// Whether the scheduler's main pass may skip a processor whose view is PROVABLY empty (some required include type
// has zero LIVE entities), bypassing the whole dispatch. Eligibility is automatic and conservative: only processors
// whose DoTick IS the template-generated view iteration qualify automatically — a custom DoTick may do work not gated
// on that view, which a skip would silently drop. A custom processor may opt in by declaring
// `using MainPassRequiredFragments = entt::type_list<...>` when ALL of its main-pass work requires at least one live
// entity with every listed fragment. Tracking and the AlwaysTick escape hatch: CkEcs/CLAUDE.md § Empty-view skip.
UENUM()
enum class ECk_ProcessorEmptyViewPolicy : uint8
{
    SkipWhenProvablyEmpty,   // Default for eligible processors: skip dispatch while the view is provably empty.
    AlwaysTick,              // Opt out: dispatch every frame even when the view is provably empty.
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FProcessorFactory = TFunction<concepts::FTickableType(const FCk_Registry&)>;
    using FDirtyChecker = TFunction<bool(const FCk_Registry&)>;
    using FEmptyViewChecker = TFunction<bool(const FCk_Registry&)>;

    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FProcessorDescriptor
    {
        CK_GENERATED_BODY(FProcessorDescriptor);

        FName _Name;

        // Optional short identity for human-facing surfaces, which fall back to _Name when unset.
        // _Name stays the canonical registry key that RunAfter/RunBefore references resolve against.
        FName _DisplayName;

        FProcessorFactory _Factory;

        FName _GroupName;

        // Optional group boundary after which this canonical processor may be replayed with DeltaT=0 as
        // part of a local settle barrier. Participation and activation are deliberately separate: only a
        // processor declaring LocalSettleTrigger activates the plan from its consumed dirty marker.
        FName _LocalSettleAfterGroupName;
        bool _IsLocalSettleTrigger = false;

        TArray<FName> _RunAfter;
        TArray<FName> _RunBefore;

        FGameplayTagContainer _RunAfterTags;
        FGameplayTagContainer _RunBeforeTags;

        // Pump-eligible when the processor declares `MarkedDirtyBy` or `MarkedDirtyByAnyOf`. The hash
        // and name arrays are index-aligned; _IsDirtyChecker is true when ANY marker has entities.
        bool _HasDirtyMarker = false;
        FDirtyChecker _IsDirtyChecker;
        TArray<uint32> _DirtyMarkerHashes;
        TArray<FName> _DirtyMarkerNames;

        // Inferred from the TReadOnly<F>/TReadWrite<F> wrappers on the processor's template parameter
        // list; drives write-write conflict detection. Name arrays are index-aligned, diagnostics only.
        TArray<uint32> _RO_FragmentHashes;
        TArray<uint32> _RW_FragmentHashes;
        TArray<FName>  _RO_FragmentNames;
        TArray<FName>  _RW_FragmentNames;

        // Main-pass empty-view skip metadata (see ECk_ProcessorEmptyViewPolicy above). Includes are the
        // view's REQUIRED types, minus TExclude<> (an exclude only shrinks a view) and TIgnoreInEditor<>
        // (world-variant-dependent, so conservatively left out). _IsViewProvablyEmpty is true when ANY
        // include has zero LIVE entities — tombstone-aware, unlike the pump's Has_AnyEntityWith.
        bool _CanSkipWhenViewEmpty = false;
        FEmptyViewChecker _IsViewProvablyEmpty;
        TArray<uint32> _ViewIncludeHashes;
        TArray<FName> _ViewIncludeNames;

        ECk_ProcessorNetMode _NetModeRequirement = ECk_ProcessorNetMode::AllNetModes;
        ECk_ProcessorNetModeRequirement _NetModeRequirementValue = ECk_ProcessorNetModeRequirement::All;

        ECk_ProcessorWorldTypeRequirement _WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::All;

        ECk_TickGroupMode _TickGroupMode = ECk_TickGroupMode::Inherit;
        ETickingGroup _TickGroupValue = TG_PrePhysics;

        // Both are opt-out/opt-in via a `static constexpr auto PumpPolicy` / `LoadPolicy` trait on the
        // processor; see the enum docs above for when each is required.
        ECk_ProcessorPumpPolicy _PumpPolicy = ECk_ProcessorPumpPolicy::Default;
        ECk_ProcessorLoadPolicy _LoadPolicy = ECk_ProcessorLoadPolicy::GatedDuringLoad;

        FGameplayTag _SchedulerTag;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // A conflict is: two processors that both write the same fragment, in the same tick group, with no explicit
    // RunAfter/RunBefore ordering between them. _AutoInserted is true when Permissive policy auto-inserted an edge
    // to resolve it; false when Strict mode reported it and Build() failed.
    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FCk_WriteConflictInfo
    {
        CK_GENERATED_BODY(FCk_WriteConflictInfo);

        FName _First;
        FName _Second;
        FName _FragmentName;
        bool  _AutoInserted = false;
    };
}

// --------------------------------------------------------------------------------------------------------------------
