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
// Declares which world types a processor is permitted to run in. Default is 'All' — the processor participates in
// both the runtime (Game/PIE) graph and the editor-world graph. Processors that dereference a NetDriver, call into
// AGameState/AGameMode, or otherwise assume a live game world must opt out via 'RuntimeOnly'. Debug visualizations
// that only make sense when authoring a level use 'EditorOnly'.
//
// The graph builder turns mismatched processors into ghost nodes — they keep their RunAfter/RunBefore edges so the
// topological ordering of the rest of the graph remains correct, but their ForEachEntity never fires.
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
// Controls whether a processor participates in the scheduler's pump phase. The pump phase invokes a processor's
// DoTick(DeltaT=0) so that cascading reactive work (a setup processor stamping a NeedsX tag that another setup
// processor consumes within the same frame) drains in one frame instead of slipping per-stage.
//
// Default is correct for processors that consume a TRANSIENT marker — i.e. the processor's body removes/drains the
// MarkedDirtyBy fragment as part of its work (FTag_*_NeedsSetup, FFragment_*_Requests). For these, repeated pumping
// is naturally bounded: once the marker is gone, _IsDirtyChecker returns false and the pump pass stops invoking it.
//
// SkipPump is required for processors that:
//   1. Read a sticky marker the processor does NOT consume (e.g. FTag_EulerIntegrator_NeedsUpdate, which lives on
//      the entity for as long as the entity participates in integration), AND
//   2. Are not idempotent w.r.t. DeltaT — e.g. an apply-offset consumer that enqueues a Request_AddLocationOffset
//      every time it runs. Pumping such a processor double-applies on the same DistanceOffset value within the same
//      frame, multiplying observed motion by the number of pump passes.
//
// The two conditions in combination are what makes SkipPump necessary; either alone is fine. A processor that reads
// a sticky marker but is fully idempotent (e.g. just inspects state) tolerates being pumped. A non-idempotent
// processor that consumes its marker tolerates pump because subsequent pumps see no work to do.
UENUM()
enum class ECk_ProcessorPumpPolicy : uint8
{
    Default,    // Pump-eligible: invoked with DeltaT=0 in the pump phase if MarkedDirtyBy entities exist.
    SkipPump,   // Pump-skipped: only the main Tick phase invokes this processor. Use for time-stepping consumers.
};

// --------------------------------------------------------------------------------------------------------------------
// Whether a processor ticks while a CkSnapshot load is rebuilding the world. Default = gated: feature processors do
// nothing mid-load and need no knowledge of loads. Only the framework construction/lifecycle/hydration kernel opts in
// (spec §4.3) — the scheduler's LoadKernel tick scope iterates only RunsDuringLoad nodes. Marking a feature processor
// RunsDuringLoad is a bug (it would run against half-rebuilt state); leaving a kernel processor gated hangs the load.
UENUM()
enum class ECk_ProcessorLoadPolicy : uint8
{
    GatedDuringLoad,   // Default: skipped while the load gate is active.
    RunsDuringLoad,    // Framework kernel only: ticks during the load rebuild.
};

