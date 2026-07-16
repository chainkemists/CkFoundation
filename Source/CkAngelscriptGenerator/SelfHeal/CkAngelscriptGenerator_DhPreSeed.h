#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Proactive DynamicHandle JSON pre-seed (boot-time).
//
// A pull that adds a new dynamic handle (`asset X of UCkDynamic_HandleDefinition`
// declaring `TypeName = "FCk_Handle_Y"`) leaves the machine-local canonical
// `Script/Generated/DynamicHandleTypes.json` without the entry. The PreCompile
// hook then registers no AS binding for it, the FIRST AS compile fails with
// `Identifier 'FCk_Handle_Y' is not a data type`, and the bootstrap self-heal
// dispatcher recovers via the modal path: failed compile → stub synthesis →
// hot-reload → full second compile (~6-8s + the Hazelight error modal, on
// every such boot, for every teammate).
//
// This pre-seed produces the OUTCOME of that heal cycle before the first
// compile runs: at StartupModule (after the stub sweep, before any AS
// compile), textually scan the project + enabled-plugin Script/ trees for
// handle-definition blocks, diff the declared TypeNames against the canonical
// JSON, and seed each missing one into the `_StubRecovery_DynamicHandleTypes.json`
// sibling via the SAME entry writer the heal path uses
// (FCkAsRecoveryDispatcher::Append_DynamicHandleStubEntries). Everything
// downstream already exists: the PreCompile load merges the sibling, the
// first compile sees the binding and succeeds, and the Mark_JsonStubSynthesized
// flag arms the OnPostEngineInit regen that rewrites the canonical from the
// now-discoverable data asset and upgrades the permissive validator to strict.
//
// The pre-seed is an OPTIMIZATION LAYER over the heal path, never a
// replacement: anything the scanner misses fails the compile exactly as today
// and the dispatcher recovers it. Scanner failures are therefore not
// ensure-worthy — a file that fails to load/parse simply contributes no
// declarations.
//
// Timing safety: StartupModule runs before the first AS compile and before
// Hazelight's hot-reload checker thread exists — and the sibling is a .json,
// which that thread doesn't watch anyway. Two things must run FIRST:
//   1. the startup stub sweep (Delete_AllStubRecoveryFiles) — it deletes
//      _StubRecovery_* and would erase the seed;
//   2. FCkAsRecoveryDispatcher::Reset_CyclesRun() — it clears the
//      Did_SynthesizeJsonStub flag this funnel sets to arm the
//      OnPostEngineInit canonical regen; seeding before the reset wipes the
//      flag and the canonical never converges.
// The module therefore calls the pre-seed inside the self-heal arming block,
// right after Reset_CyclesRun().

namespace ck::angelscriptgenerator::self_heal
{
    // One `asset <Name> of UCkDynamic_HandleDefinition` block found in AS source.
    struct CKANGELSCRIPTGENERATOR_API FCk_DhDeclaration
    {
        FString TypeName;
        FString ShortName;      // explicit from the block when set, else derived (registry-load parity)
        FString SourceFilePath; // declaring .as — diagnostics only
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsDhPreSeed
    {
    public:
        // Pure-text parse of one file's contents for handle-definition blocks.
        // Comment/string-aware via FCkAsSourceScanner::Blank_CommentsAndStrings,
        // so commented-out blocks (both `//` and `/* */`) never seed a phantom
        // entry and tricky Description strings can't fool the structural scan.
        // ShortName falls back to FCkDynamic_HandleTypeRegistry::
        // ExtractShortNameFromTypeName when the block doesn't set it (the same
        // derivation the registry load applies to an entry without one).
        static auto Parse_HandleDefinitions(
            const FString& InFileContents,
            const FString& InSourcePathForDiagnostics) -> TArray<FCk_DhDeclaration>;

        // Scans every *.as under the roots (FCkAsSourceScanner enumeration —
        // Generated/ and _StubRecovery_* excluded), deduped by TypeName
        // (first declaring file wins; files are sorted), sorted by TypeName.
        static auto Scan_ScriptTrees_ForHandleDefinitions(
            const TArray<FString>& InScanRoots) -> TArray<FCk_DhDeclaration>;

        // Module-facing boot funnel (ownership-gated, G12): scan the default
        // roots, diff against the canonical registry JSON, seed missing
        // TypeNames into the sibling, arm the deferred regen. Logs the scan
        // duration. Returns the number seeded (0 = no writes, no flag).
        static auto PreSeed_MissingDynamicHandles() -> int32;

        // Injectable core (paths + declarations supplied) — reachable only
        // under the G12 gate in production; exposed for unit tests. A missing
        // or unparsable canonical counts as "no known TypeNames": seeding
        // everything is the productive recovery there too (the post-init
        // regen rewrites the canonical either way).
        static auto PreSeed_MissingDynamicHandles(
            const FString& InCanonicalJsonPath,
            const TArray<FCk_DhDeclaration>& InDeclarations) -> int32;
    };
}

// --------------------------------------------------------------------------------------------------------------------