// --------------------------------------------------------------------------------------------------------------------
// Controls whether the scheduler's main pass may skip a processor whose view is PROVABLY empty — i.e. some required
// (include) fragment/tag type of its view has zero LIVE entities in the registry, so the generated DoTick would
// visit nothing. Skipping bypasses the entire dispatch: no Tick call, no view construction, no tombstone walk, no
// per-processor trace/stat/debug-timing overhead.
//
// Eligibility is automatic and conservative: only processors whose DoTick IS the template-generated view iteration
// (ck::TProcessor / ck_exp::TProcessor, not shadowed by a custom DoTick) qualify — a custom DoTick body may do work
// that is not gated on the view (drain external queues, poll UE state), which a skip would silently drop. Hand-built
// descriptors (script processors) and TParallelProcessor never qualify. AlwaysTick is the escape hatch for a
// template processor with unusual expectations:
//
//     static constexpr auto EmptyViewPolicy = ECk_ProcessorEmptyViewPolicy::AlwaysTick;
//
// Emptiness tracking mirrors the pump's dirty short-circuit: the include types' mutation version counters are
// summed and compared against a per-node cache; the (tombstone-aware) storage scan re-runs only when some counter
// moved. The check runs at the node's dispatch position, so an include added by an earlier processor in the same
// frame wakes the node the same frame — zero latency vs. the always-tick path.
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

        // Optional short identity for human-facing surfaces (per-processor Insights trace scopes).
        // _Name stays the canonical registry key (C++ type name / script class path name) that
        // RunAfter/RunBefore references resolve against; surfaces fall back to _Name when this is unset.
        FName _DisplayName;

        FProcessorFactory _Factory;

        FName _GroupName;

        TArray<FName> _RunAfter;
        TArray<FName> _RunBefore;

        FGameplayTagContainer _RunAfterTags;
        FGameplayTagContainer _RunBeforeTags;

        // A processor is pump-eligible when it declares at least one dirty marker — either a single
        // `using MarkedDirtyBy = FTag_X;` or `using MarkedDirtyByAnyOf = TDepList<FTag_A, FTag_B, ...>;`
        // (composites whose internal sub-processors consume several distinct markers). The hash and
        // name arrays are index-aligned; _IsDirtyChecker returns true when ANY marker has entities.
        bool _HasDirtyMarker = false;
        FDirtyChecker _IsDirtyChecker;
        TArray<uint32> _DirtyMarkerHashes;
        TArray<FName> _DirtyMarkerNames;

        // Fragment access metadata inferred from TReadOnly<F>/TReadWrite<F> wrappers on the processor's
        // template parameter list. Used by FProcessorGraphBuilder to detect write-write conflicts between
        // processors that touch the same fragment without an explicit RunAfter/RunBefore ordering.
        // The name arrays are index-aligned with their matching hash arrays and exist purely for
        // human-readable diagnostics (debugger surfaces, log messages).
        TArray<uint32> _RO_FragmentHashes;
        TArray<uint32> _RW_FragmentHashes;
        TArray<FName>  _RO_FragmentNames;
        TArray<FName>  _RW_FragmentNames;

        // Main-pass empty-view skip metadata (see ECk_ProcessorEmptyViewPolicy above). The include
        // hashes are the view's REQUIRED types — access-wrapped data fragments AND filter tags —
        // with TExclude<...> entries stripped (an exclude only shrinks a view; it cannot make an
        // empty intersection non-empty) and TIgnoreInEditor<...> entries stripped (their
        // participation is world-variant-dependent, so they are conservatively not part of the
        // provable-empty set). _IsViewProvablyEmpty returns true when ANY include type has zero
        // LIVE entities (tombstone-aware Has_AnyLiveEntityWith, not the pump's Has_AnyEntityWith).
        // The name array is index-aligned with the hashes, diagnostics only.
        bool _CanSkipWhenViewEmpty = false;
        FEmptyViewChecker _IsViewProvablyEmpty;
        TArray<uint32> _ViewIncludeHashes;
        TArray<FName> _ViewIncludeNames;

        ECk_ProcessorNetMode _NetModeRequirement = ECk_ProcessorNetMode::AllNetModes;
        ECk_ProcessorNetModeRequirement _NetModeRequirementValue = ECk_ProcessorNetModeRequirement::All;

        ECk_ProcessorWorldTypeRequirement _WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::All;

        ECk_TickGroupMode _TickGroupMode = ECk_TickGroupMode::Inherit;
        ETickingGroup _TickGroupValue = TG_PrePhysics;

        // Default pumps; processors opt out via `static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;`.
        // See enum docs in this header for when SkipPump is required.
        ECk_ProcessorPumpPolicy _PumpPolicy = ECk_ProcessorPumpPolicy::Default;

        // Default gated during a load; the kernel opts in with a `static constexpr auto LoadPolicy = RunsDuringLoad;`
        // trait (spec §4.3 — see ECk_ProcessorLoadPolicy above).
        ECk_ProcessorLoadPolicy _LoadPolicy = ECk_ProcessorLoadPolicy::GatedDuringLoad;

        FGameplayTag _SchedulerTag;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // One record per detected write-write conflict. Populated by DoValidateAndResolveWriteConflicts and forwarded
    // to the per-partition conflict list so the scheduler debugger can surface it in the Inspector panel.
    // A conflict is: two processors that both write the same fragment, in the same tick group, with no explicit
    // RunAfter/RunBefore ordering between them. _AutoInserted is true when the Permissive policy auto-inserted an
    // edge to resolve the conflict; false when Strict mode reported it and the Build() failed.
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
